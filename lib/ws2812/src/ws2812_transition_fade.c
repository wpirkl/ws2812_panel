#include <string.h> // for memcpy

#include "FreeRTOS.h"

#include "ws2812.h"             // for color_f

#include "ws2812_anim_p.h"      // for ws2812_transition_done
#include "ws2812_anim_obj.h"    // for tu_ws2812_anim

#include "ws2812_transition_obj.h"
#include "ws2812_transition_fade.h"


#define REGULARIZE_COLOR(x) ((uint8_t)((x > 255.0f)? 255 : ((x < 0.0f)? 0: x + 0.5f)))


static void ws2812_trans_fade_update(tu_ws2812_trans * pThis, color * pAnimationOne, color * pAnimationTwo) {

    size_t lColumnCount;
    size_t lRowCount;

    float lPercentage;

    lPercentage = (float)(++pThis->mFade.mElapsed) / (float)pThis->mFade.mDuration;

    /* update all colors */
    for(lColumnCount = 0; lColumnCount < WS2812_NR_COLUMNS; lColumnCount++) {
        for(lRowCount = 0; lRowCount < WS2812_NR_ROWS; lRowCount++) {

            size_t lLedIndex = lRowCount * WS2812_NR_COLUMNS + lColumnCount;

            float r = ((1.0f - lPercentage) * pAnimationOne[lLedIndex].R + (lPercentage) * pAnimationTwo[lLedIndex].R);
            float g = ((1.0f - lPercentage) * pAnimationOne[lLedIndex].G + (lPercentage) * pAnimationTwo[lLedIndex].G);
            float b = ((1.0f - lPercentage) * pAnimationOne[lLedIndex].B + (lPercentage) * pAnimationTwo[lLedIndex].B);

            pThis->mBase.mPanel[lLedIndex].R = REGULARIZE_COLOR(r);
            pThis->mBase.mPanel[lLedIndex].G = REGULARIZE_COLOR(g);
            pThis->mBase.mPanel[lLedIndex].B = REGULARIZE_COLOR(b);
        }
    }

    if(pThis->mFade.mElapsed >= pThis->mFade.mDuration) {
        ws2812_transition_done();
    }
}




void ws2812_trans_fade_init(tu_ws2812_trans * pThis, tu_ws2812_trans_param * pParam) {

    pThis->mBase.mfUpdate = ws2812_trans_fade_update;
    pThis->mFade.mDuration = pParam->mFade.mDuration;
    pThis->mFade.mElapsed  = 0;

    if(pThis->mFade.mDuration == 0) {
        pThis->mFade.mDuration = 1;
    }
}

/* eof */
