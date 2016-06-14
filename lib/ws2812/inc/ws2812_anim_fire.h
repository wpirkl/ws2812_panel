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

} ts_ws2812_anim_fire;


typedef struct {

    /*! color palette */
    te_color_palettes       mPalette;

} ts_ws2812_anim_param_fire;




/*! Initialize fire animation

    \param[in]  inColor     The color to show
*/
void ws2812_anim_fire_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam);






#endif /* WS2812_ANIM_FIRE_H_ */

/* eof */
