#ifndef WS2812_TRANSITION_BASE_H_
#define WS2812_TRANSITION_BASE_H_

#include "ws2812.h"     // for color_f

#include "ws2812_anim_base.h"   // for tu_ws2812_anim

union u_ws2812_trans;
typedef union u_ws2812_trans tu_ws2812_trans;

union u_ws2812_trans_param;
typedef union u_ws2812_trans_param tu_ws2812_trans_param;

/*! Base transition object */
typedef struct {

    /*! update function */
    void        (* mfUpdate)(tu_ws2812_trans * pThis, tu_ws2812_anim * pAnimationOne, tu_ws2812_anim * pAnimationTwo);

    /*! Panel to paint on */
    color_f        mPanel[WS2812_NR_ROWS * WS2812_NR_COLUMNS];

} ts_ws2812_trans_base;


/*! Transition init function */
typedef void (*f_ws2812_trans_init)(tu_ws2812_trans * pThis, tu_ws2812_trans_param * pParam);



#endif /* WS2812_TRANSITION_BASE_H_ */

/* eof */
