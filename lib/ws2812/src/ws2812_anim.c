
#include <stdbool.h>
#include <stdio.h>      // for dbg
#include <string.h>     // for memcpy

#include "ws2812_p.h"   // for color and NR_ROWS, NR_COLUMNS

#include "ws2812_anim_obj.h"
#include "ws2812_transition_obj.h"
#include "ws2812_anim.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

// animations
#include "ws2812_anim_const_color.h"


// ------------------- debug ------------------------

#define dbg_on(x...)      printf(x)
#define dbg_off(x...)

#define dbg dbg_off


#define dbg_err_on(x...)      printf(x)
#define dbg_err_off(x...)

#define dbg_err dbg_err_on

// ------------------- defines ----------------------

/*! Enumerates the animation states */
typedef enum {

    /*! Transition state */
    WS2812_ANIM_STATE_TRANSIT,

    /*! Main state */
    WS2812_ANIM_STATE_MAIN,

} te_ws2812_animation_state;

/*! Enumerates the animations */
typedef enum {

    /*! Constant color animation */
    WS2812_ANIMATION_CONSTANT_COLOR = 0,

} te_ws2812_animations;


/*! Enumerates the transitions */
typedef enum {

    /*! Fade effect */
    WS2812_TRANSITION_FADE = 0,

} te_ws2812_transitions;


/*! Defines a message */
typedef struct {

    /*! The animation to switch to */
    te_ws2812_animations    mAnimation;

    /*! The transition to use */
    te_ws2812_transitions   mTransition;

    /*! The animation parameters */
    tu_ws2812_anim_param    mAnimParam;

    /*! The transition parameters */
    tu_ws2812_trans_param   mTransParam;

} ts_ws2812_anim_ctrl_cmd;


/*! Defines the animation control object */
typedef struct {

    /*! The notification message queue */
    QueueHandle_t               mMsgQueue;

    /*! for timing */
    TimeOut_t                   mTimeout;

    /*! Animation state */
    te_ws2812_animation_state   mState;

    /*! Received command */
    ts_ws2812_anim_ctrl_cmd     mLastCommand;

    /*! Transition */
    te_ws2812_transitions       mTransition;

    /*! Transition parameters */
    tu_ws2812_trans_param       mTransParam;

} ts_ws2812_anim_ctrl;


// ------------------- local variables --------------


/*! Animation object */
static tu_ws2812_anim sAnimation;


/*! Animation control object */
static ts_ws2812_anim_ctrl sAnimationControl;


/*! Animation initialization functions */
static const f_ws2812_anim_init sAnimationInitFuncs[] = {

    [WS2812_ANIMATION_CONSTANT_COLOR] = ws2812_anim_const_color_init,
};


/*! Transition object */
static tu_ws2812_trans sTransition;


/*! Transition initialization functions */
static const f_ws2812_trans_init sTransitionInitFuncs[] = {

    [WS2812_TRANSITION_FADE] = ws2812_trans_fade_init,
};


// ------------------- functions --------------------


void ws2812_animation_init(void) {

    sAnimationControl.mMsgQueue    = xQueueCreate(4, sizeof(ts_ws2812_anim_ctrl_cmd));
    sAnimationControl.mState       = WS2812_ANIM_STATE_MAIN;

    if(!sAnimationControl.mMsgQueue) {
        dbg_err("%s(%d): Failed initializing mMsgQueue\r\n", __FILE__, __LINE__);
    }

    /* fade transition */
    sAnimationControl.mTransition = WS2812_TRANSITION_FADE;
    sAnimationControl.mTransParam.mFade.mDuration = 25;

    /* black color */
    sAnimationControl.mLastCommand.mAnimation = WS2812_ANIMATION_CONSTANT_COLOR;
    sAnimationControl.mLastCommand.mAnimParam.mConstantColor.mColor.R = 0;
    sAnimationControl.mLastCommand.mAnimParam.mConstantColor.mColor.G = 0;
    sAnimationControl.mLastCommand.mAnimParam.mConstantColor.mColor.B = 0;

    /* start with a clean transition */
    sAnimationControl.mLastCommand.mTransition = sAnimationControl.mTransition;
    sAnimationControl.mLastCommand.mTransParam.mFade.mDuration = sAnimationControl.mTransParam.mFade.mDuration;
}


TickType_t ws2812_animation_update(void) {

    if(sAnimation.mBase.mf_update) {

        return sAnimation.mBase.mf_update(&sAnimation);
    }

    return portMAX_DELAY;
}

static TickType_t ws2812_transition_update(void) {

    if(sTransition.mBase.mf_update) {

        return sTransition.mBase.mf_update(&sTransition);
    }

    return portMAX_DELAY;
}

void ws2812_animation_switch(void) {

    sAnimationInitFuncs[sAnimationControl.mLastCommand.mAnimation](&sAnimation, &sAnimationControl.mLastCommand.mAnimParam);
}

void ws2812_transition_done(void) {

    sAnimationControl.mState = WS2812_ANIM_STATE_MAIN;
}


void ws2812_animation_main(void) {

    TickType_t lDelay;

    /* get the time base */
    vTaskSetTimeOutState(&sAnimationControl.mTimeout);

    switch(sAnimationControl.mState) {

        case WS2812_ANIM_STATE_TRANSIT: {

                lDelay = ws2812_transition_update();
            }
            break;
        case WS2812_ANIM_STATE_MAIN:
        default: {

                /* run animation */
                lDelay = ws2812_animation_update();
            }
            break;
    }
    ws2812_updateLED();

    /* Adapt the timeout according to the animation's request */
    if(xTaskCheckForTimeOut(&sAnimationControl.mTimeout, &lDelay)) {

        lDelay = 0; // timed out
    } // xTaskCheckForTimeOut adapts lDelay to fit to the timeout

    /* check if there are new transition commands */
    if(xQueueReceive(sAnimationControl.mMsgQueue, &sAnimationControl.mLastCommand, lDelay)) {

        /* if we received a command, go to transition state */
        sAnimationControl.mState = WS2812_ANIM_STATE_TRANSIT;
        sTransitionInitFuncs[sAnimationControl.mLastCommand.mTransition](&sTransition, &sAnimationControl.mLastCommand.mTransParam);
    }
}


void ws2812_anim_const_color(uint8_t inRed, uint8_t inGreen, uint8_t inBlue) {

    ts_ws2812_anim_ctrl_cmd lCommand;

    lCommand.mAnimation = WS2812_ANIMATION_CONSTANT_COLOR;
    lCommand.mAnimParam.mConstantColor.mColor.R = inRed;
    lCommand.mAnimParam.mConstantColor.mColor.G = inGreen;
    lCommand.mAnimParam.mConstantColor.mColor.B = inBlue;

    /*! Use configured transition */
    lCommand.mTransition = sAnimationControl.mTransition;
    memcpy(&lCommand.mTransParam, &sAnimationControl.mTransParam, sizeof(tu_ws2812_trans_param));

    xQueueSend(sAnimationControl.mMsgQueue, &lCommand, portMAX_DELAY );
}



/* eof */
