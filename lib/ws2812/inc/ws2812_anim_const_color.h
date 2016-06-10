#ifndef WS2812_ANIM_CONST_COLOR_H_
#define WS2812_ANIM_CONST_COLOR_H_

#include <stdint.h>
#include <stdbool.h>

#include "color.h"             // for color

#include "ws2812_anim_base.h"

typedef struct {

    /*! base object */
    ts_ws2812_anim_base     mBase;

    /*! constant color */
    color                   mColor;

} ts_ws2812_anim_const_color;


typedef struct {

    /*! constant color */
    color                   mColor;

} ts_ws2812_anim_param_const_color;




/*! Initialize constant color animation

    \param[in]  inColor     The color to show
*/
void ws2812_anim_const_color_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam);






#endif /* WS2812_ANIM_CONST_COLOR_H_ */

/* eof */
