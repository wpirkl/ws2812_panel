
#include "ws2812.h"

#include "ws2812_anim_obj.h"
#include "ws2812_anim_fire.h"


static void ws2812_anim_fire_update(tu_ws2812_anim * pThis) {

}


void ws2812_anim_fire_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam) {

    pThis->mBase.mfUpdate   = ws2812_anim_fire_update;
    pThis->mFire.mPalette   = pParam->mFire.mPalette;
}


/* eof */
