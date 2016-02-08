#include <string.h> // for memcpy

#include "FreeRTOS.h"

#include "ws2812.h"
#include "ws2812_p.h"

#include "ws2812_anim_p.h"

#include "ws2812_transition_obj.h"
#include "ws2812_transition_fade.h"



static TickType_t ws2812_trans_fade_update(tu_ws2812_trans * pThis) {

    color_f * lLedBuffer;

    size_t lColumnCount;
    size_t lRowCount;

    /* get the led buffer to draw directly to the leds */
    ws2812_getLED_Buffer(&lLedBuffer);

    /* update all colors */
    for(lColumnCount = 0; lColumnCount < NR_COLUMNS; lColumnCount++) {
        for(lRowCount = 0; lRowCount < NR_ROWS; lRowCount++) {

            size_t lLedIndex = lRowCount * NR_COLUMNS + lColumnCount;

            lLedBuffer[lLedIndex].R += pThis->mBase.mPanel[lLedIndex].R;
            lLedBuffer[lLedIndex].G += pThis->mBase.mPanel[lLedIndex].G;
            lLedBuffer[lLedIndex].B += pThis->mBase.mPanel[lLedIndex].B;
        }
    }

    if(++pThis->mFade.mElapsed >= pThis->mFade.mDuration) {
        ws2812_transition_done();
    }

    return 1;
}




void ws2812_trans_fade_init(tu_ws2812_trans * pThis, tu_ws2812_trans_param * pParam) {

    pThis->mBase.mf_update = ws2812_trans_fade_update;
    pThis->mFade.mDuration = pParam->mFade.mDuration;
    pThis->mFade.mElapsed  = 0;

    {
        color_f * lLedBuffer;
        size_t lColumnCount;
        size_t lRowCount;

        /* first of all, take a snapshot of the current state */
        ws2812_getLED_Buffer(&lLedBuffer);
        memcpy(pThis->mBase.mPanel, lLedBuffer, sizeof(pThis->mBase.mPanel));

        /* now initialize the current state and let it update the buffer, once */
        ws2812_animation_switch();
        ws2812_animation_update();

        /* now calculate the transition */
        for(lColumnCount = 0; lColumnCount < NR_COLUMNS; lColumnCount++) {
            for(lRowCount = 0; lRowCount < NR_ROWS; lRowCount++) {

                size_t lLedIndex = lRowCount * NR_COLUMNS + lColumnCount;
                color_f lBackup;

                /* backup old color */
                lBackup.R = pThis->mBase.mPanel[lLedIndex].R;
                lBackup.G = pThis->mBase.mPanel[lLedIndex].G;
                lBackup.B = pThis->mBase.mPanel[lLedIndex].B;

                /* calculate transition */
                pThis->mBase.mPanel[lLedIndex].R = (lLedBuffer[lLedIndex].R - pThis->mBase.mPanel[lLedIndex].R) / pThis->mFade.mDuration;
                pThis->mBase.mPanel[lLedIndex].G = (lLedBuffer[lLedIndex].G - pThis->mBase.mPanel[lLedIndex].G) / pThis->mFade.mDuration;
                pThis->mBase.mPanel[lLedIndex].B = (lLedBuffer[lLedIndex].B - pThis->mBase.mPanel[lLedIndex].B) / pThis->mFade.mDuration;

                /* restore old color */
                lLedBuffer[lLedIndex].R = lBackup.R;
                lLedBuffer[lLedIndex].G = lBackup.G;
                lLedBuffer[lLedIndex].B = lBackup.B;
            }
        }
    }
}

/* eof */
