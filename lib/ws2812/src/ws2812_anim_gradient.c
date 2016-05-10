
#include "ws2812.h"

#include "ws2812_anim_obj.h"
#include "ws2812_anim_const_color.h"


static void ws2812_anim_gradient_update(tu_ws2812_anim * pThis) {

}


void ws2812_anim_gradient_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam) {

    pThis->mBase.mfUpdate         = ws2812_anim_gradient_update;
    pThis->mGradient.mFirstColor  = pParam->mGradient.mFirstColor;
    pThis->mGradient.mSecondColor = pParam->mGradient.mSecondColor;
    pThis->mGradient.mAngle       = pParam->mGradient.mAngle;
}


/* eof */
