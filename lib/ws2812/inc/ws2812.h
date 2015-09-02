#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include <stdio.h>

#include "stm32f4xx.h"

// ----------------------------- definitions -----------------------------
#define MAX(a,b)        ((a > b)? a : b)

#define NR_ROWS         (5)
#define NR_COLUMNS      (172)

#define SIZE_OF_LED     (24)      // 3(RGB) * 8 Bit

#define WS2812_TIM_FREQ       ((168000000 / 4) * 2)
#define WS2812_OUT_FREQ       (800000)

// timer values to generate a "one" or a "zero" according to ws2812 datasheet
#define WS2812_PWM_PERIOD       ((WS2812_TIM_FREQ / WS2812_OUT_FREQ))
#define WS2812_PWM_ZERO         (29)  // 0.5 탎 of 2.5탎 is high => 1/5 of the period
#define WS2812_PWM_ONE          (58) // 2탎 of 2.5탎 is high -> 4/5 of the period


// number of timer cycles (~2.5탎) for the reset pulse
#define WS2812_RESET_LEN        (20) //20*2.5탎 = 50탎


//------------------------------ structs ------------------------------
typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} color;

// ----------------------------- functions -----------------------------

/*!
    Initialize the driver
*/
void ws2812_init(void);

/*!
    Send the update to the leds
*/
void ws2812_updateLED(void);

// ----------------------------- graphics -----------------------------
/*!
    Set a led to a specific color

    \param[in]  inRow       The row index of the led
    \param[in]  inColumn    The column index of the led
    \param[in]  r   Red part of the color
    \param[in]  g   Green part of the color
    \param[in]  b   Blue part of the color
*/
void ws2812_setLED(size_t inRow, size_t inColumn, uint8_t r, uint8_t g, uint8_t b);

/*!
    Set all pixels of one column to a color

    \param[in]  inColumn    The column index of the led
    \param[in]  r   Red part of the color
    \param[in]  g   Green part of the color
    \param[in]  b   Blue part of the color
*/
void ws2812_setLED_Column(size_t inColumn, uint8_t r, uint8_t g, uint8_t b);

/*!
    Set all pixels of one row to a color

    \param[in]  inRow    The row index of the led
    \param[in]  r   Red part of the color
    \param[in]  g   Green part of the color
    \param[in]  b   Blue part of the color
*/
void ws2812_setLED_Row(size_t inRow, uint8_t r, uint8_t g, uint8_t b);

/*!
    Set all leds by using a specific rgb color

    \param[in]  r   Red part of the color
    \param[in]  g   Green part of the color
    \param[in]  b   Blue part of the color
*/
void ws2812_setLED_All(uint8_t r, uint8_t g, uint8_t b);

/*!
    Set all panel in a bulk

    Panel numbering is from right to left from top to bottom. Use getLEDPanelNumberOfRows and
    getLEDPanelNumberOfColumns to determine how many LEDs per row and per column excist

    The data is:
    - byte 0: red of row 0 column 0
    - byte 1: green of row 0 column 0
    - byte 2: blue of row 0 column 0
    - byte 3: red of row 0 column 1
    - byte 4: green of row 0 column 1
    - byte 5: blue of row 0 column 1
    - ...
    - byte (getLEDPanelNumberOfColumns()-1) * 3 + 0: red of last pixel in first row
    - byte (getLEDPanelNumberOfColumns()-1) * 3 + 1: green of last pixel in first row
    - byte (getLEDPanelNumberOfColumns()-1) * 3 + 2: blue of last pixel in first row
    - byte 3*getLEDPanelNumberOfColumns() + 0: red of row 1 column 0
    - byte 3*getLEDPanelNumberOfColumns() + 1: green of row 1 column 0
    - byte 3*getLEDPanelNumberOfColumns() + 2: blue of row 1 column 0
    - ...
    - byte 3*(getLEDPanelNumberOfColumns()*getLEDPanelNumberOfRows() - 1) + 0: red of last pixel in last row
    - byte 3*(getLEDPanelNumberOfColumns()*getLEDPanelNumberOfRows() - 1) + 1: green of last pixel in last row
    - byte 3*(getLEDPanelNumberOfColumns()*getLEDPanelNumberOfRows() - 1) + 2: blue of last pixel in last row

    \param[in]  inBuffer        buffer containing panel data
    \param[in]  inOffset        offset into panel data
    \param[in]  inBufferSize    number of bytes to send
*/
void ws2812_setLED_bulk(uint8_t * inBuffer, size_t inOffset, size_t inBufferSize);

/*!
    Get the number of leds in one row

    \return the number of leds in one row
*/
size_t ws2812_getLED_PanelNumberOfRows(void);

/*!
    Get the number of leds in one column

    \return the number of leds in one column
*/
size_t ws2812_getLED_PanelNumberOfColumns(void);



#endif

/* eof */
