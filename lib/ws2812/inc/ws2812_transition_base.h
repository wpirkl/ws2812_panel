#ifndef WS2812_TRANSITION_BASE_H_
#define WS2812_TRANSITION_BASE_H_

#include "FreeRTOS.h"   // for TickType_t

#include "ws2812.h"     // for color_f
#include "ws2812_p.h"     // for color_f

union u_ws2812_trans;
typedef union u_ws2812_trans tu_ws2812_trans;

union u_ws2812_trans_param;
typedef union u_ws2812_trans_param tu_ws2812_trans_param;

/*! Base transition object */
typedef struct {

    /*! update function */
    TickType_t   (* mf_update)(tu_ws2812_trans * pThis);

    /*! Buffer for tansitions */
    color_f         mPanel[NR_ROWS * NR_COLUMNS];

} ts_ws2812_trans_base;


/*! Transition init function */
typedef void (*f_ws2812_trans_init)(tu_ws2812_trans * pThis, tu_ws2812_trans_param * pParam);



#endif /* WS2812_TRANSITION_BASE_H_ */

/* eof */
