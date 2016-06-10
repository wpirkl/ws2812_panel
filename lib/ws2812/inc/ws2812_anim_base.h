#ifndef WS2812_ANIM_BASE_H_
#define WS2812_ANIM_BASE_H_


#include "color.h"                  // for color / color_f
#include "ws2812_modifier_obj.h"    // for tu_ws2812_modifier

union u_ws2812_anim;
typedef union u_ws2812_anim tu_ws2812_anim;

union u_ws2812_anim_param;
typedef union u_ws2812_anim_param tu_ws2812_anim_param;

struct s_ws2812_anim_base;
typedef struct s_ws2812_anim_base ts_ws2812_anim_base;


struct s_ws2812_anim_base {

    /*! Process function */
    void     (* mfUpdate)(tu_ws2812_anim * pThis);

    /*! modifiers */
    tu_ws2812_modifier * mModifier;

    /*! panel to paint on */
    color       mPanel[WS2812_NR_ROWS * WS2812_NR_COLUMNS];
};


typedef void (*f_ws2812_anim_init)(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam);






#endif /* WS2812_ANIM_BASE_H_ */

/* eof */
