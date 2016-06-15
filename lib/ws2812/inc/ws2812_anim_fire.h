#ifndef WS2812_ANIM_FIRE_H_
#define WS2812_ANIM_FIRE_H_

#include <stdint.h>
#include <stdbool.h>

#include "color.h"              // for color
#include "color_palette.h"      // for color palettes

#include "ws2812_anim_base.h"

typedef struct {

    /*! base object */
    ts_ws2812_anim_base     mBase;

    /*! color palette */
    te_color_palettes       mPalette;

    /*! heat map */
    uint8_t               * mHeat;

} ts_ws2812_anim_fire;


typedef struct {

    /*! color palette */
    te_color_palettes       mPalette;

} ts_ws2812_anim_param_fire;




/*! Initialize fire animation */
void ws2812_anim_fire_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam);


/*! Cleanup fire animation */
void ws2812_anim_fire_clean(tu_ws2812_anim * pThis);



#endif /* WS2812_ANIM_FIRE_H_ */

/* eof */
