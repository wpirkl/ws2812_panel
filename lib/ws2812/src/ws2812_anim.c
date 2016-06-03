
#include <stdbool.h>
#include <stdio.h>      // for dbg
#include <string.h>     // for memcpy

#include "ws2812.h"   // for color_f and WS2812_NR_ROWS, WS2812_NR_COLUMNS

#include "ws2812_anim_obj.h"
#include "ws2812_modifier_obj.h"
#include "ws2812_transition_obj.h"

#include "ws2812_anim.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

// animations
#include "ws2812_anim_const_color.h"
#include "ws2812_anim_gradient.h"

// transitions
#include "ws2812_transition_fade.h"


// ------------------- debug ------------------------

#define dbg_on(x...)      printf(x)
#define dbg_off(x...)

#define dbg dbg_off


#define dbg_err_on(x...)      printf(x)
#define dbg_err_off(x...)

#define dbg_err dbg_err_on

// ------------------- defines ----------------------

/*! Defines the animation frequency in Hz */
#define WS2812_ANIMATION_FREQ       (100)

/*! Defines the ms per animation tick */
#define WS2812_ANIMATION_DELAY_MS   (1000 / WS2812_ANIMATION_FREQ)



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

    /*! Gradient animation */
    WS2812_ANIMATION_GRADIENT,

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

    /*! next animation */
    size_t                      mCurrentAnimation;

    /*! Received command */
    ts_ws2812_anim_ctrl_cmd     mLastCommand;

    /*! Animation object */
    tu_ws2812_anim              mAnimation[2];

    /*! Transition object */
    tu_ws2812_trans             mTransition;

} ts_ws2812_anim_ctrl;


// ------------------- local variables --------------

/*! Animation control object */
static ts_ws2812_anim_ctrl sAnimationControl;


/*! Animation initialization functions */
static const f_ws2812_anim_init sAnimationInitFuncs[] = {

    [WS2812_ANIMATION_CONSTANT_COLOR] = ws2812_anim_const_color_init,
    [WS2812_ANIMATION_GRADIENT]       = ws2812_anim_gradient_init,
};


/*! Transition initialization functions */
static const f_ws2812_trans_init sTransitionInitFuncs[] = {

    [WS2812_TRANSITION_FADE] = ws2812_trans_fade_init,
};


// ------------------- functions --------------------


void ws2812_animation_init(void) {

    sAnimationControl.mMsgQueue         = xQueueCreate(4, sizeof(ts_ws2812_anim_ctrl_cmd));
    sAnimationControl.mState            = WS2812_ANIM_STATE_MAIN;
    sAnimationControl.mCurrentAnimation = 0;

    if(!sAnimationControl.mMsgQueue) {
        dbg_err("%s(%d): Failed initializing mMsgQueue\r\n", __FILE__, __LINE__);
    }

    /* black color */
    sAnimationControl.mLastCommand.mAnimation = WS2812_ANIMATION_CONSTANT_COLOR;
    sAnimationControl.mLastCommand.mAnimParam.mConstantColor.mColor.R = 0;
    sAnimationControl.mLastCommand.mAnimParam.mConstantColor.mColor.G = 0;
    sAnimationControl.mLastCommand.mAnimParam.mConstantColor.mColor.B = 0;

    /* clean init of modifier */
    sAnimationControl.mAnimation[sAnimationControl.mCurrentAnimation].mBase.mModifier = NULL;

    /* initialize animation */
    sAnimationInitFuncs[sAnimationControl.mLastCommand.mAnimation](&sAnimationControl.mAnimation[sAnimationControl.mCurrentAnimation], &sAnimationControl.mLastCommand.mAnimParam);
}


static void ws2812_animation_update(tu_ws2812_anim * pThis) {

    color * lPanel;
    tu_ws2812_modifier * lModifier;

    pThis->mBase.mfUpdate(pThis);

    /* iterate over all modifiers */
    for(lPanel = pThis->mBase.mPanel,     lModifier = pThis->mBase.mModifier;
        lModifier != NULL;
        lPanel = lModifier->mBase.mPanel, lModifier = lModifier->mBase.mModifier) {

        /* run current modifier */
        lModifier->mBase.mfUpdate(lModifier, lPanel);
    }
}


static color * ws2812_animation_get_modifier_panel(tu_ws2812_modifier * pThis) {

    tu_ws2812_modifier * lModifier;

    /* find last modifier */
    for(lModifier = pThis; lModifier->mBase.mModifier != NULL; lModifier = lModifier->mBase.mModifier);

    /* return it's panel */
    return lModifier->mBase.mPanel;
}


static color * ws2812_animation_get_panel(tu_ws2812_anim * pThis) {

    if(pThis->mBase.mModifier != NULL) {

        return ws2812_animation_get_modifier_panel(pThis->mBase.mModifier);
    } else {

        return pThis->mBase.mPanel;
    }
}


void ws2812_transition_done(void) {

    /* switch animation */
    sAnimationControl.mCurrentAnimation = (sAnimationControl.mCurrentAnimation + 1) & 1;

    /* switch mode */
    sAnimationControl.mState = WS2812_ANIM_STATE_MAIN;
}


