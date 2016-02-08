#ifndef WS2812_ANIM_H_
#define WS2812_ANIM_H_

#include <stdint.h>     // uint8_t


/*! Initialize Animation */
void ws2812_animation_init(void);


/*! This function executes the led animation control

    It should be called from an infinite loop
*/
void ws2812_animation_main(void);


/*! This function will switch to constant color mode

    \param[in]  inRed   Red color from 0 to 255
    \param[in]  inGreen Green color from 0 to 255
    \param[in]  inBlue  Blue part from 0 to 255
*/
void ws2812_anim_const_color(uint8_t inRed, uint8_t inGreen, uint8_t inBlue);



#endif /* WS2812_ANIM_H_ */

/* eof */
