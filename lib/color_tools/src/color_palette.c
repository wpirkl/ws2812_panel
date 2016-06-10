
#include <stdint.h>

#include "color.h"
#include "color_palette.h"


void color_palette_get(const ts_color_palette_16 * const inPalette, color * outColor, uint8_t inIndex) {

    uint8_t lIndex  = (inIndex >> 4) & 0x0f;
    uint8_t lIndex2 = (lIndex == 0x0f)? 0 : (lIndex + 1);
    float lInterpol = (float)(inIndex & 0x0f);

    lIndex2 = (lIndex == 0x0f)? 0 : (lIndex + 1);

    outColor->R = (uint8_t)((float)inPalette->mColors[lIndex].R * (16.0f - lInterpol) * 1.0f / 16.0f + (float)inPalette->mColors[lIndex2].R * lInterpol * 1.0f / 16.0f + 0.5f);
    outColor->G = (uint8_t)((float)inPalette->mColors[lIndex].G * (16.0f - lInterpol) * 1.0f / 16.0f + (float)inPalette->mColors[lIndex2].G * lInterpol * 1.0f / 16.0f + 0.5f);
    outColor->B = (uint8_t)((float)inPalette->mColors[lIndex].B * (16.0f - lInterpol) * 1.0f / 16.0f + (float)inPalette->mColors[lIndex2].B * lInterpol * 1.0f / 16.0f + 0.5f);
}


void color_palette_get_e(const te_color_palettes inPalette, color * outColor, uint8_t inIndex) {

    switch(inPalette) {
        case COLOR_PALETTE_RAINBOW: color_palette_get(&color_palette_rainbow, outColor, inIndex); break;
        case COLOR_PALETTE_SKY:     color_palette_get(&color_palette_sky,     outColor, inIndex); break;
        case COLOR_PALETTE_LAVA:    color_palette_get(&color_palette_lava,    outColor, inIndex); break;
        case COLOR_PALETTE_HEAT:    color_palette_get(&color_palette_heat,    outColor, inIndex); break;
        case COLOR_PALETTE_OCEAN:   color_palette_get(&color_palette_ocean,   outColor, inIndex); break;
        case COLOR_PALETTE_FOREST:  color_palette_get(&color_palette_forest,  outColor, inIndex); break;
        default: break;
    }
}


const ts_color_palette_16 color_palette_rainbow = {

    .mColors = {
        [ 0] = HTML_2_COLOR(0x5500ab),
        [ 1] = HTML_2_COLOR(0x84007c),
        [ 2] = HTML_2_COLOR(0xb5004b),
        [ 3] = HTML_2_COLOR(0xe5001b),

        [ 4] = HTML_2_COLOR(0xe81700),
        [ 5] = HTML_2_COLOR(0xe84700),
        [ 6] = HTML_2_COLOR(0xab7700),
        [ 7] = HTML_2_COLOR(0xabab00),

        [ 8] = HTML_2_COLOR(0xab5500),
        [ 9] = HTML_2_COLOR(0xdd2200),
        [10] = HTML_2_COLOR(0xf2000e),
        [11] = HTML_2_COLOR(0xc2003e),

        [12] = HTML_2_COLOR(0x8f0071),
        [13] = HTML_2_COLOR(0x5f00a1),
        [14] = HTML_2_COLOR(0x2f00d0),
        [15] = HTML_2_COLOR(0x0007f9)
    }
};


const ts_color_palette_16 color_palette_sky = {

    .mColors = {
        [ 0] = HTML_2_COLOR(HTML_Blue),
        [ 1] = HTML_2_COLOR(HTML_DarkBlue),
        [ 2] = HTML_2_COLOR(HTML_DarkBlue),
        [ 3] = HTML_2_COLOR(HTML_DarkBlue),

        [ 4] = HTML_2_COLOR(HTML_DarkBlue),
        [ 5] = HTML_2_COLOR(HTML_DarkBlue),
        [ 6] = HTML_2_COLOR(HTML_DarkBlue),
        [ 7] = HTML_2_COLOR(HTML_DarkBlue),

        [ 8] = HTML_2_COLOR(HTML_Blue),
        [ 9] = HTML_2_COLOR(HTML_DarkBlue),
        [10] = HTML_2_COLOR(HTML_SkyBlue),
        [11] = HTML_2_COLOR(HTML_SkyBlue),

        [12] = HTML_2_COLOR(HTML_LightBlue),
        [13] = HTML_2_COLOR(HTML_White),
        [14] = HTML_2_COLOR(HTML_LightBlue),
        [15] = HTML_2_COLOR(HTML_SkyBlue)
    }
};


