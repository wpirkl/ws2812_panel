#ifndef WS2812_P_H
#define WS2812_P_H

#include <stdint.h>
#include <stdio.h>

#include "ws2812.h"
#include "ws2812_p.h"

//------------------------------ defines ------------------------------

/*! Defines the number of rows of the panel */
#define NR_ROWS         (5)

/*! Defines the number of columns of the panel */
#define NR_COLUMNS      (172)


// ----------------------------- graphics -----------------------------

/*! Get the panel's buffer

    \param[out] outBuffer   Returns a pointer to the LED buffer
*/
void ws2812_getLED_Buffer(color_f ** outBuffer);


#endif

/* eof */
