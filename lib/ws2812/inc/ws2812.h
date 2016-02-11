#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include <stdio.h>


//------------------------------ defines ------------------------------

/*! Defines the number of rows of the panel */
#define WS2812_NR_ROWS         (5)

/*! Defines the number of columns of the panel */
#define WS2812_NR_COLUMNS      (172)

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


/*! This structure defines an RGB color in float per color */
typedef struct {
    /*! Red part of the color */
    float R;
    /*! Green part of the color */
    float G;
    /*! Blue part of the color */
    float B;
} color_f;

// ----------------------------- functions -----------------------------

/*!
    Initialize the driver
*/
void ws2812_init(void);

/*!
    Send the update to the leds
*/
void ws2812_updateLED(color_f * inPanel);

// ----------------------------- graphics -----------------------------
/*!
    Set a led to a specific color

    \param[in]  inRow       The row index of the led
    \param[in]  inColumn    The column index of the led
    \param[in]  r   Red part of the color
    \param[in]  g   Green part of the color
    \param[in]  b   Blue part of the color
*/
void ws2812_setLED(color_f * inPanel, size_t inRow, size_t inColumn, uint8_t r, uint8_t g, uint8_t b);

/*!
    Set all pixels of one column to a color

    \param[in]  inColumn    The column index of the led
    \param[in]  r   Red part of the color
    \param[in]  g   Green part of the color
    \param[in]  b   Blue part of the color
*/
void ws2812_setLED_Column(color_f * inPanel, size_t inColumn, uint8_t r, uint8_t g, uint8_t b);

/*!
    Set all pixels of one row to a color

    \param[in]  inRow    The row index of the led
    \param[in]  r   Red part of the color
    \param[in]  g   Green part of the color
    \param[in]  b   Blue part of the color
*/
void ws2812_setLED_Row(color_f * inPanel, size_t inRow, uint8_t r, uint8_t g, uint8_t b);

/*!
    Set all leds by using a specific rgb color

    \param[in]  r   Red part of the color
    \param[in]  g   Green part of the color
    \param[in]  b   Blue part of the color
*/
void ws2812_setLED_All(color_f * inPanel, uint8_t r, uint8_t g, uint8_t b);

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
