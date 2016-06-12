
#include "ws2812.h"

#include "ws2812_anim_obj.h"
#include "ws2812_anim_const_color.h"


static void ws2812_anim_color_palette_update(tu_ws2812_anim * pThis) {

    size_t lCount;

    color lColor;

    float lIncrement = 256.0f / WS2812_NR_COLUMNS;

    for(lCount = 0; lCount < WS2812_NR_COLUMNS; lCount++) {

        color_palette_get_e(pThis->mPalette.mPalette, &lColor, (uint8_t)(lIncrement * lCount + 0.5f));

//        printf("%s(%d): got Color: %d, %d, %d\r\n", __func__, __LINE__, lColor.R, lColor.G, lColor.B);

        ws2812_setLED_Column(pThis->mBase.mPanel, lCount, lColor.R, lColor.G, lColor.B);
    }
}


void ws2812_anim_color_palette_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam) {

    pThis->mBase.mfUpdate       = ws2812_anim_color_palette_update;
    pThis->mPalette.mPalette    = pParam->mPalette.mPalette;
}


/* eof */
