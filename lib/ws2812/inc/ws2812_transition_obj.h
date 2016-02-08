#ifndef WS2812_TRANSITION_OBJ_H_
#define WS2812_TRANSITION_OBJ_H_

#include "ws2812_transition_base.h"
#include "ws2812_transition_fade.h"



/*! Transition object definition */
union u_ws2812_trans {

    /*! Base object */
    ts_ws2812_trans_base    mBase;

    /*! Fade object */
    ts_ws2812_trans_fade    mFade;
};


union u_ws2812_trans_param {

    /*! Fade parameters */
    ts_ws2812_trans_fade_param  mFade;
};






#endif /* WS2812_TRANSITION_OBJ_H_ */

/* eof */
