
#include <stdbool.h>
#include <stdio.h>      // for dbg
#include <string.h>     // for memcpy

#include "ws2812.h"     // for color and NR_ROWS, NR_COLUMNS

#include "ws2812_anim_obj.h"
#include "ws2812_anim.h"

#include "FreeRTOS.h"
#include "queue.h"

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

    /*! Main state */
    WS2812_ANIM_STATE_MAIN,

    /*! Transition init */
    WS2812_ANIM_STATE_TRANSIT_INIT,

    /*! Transition state */
    WS2812_ANIM_STATE_TRANSIT,

} te_ws2812_animation_state;

/*! Enumerates the animations */
typedef enum {

    /*! Constant color animation */
    WS2812_ANIMATION_CONSTANT_COLOR = 0,

} te_ws2812_animations;


/*! Defines a message */
typedef struct {

    /*! The animation to switch to */
    te_ws2812_animations    mAnimation;

    /*! The number of frames for the transition MUST be > 0 */
    size_t                  mTransitionFrameCnt;

    /*! The animation parameters */
    tu_ws2812_anim_param    mParam;

} ts_ws2812_anim_ctrl_cmd;


/*! Defines the animation control object */
typedef struct {

    /*! The notification message queue */
    QueueHandle_t               mMsgQueue;

    /*! Animation state */
    te_ws2812_animation_state   mState;

    /*! Received command */
    ts_ws2812_anim_ctrl_cmd     mLastCommand;

    /*! Fade over buffer */
    color                       mPanel[NR_ROWS * NR_COLUMNS];

} ts_ws2812_anim_ctrl;


// ------------------- local variables --------------


/*! Animation object */
static tu_ws2812_anim sAnimation;


/*! Animation control object */
static ts_ws2812_anim_ctrl  sAnimationControl;


/*! Initialization functions */
static const f_ws2812_anim_init sAnimationInitFuncs[] = {

    [WS2812_ANIMATION_CONSTANT_COLOR] = ws2812_anim_const_color_init,
};


// ------------------- functions --------------------


void ws2812_animation_init(void) {

    tu_ws2812_anim_param lParam;

    sAnimationControl.mMsgQueue    = xQueueCreate(4, sizeof(ts_ws2812_anim_ctrl_cmd));
    sAnimationControl.mState       = WS2812_ANIM_STATE_MAIN;

    if(!sAnimationControl.mMsgQueue) {
        dbg_err("%s(%d): Failed initializing mMsgQueue\r\n", __FILE__, __LINE__);
    }

    lParam.mConstantColor.mColor.R = 0;
    lParam.mConstantColor.mColor.G = 0;
    lParam.mConstantColor.mColor.B = 0;

    ws2812_anim_const_color_init(&sAnimation, &lParam);
}


static bool ws2812_animation_update(void) {

    if(sAnimation.mBase.mf_update) {

        return sAnimation.mBase.mf_update(&sAnimation);
    }

    return false;
}


