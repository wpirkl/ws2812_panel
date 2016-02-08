#ifndef WS2812_TRANSITION_FADE_H_
#define WS2812_TRANSITION_FADE_H_

#include "FreeRTOS.h"

#include "ws2812_transition_base.h"


/*! Base transition object */
typedef struct {

    /*! Base object */
    ts_ws2812_trans_base mBase;

    /*! Number of ticks to fade over */
    TickType_t mDuration;

    /*! Number of ticks elapsed */
    TickType_t mElapsed;

} ts_ws2812_trans_fade;


typedef struct {

    /*! Number of ticks to fade over */
    TickType_t mDuration;

} ts_ws2812_trans_fade_param;



/*! Transition init function */
void ws2812_trans_fade_init(tu_ws2812_trans * pThis, tu_ws2812_trans_param * pParam);



#endif /* WS2812_TRANSITION_FADE_H_ */

/* eof */
