
#include "ws2812.h"

#include "ws2812_anim_obj.h"
#include "ws2812_anim_gradient.h"




static void ws2812_anim_const_color_update(tu_ws2812_anim * pThis) {

    /* set the buffer once */
    ws2812_setLED_All(pThis->mBase.mPanel, pThis->mConstantColor.mColor.R, pThis->mConstantColor.mColor.G, pThis->mConstantColor.mColor.B);
}

void ws2812_anim_const_color_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam) {

    pThis->mBase.mfUpdate        = ws2812_anim_const_color_update;
    pThis->mConstantColor.mColor  = pParam->mConstantColor.mColor;
}


/* eof */
