#ifndef WS2812_ANIM_GRADIENT_H_
#define WS2812_ANIM_GRADIENT_H_

#include <stdint.h>
#include <stdbool.h>

#include "color.h"             // for color

#include "ws2812_anim_base.h"

typedef struct {

    /*! base object */
    ts_ws2812_anim_base     mBase;

    /*! first color */
    color                   mFirstColor;

    /*! second color */
    color                   mSecondColor;

    /*! angle */
    int16_t                 mAngle;

} ts_ws2812_anim_gradient;


typedef struct {

    /*! first color */
    color                   mFirstColor;

    /*! second color */
    color                   mSecondColor;

    /*! angle */
    int16_t                 mAngle;

} ts_ws2812_anim_param_gradient;




/*! Initialize gradient color animation

    \param[in]  inColor     The color to show
*/
void ws2812_anim_gradient_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam);



#endif /* WS2812_ANIM_GRADIENT_H_ */

/* eof */
