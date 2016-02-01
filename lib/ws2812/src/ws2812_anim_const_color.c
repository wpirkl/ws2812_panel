
#include <stdbool.h>

#include "ws2812.h"

#include "ws2812_anim_obj.h"




static bool ws2812_anim_const_color_update(tu_ws2812_anim * pThis) {

    /* set the buffer once */
    ws2812_setLED_All(pThis->mConstantColor.mColor.R, pThis->mConstantColor.mColor.G, pThis->mConstantColor.mColor.B);

    /* update complete */
    pThis->mConstantColor.mReDraw = false;

    return pThis->mConstantColor.mReDraw;
}

void ws2812_anim_const_color_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam) {

    pThis->mBase.mf_update        = ws2812_anim_const_color_update;
    pThis->mConstantColor.mColor  = pParam->mConstantColor.mColor;
    pThis->mConstantColor.mReDraw = true;
}


/* eof */
