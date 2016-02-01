#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include <stdio.h>

#include "stm32f4xx.h"

//------------------------------ defines ------------------------------

/*! Defines the number of rows of the panel */
#define NR_ROWS         (5)

/*! Defines the number of columns of the panel */
#define NR_COLUMNS      (172)


//------------------------------ structs ------------------------------

/*! This structure defines an RGB color, 8bit per color */
typedef struct {
    /*! Red part of the color */
    uint8_t R;
    /*! Green part of the color */
    uint8_t G;
    /*! Blue part of the color */
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


/*! Get the panel's buffer

    \param[out] outBuffer   Returns a pointer to the LED buffer
*/
void ws2812_getLED_Buffer(color ** outBuffer);


#endif

/* eof */