const ts_color_palette_16 color_palette_lava = {

    .mColors = {
        [ 0] = HTML_2_COLOR(HTML_Black),
        [ 1] = HTML_2_COLOR(HTML_Maroon),
        [ 2] = HTML_2_COLOR(HTML_Black),
        [ 3] = HTML_2_COLOR(HTML_Maroon),

        [ 4] = HTML_2_COLOR(HTML_DarkRed),
        [ 5] = HTML_2_COLOR(HTML_Maroon),
        [ 6] = HTML_2_COLOR(HTML_DarkRed),
        [ 7] = HTML_2_COLOR(HTML_DarkRed),

        [ 8] = HTML_2_COLOR(HTML_DarkRed),
        [ 9] = HTML_2_COLOR(HTML_DarkRed),
        [10] = HTML_2_COLOR(HTML_Red),
        [11] = HTML_2_COLOR(HTML_Orange),

        [12] = HTML_2_COLOR(HTML_White),
        [13] = HTML_2_COLOR(HTML_Orange),
        [14] = HTML_2_COLOR(HTML_Red),
        [15] = HTML_2_COLOR(HTML_DarkRed)
    }
};


const ts_color_palette_16 color_palette_heat = {

    .mColors = {
        [ 0] = HTML_2_COLOR(0x000000),
        [ 1] = HTML_2_COLOR(0x330000),
        [ 2] = HTML_2_COLOR(0x660000),
        [ 3] = HTML_2_COLOR(0x990000),

        [ 4] = HTML_2_COLOR(0xcc0000),
        [ 5] = HTML_2_COLOR(0xff0000),
        [ 6] = HTML_2_COLOR(0xff3300),
        [ 7] = HTML_2_COLOR(0xff6600),

        [ 8] = HTML_2_COLOR(0xff9900),
        [ 9] = HTML_2_COLOR(0xffcc00),
        [10] = HTML_2_COLOR(0xffff00),
        [11] = HTML_2_COLOR(0xffff33),

        [12] = HTML_2_COLOR(0xffff66),
        [13] = HTML_2_COLOR(0xffff99),
        [14] = HTML_2_COLOR(0xffffcc),
        [15] = HTML_2_COLOR(0xffffff)
    }
};


const ts_color_palette_16 color_palette_ocean = {

    .mColors = {
        [ 0] = HTML_2_COLOR(HTML_MidnightBlue),
        [ 1] = HTML_2_COLOR(HTML_DarkBlue),
        [ 2] = HTML_2_COLOR(HTML_MidnightBlue),
        [ 3] = HTML_2_COLOR(HTML_Navy),

        [ 4] = HTML_2_COLOR(HTML_DarkBlue),
        [ 5] = HTML_2_COLOR(HTML_MediumBlue),
        [ 6] = HTML_2_COLOR(HTML_SeaGreen),
        [ 7] = HTML_2_COLOR(HTML_Teal),

        [ 8] = HTML_2_COLOR(HTML_CadetBlue),
        [ 9] = HTML_2_COLOR(HTML_Blue),
        [10] = HTML_2_COLOR(HTML_DarkCyan),
        [11] = HTML_2_COLOR(HTML_CornflowerBlue),

        [12] = HTML_2_COLOR(HTML_Aquamarine),
        [13] = HTML_2_COLOR(HTML_SeaGreen),
        [14] = HTML_2_COLOR(HTML_Aqua),
        [15] = HTML_2_COLOR(HTML_LightSkyBlue)
    }
};


const ts_color_palette_16 color_palette_forest = {

    .mColors = {
        [ 0] = HTML_2_COLOR(HTML_DarkGreen),
        [ 1] = HTML_2_COLOR(HTML_DarkGreen),
        [ 2] = HTML_2_COLOR(HTML_DarkOliveGreen),
        [ 3] = HTML_2_COLOR(HTML_DarkGreen),

        [ 4] = HTML_2_COLOR(HTML_Green),
        [ 5] = HTML_2_COLOR(HTML_ForestGreen),
        [ 6] = HTML_2_COLOR(HTML_OliveDrab),
        [ 7] = HTML_2_COLOR(HTML_Green),

        [ 8] = HTML_2_COLOR(HTML_SeaGreen),
        [ 9] = HTML_2_COLOR(HTML_MediumAquamarine),
        [10] = HTML_2_COLOR(HTML_LimeGreen),
        [11] = HTML_2_COLOR(HTML_YellowGreen),

        [12] = HTML_2_COLOR(HTML_LightGreen),
        [13] = HTML_2_COLOR(HTML_LawnGreen),
        [14] = HTML_2_COLOR(HTML_MediumAquamarine),
        [15] = HTML_2_COLOR(HTML_ForestGreen)
    }
};







/* eof */