void ws2812_animation_main(void) {

    /* run at 100 Hz */
    TickType_t lDelay = configTICK_RATE_HZ / WS2812_ANIMATION_FREQ;

    vTaskSetTimeOutState(&sAnimationControl.mTimeout);

    switch(sAnimationControl.mState) {

        case WS2812_ANIM_STATE_TRANSIT: {

                /* run animation 1 */
                ws2812_animation_update(&sAnimationControl.mAnimation[sAnimationControl.mCurrentAnimation]);

                /* run animation 2 */
                ws2812_animation_update(&sAnimationControl.mAnimation[(sAnimationControl.mCurrentAnimation + 1) & 1]);

                sAnimationControl.mTransition.mBase.mfUpdate(&sAnimationControl.mTransition,
                                                             ws2812_animation_get_panel(&sAnimationControl.mAnimation[sAnimationControl.mCurrentAnimation]),
                                                             ws2812_animation_get_panel(&sAnimationControl.mAnimation[(sAnimationControl.mCurrentAnimation + 1) & 1]));

                /* update led from transition buffer */
                ws2812_updateLED(sAnimationControl.mTransition.mBase.mPanel);
            }
            break;
        case WS2812_ANIM_STATE_MAIN:
        default: {

                /* run animation */
                sAnimationControl.mAnimation[sAnimationControl.mCurrentAnimation].mBase.mfUpdate(&sAnimationControl.mAnimation[sAnimationControl.mCurrentAnimation]);

                /* update led from animation buffer */
                ws2812_updateLED(sAnimationControl.mAnimation[sAnimationControl.mCurrentAnimation].mBase.mPanel);
            }
            break;
    }

    /* Adapt the timeout according to the animation's request */
    if(xTaskCheckForTimeOut(&sAnimationControl.mTimeout, &lDelay)) {

        dbg_err("%s(%d): timed out!\r\n", __FILE__, __LINE__);

        lDelay = 0; // timed out
    } // xTaskCheckForTimeOut adapts lDelay to fit to the timeout

    if(sAnimationControl.mState == WS2812_ANIM_STATE_TRANSIT) {

        vTaskDelay(lDelay);

    } else {
        /* check if there are new transition commands */
        if(xQueueReceive(sAnimationControl.mMsgQueue, &sAnimationControl.mLastCommand, lDelay)) {

            /* if we received a command, go to transition state */
            sAnimationControl.mState = WS2812_ANIM_STATE_TRANSIT;
            sTransitionInitFuncs[sAnimationControl.mLastCommand.mTransition](&sAnimationControl.mTransition, &sAnimationControl.mLastCommand.mTransParam);

            /* init second animation */
            sAnimationInitFuncs[sAnimationControl.mLastCommand.mAnimation](&sAnimationControl.mAnimation[(sAnimationControl.mCurrentAnimation + 1) & 1], &sAnimationControl.mLastCommand.mAnimParam);

            /* clean init of modifier */
            sAnimationControl.mAnimation[(sAnimationControl.mCurrentAnimation + 1) & 1].mBase.mModifier = NULL;
        }
    }
}


void ws2812_anim_const_color(uint8_t inRed, uint8_t inGreen, uint8_t inBlue) {

    ts_ws2812_anim_ctrl_cmd lCommand;

    lCommand.mAnimation = WS2812_ANIMATION_CONSTANT_COLOR;
    lCommand.mAnimParam.mConstantColor.mColor.R = inRed;
    lCommand.mAnimParam.mConstantColor.mColor.G = inGreen;
    lCommand.mAnimParam.mConstantColor.mColor.B = inBlue;

    /*! todo: use configured transition */
    lCommand.mTransition = WS2812_TRANSITION_FADE;
    lCommand.mTransParam.mFade.mDuration = 1000 / WS2812_ANIMATION_DELAY_MS;    // 1000ms @ 100 Hz

    xQueueSend(sAnimationControl.mMsgQueue, &lCommand, portMAX_DELAY );
}


void ws2812_anim_gradient(uint8_t inFirstRed, uint8_t inFirstGreen, uint8_t inFirstBlue,
                          uint8_t inSecondRed, uint8_t inSecondGreen, uint8_t inSecondBlue,
                          int16_t inAngle) {

    ts_ws2812_anim_ctrl_cmd lCommand;

    lCommand.mAnimation = WS2812_ANIMATION_GRADIENT;
    lCommand.mAnimParam.mGradient.mFirstColor.R = inFirstRed;
    lCommand.mAnimParam.mGradient.mFirstColor.G = inFirstGreen;
    lCommand.mAnimParam.mGradient.mFirstColor.B = inFirstBlue;

    lCommand.mAnimParam.mGradient.mSecondColor.R = inSecondRed;
    lCommand.mAnimParam.mGradient.mSecondColor.G = inSecondGreen;
    lCommand.mAnimParam.mGradient.mSecondColor.B = inSecondBlue;

    lCommand.mAnimParam.mGradient.mAngle = inAngle;

    /*! todo: use configured transition */
    lCommand.mTransition = WS2812_TRANSITION_FADE;
    lCommand.mTransParam.mFade.mDuration = 1000 / WS2812_ANIMATION_DELAY_MS;    // 1000ms @ 100 Hz

    xQueueSend(sAnimationControl.mMsgQueue, &lCommand, portMAX_DELAY );
}



/* eof */
