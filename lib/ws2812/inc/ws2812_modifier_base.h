#ifndef WS2812_MODIFIER_BASE_H_
#define WS2812_MODIFIER_BASE_H_


#include "color.h"     // for color / color_f

union u_ws2812_modifier;
typedef union u_ws2812_modifier tu_ws2812_modifier;

struct s_ws2812_modifier_base;
typedef struct s_ws2812_modifier_base ts_ws2812_modifier_base;

union u_ws2812_modifier_param;
typedef union u_ws2812_modifier_param tu_ws2812_modifier_param;

struct s_ws2812_modifier_base {

    /*! Process function */
    void      (* mfUpdate)(tu_ws2812_modifier * pThis, color * pBasePanel);

    /*! next modifier */
    tu_ws2812_modifier * mModifier;

    /*! panel to paint on */
    color       mPanel[WS2812_NR_ROWS * WS2812_NR_COLUMNS];
};


typedef void (*f_ws2812_modifier_init)(tu_ws2812_modifier * pThis, tu_ws2812_modifier_param * pParam);


#endif /* WS2812_MODIFIER_BASE_H_ */

/* eof */
