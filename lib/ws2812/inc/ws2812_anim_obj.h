#ifndef WS2812_ANIM_OBJ_H_
#define WS2812_ANIM_OBJ_H_

#include <stdint.h>
#include <stdbool.h>

#include "ws2812_anim_base.h"
#include "ws2812_anim_const_color.h"

/*! Animation object definition */
union u_ws2812_anim {

    /*! Base object */
    ts_ws2812_anim_base         mBase;

    /*! Constant Color */
    ts_ws2812_anim_const_color  mConstantColor;
};


/*! Animation object parameters definition */
union u_ws2812_anim_param {

    /*! Parameters for constant color mode */
    ts_ws2812_anim_param_const_color    mConstantColor;

};








#endif /* WS2812_ANIM_OBJ_H_ */

/* eof */