void ws2812_animation_main(void) {

    switch(sAnimationControl.mState) {

        case WS2812_ANIM_STATE_TRANSIT_INIT: {

                color * lLedBuffer;
                size_t lColumnCount;
                size_t lRowCount;

                /* first of all, take a snapshot of the current state */
                ws2812_getLED_Buffer(&lLedBuffer);
                memcpy(sAnimationControl.mPanel, lLedBuffer, sizeof(sAnimationControl.mPanel));

                /* now initialize the new state and let it update the buffer, once */
                sAnimationInitFuncs[sAnimationControl.mLastCommand.mAnimation](&sAnimation, &sAnimationControl.mLastCommand.mParam);
                ws2812_animation_update();

                /* now calculate a transition matrix */
                for(lColumnCount = 0; lColumnCount < NR_COLUMNS; lColumnCount++) {
                    for(lRowCount = 0; lRowCount < NR_ROWS; lRowCount++) {

                        size_t lLedIndex = lRowCount * NR_COLUMNS + lColumnCount;
                        color lBackup;

                        /* backup old color */
                        lBackup.R = sAnimationControl.mPanel[lLedIndex].R;
                        lBackup.G = sAnimationControl.mPanel[lLedIndex].G;
                        lBackup.B = sAnimationControl.mPanel[lLedIndex].B;

                        /* calculate transition matrix */
                        sAnimationControl.mPanel[lLedIndex].R = (lLedBuffer[lLedIndex].R - sAnimationControl.mPanel[lLedIndex].R) / sAnimationControl.mLastCommand.mTransitionFrameCnt;
                        sAnimationControl.mPanel[lLedIndex].G = (lLedBuffer[lLedIndex].G - sAnimationControl.mPanel[lLedIndex].G) / sAnimationControl.mLastCommand.mTransitionFrameCnt;
                        sAnimationControl.mPanel[lLedIndex].B = (lLedBuffer[lLedIndex].B - sAnimationControl.mPanel[lLedIndex].B) / sAnimationControl.mLastCommand.mTransitionFrameCnt;

                        /* restore old color */
                        lLedBuffer[lLedIndex].R = lBackup.R;
                        lLedBuffer[lLedIndex].G = lBackup.G;
                        lLedBuffer[lLedIndex].B = lBackup.B;
                    }
                }

                /* now fall through to apply the first transition step */

                sAnimationControl.mState = WS2812_ANIM_STATE_TRANSIT;
            }
            /* fall through is wanted */
        case WS2812_ANIM_STATE_TRANSIT: {

                color * lLedBuffer;

                size_t lColumnCount;
                size_t lRowCount;

                /* count number of frames */
                if(--sAnimationControl.mLastCommand.mTransitionFrameCnt > 0) {

                    /* get the led buffer to draw directly to the leds */
                    ws2812_getLED_Buffer(&lLedBuffer);

                    /* update all colors */
                    for(lColumnCount = 0; lColumnCount < NR_COLUMNS; lColumnCount++) {
                        for(lRowCount = 0; lRowCount < NR_ROWS; lRowCount++) {

                            size_t lLedIndex = lRowCount * NR_COLUMNS + lColumnCount;

                            lLedBuffer[lLedIndex].R += sAnimationControl.mPanel[lLedIndex].R;
                            lLedBuffer[lLedIndex].G += sAnimationControl.mPanel[lLedIndex].G;
                            lLedBuffer[lLedIndex].B += sAnimationControl.mPanel[lLedIndex].B;
                        }
                    }

                    /* push it out to the leds */
                    ws2812_updateLED();

                    /* continue in this state */
                    break;
                } else {

                    /* go to main state */
                    sAnimationControl.mState = WS2812_ANIM_STATE_MAIN;
                }
            }
            /* conditionnal fall-through wanted */
        case WS2812_ANIM_STATE_MAIN:
        default: {

                bool lRetVal;
                TickType_t lDelay;

                /* run animation */
                lRetVal = ws2812_animation_update();
                ws2812_updateLED();

                if(lRetVal) {       /* Animation want's to be re-executed */
                    lDelay = 0;
                } else {            /* Go to sleep until new command received */
                    lDelay = portMAX_DELAY;
                }

                if(xQueueReceive(sAnimationControl.mMsgQueue, &sAnimationControl.mLastCommand, lDelay)) {

                    /* if we received a command, go to transition state */
                    sAnimationControl.mState = WS2812_ANIM_STATE_TRANSIT_INIT;
                }
            }
            break;
    }
}


void ws2812_anim_const_color(uint8_t inRed, uint8_t inGreen, uint8_t inBlue) {

    ts_ws2812_anim_ctrl_cmd lCommand;

    lCommand.mAnimation = WS2812_ANIMATION_CONSTANT_COLOR;
    lCommand.mTransitionFrameCnt = 25;
    lCommand.mParam.mConstantColor.mColor.R = inRed;
    lCommand.mParam.mConstantColor.mColor.G = inGreen;
    lCommand.mParam.mConstantColor.mColor.B = inBlue;

    xQueueSend(sAnimationControl.mMsgQueue, &lCommand, portMAX_DELAY );
}



/* eof */
