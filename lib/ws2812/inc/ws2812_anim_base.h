#ifndef WS2812_ANIM_BASE_H_
#define WS2812_ANIM_BASE_H_


#include "FreeRTOS.h"   // for TickType_t


union u_ws2812_anim;
typedef union u_ws2812_anim tu_ws2812_anim;

union u_ws2812_anim_param;
typedef union u_ws2812_anim_param tu_ws2812_anim_param;


typedef struct {

    /*! Process function */
    TickType_t  (* mf_update)(tu_ws2812_anim * pThis);

} ts_ws2812_anim_base;


typedef void (*f_ws2812_anim_init)(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam);






#endif /* WS2812_ANIM_BASE_H_ */

/* eof */
