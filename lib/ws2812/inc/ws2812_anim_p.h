#ifndef WS2812_ANIM_P_H_
#define WS2812_ANIM_P_H_


#include "FreeRTOS.h"   // for TickType_t


/*! Switch animation

*/
void ws2812_animation_switch(void);


/*! Execute one tick of animation

    \return the number of ticks to delay before doing a new refresh
*/
TickType_t ws2812_animation_update(void);


/*! Transition done

*/
void ws2812_transition_done(void);



#endif /* WS2812_ANIM_P_H_ */

/* eof */
