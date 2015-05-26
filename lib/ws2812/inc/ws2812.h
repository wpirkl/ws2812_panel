#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include <stdio.h>

#include "stm32f4xx.h"

// ----------------------------- definitions -----------------------------
#define MAX(a,b)        ((a > b)? a : b)

#define NR_ROWS         (5)
#define NR_COLUMNS_0    (172)
#define NR_COLUMNS_1    (172)
#define NR_COLUMNS_2    (172)
#define NR_COLUMNS_3    (172)
#define NR_COLUMNS_4    (172)
#define NR_COLUMNS_MAX  (MAX(MAX(MAX(NR_COLUMNS_0,NR_COLUMNS_1),MAX(NR_COLUMNS_2,NR_COLUMNS_3)),NR_COLUMNS_4))

#define SIZE_OF_LED     (24)      // 3(RGB) * 8 Bit

#define WS2812_TIM_FREQ       ((168000000 / 4) * 2)
#define WS2812_OUT_FREQ       (800000)

// timer values to generate a "one" or a "zero" according to ws2812 datasheet
#define WS2812_PWM_PERIOD       ((WS2812_TIM_FREQ / WS2812_OUT_FREQ))
#define WS2812_PWM_ZERO         (29)  // 0.5 탎 of 2.5탎 is high => 1/5 of the period
#define WS2812_PWM_ONE          (58) // 2탎 of 2.5탎 is high -> 4/5 of the period


// number of timer cycles (~2.5탎) for the reset pulse
#define WS2812_RESET_LEN     20 //20*2.5탎 = 50탎


//------------------------------ structs ------------------------------
typedef struct{
    uint8_t R;
    uint8_t G;
    uint8_t B;
} color;

// ----------------------------- functions -----------------------------
void ws2812_init(void);
void updateLED(void);

// ----------------------------- graphics -----------------------------
void setLED(size_t inRow, size_t inColumn, uint8_t r, uint8_t g, uint8_t b);
void setLED_Color(size_t inRow, size_t inColumn, color *c);
void setAllLED(uint8_t r, uint8_t g, uint8_t b);
void setAllLED_Color(color *c);

#endif

/* eof */
