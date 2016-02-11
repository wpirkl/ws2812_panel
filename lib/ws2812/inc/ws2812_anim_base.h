#ifndef WS2812_ANIM_BASE_H_
#define WS2812_ANIM_BASE_H_


#include "ws2812.h"     // for color_f

union u_ws2812_anim;
typedef union u_ws2812_anim tu_ws2812_anim;

union u_ws2812_anim_param;
typedef union u_ws2812_anim_param tu_ws2812_anim_param;


typedef struct {

    /*! Process function */
    void     (* mf_update)(tu_ws2812_anim * pThis);

    /*! panel to paint on */
    color_f     mPanel[WS2812_NR_ROWS * WS2812_NR_COLUMNS];

} ts_ws2812_anim_base;


typedef void (*f_ws2812_anim_init)(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam);






#endif /* WS2812_ANIM_BASE_H_ */

/* eof */
