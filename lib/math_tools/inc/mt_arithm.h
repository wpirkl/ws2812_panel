#ifndef MT_ARITHM_H_
#define MT_ARITHM_H_

#include <stdint.h>



/*! Substract B from A without underrun

    \retval (inA - inB < 0)? 0 : (inA - inB)
*/
static inline uint8_t sub8_f(uint8_t inA, uint8_t inB) {

    if(inA < inB) {
        return 0;
    } else {
        return inA - inB;
    }
}


/*! Add B to A without overrun

    \retval (inA + inB > 255)? 255 : (inA + inB)
*/
static inline uint8_t add8_c(uint8_t inA, uint8_t inB) {

    if(255 - inA < inB) {
        return 255;
    } else {
        return inA + inB;
    }
}


/*! Add B to A without overrun of a certain value

    \retval (inA + inB > inLimit)? inLimit : (inA + inB)
*/
static inline uint8_t add8_cl(uint8_t inA, uint8_t inB, uint8_t inLimit) {

    if(inLimit - inA < inB) {

        return inLimit;
    } else {
        return inA + inB;
    }
}







#endif /* MT_ARITHM_H_ */

/* eof */
