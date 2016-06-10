#ifndef COLOR_PALETTE_H_
#define COLOR_PALETTE_H_


#include "color.h"

/*! Defines the structure of a color palette */
typedef struct s_color_palette_16 {

    /* 16 colors to define the palette */
    color       mColors[16];

} ts_color_palette_16;


/*! Enumerates the color palettes */
typedef enum e_color_palettes {

    COLOR_PALETTE_RAINBOW,
    COLOR_PALETTE_SKY,
    COLOR_PALETTE_LAVA,
    COLOR_PALETTE_HEAT,
    COLOR_PALETTE_OCEAN,
    COLOR_PALETTE_FOREST,

    // has to be the last one
    COLOR_PALETTE_NUM

} te_color_palettes;


/*! Get a value of an upscaled 256 entry table

    \param[in]  inPalette   Color palette to retreive from
    \param[out] outColor    Upscaled color
    \param[in]  inIndex     Index of the entry (0-255)
*/
void color_palette_get(const ts_color_palette_16 * const inPalette, color * outColor, uint8_t inIndex);


/*! Get a value of a palette identified by an enum

    \param[in]  inPalette   Color palette to use
    \param[out] outColor    Upscaled color
    \param[in]  inIndex     Index of the entry (0-255)
*/
void color_palette_get_e(const te_color_palettes inPalette, color * outColor, uint8_t inIndex);


/*! Rainbow */
extern const ts_color_palette_16 color_palette_rainbow;


/*! Sky */
extern const ts_color_palette_16 color_palette_sky;


/*! Lava */
extern const ts_color_palette_16 color_palette_lava;


/*! Heat */
extern const ts_color_palette_16 color_palette_heat;


/*! Ocean */
extern const ts_color_palette_16 color_palette_ocean;


/*! Forest */
extern const ts_color_palette_16 color_palette_forest;


#endif /* COLOR_PALETTE_H_ */

/* eof */
