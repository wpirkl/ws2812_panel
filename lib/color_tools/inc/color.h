#ifndef COLOR_H_
#define COLOR_H_

#include <stdint.h>


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




static inline float uint8_to_float(uint8_t inColor) {

    return inColor * 1.0f / 255.0f;
}


static inline uint8_t float_to_uint8(float inColor) {

    return (uint8_t)((inColor * 255.0f) + 0.5f);
}


static inline void color_to_color_f(color_f * outColor, color * inColor) {

    outColor->R = uint8_to_float(inColor->R);
    outColor->G = uint8_to_float(inColor->G);
    outColor->B = uint8_to_float(inColor->B);
}


static inline void color_f_to_color(color * outColor, color_f * inColor) {

    outColor->R = float_to_uint8(inColor->R);
    outColor->G = float_to_uint8(inColor->G);
    outColor->B = float_to_uint8(inColor->B);
}




/*! HTML color code predefined colors -- taken from FastLED */
typedef enum {
    HTML_AliceBlue               = 0xF0F8FF,
    HTML_Amethyst                = 0x9966CC,
    HTML_AntiqueWhite            = 0xFAEBD7,
    HTML_Aqua                    = 0x00FFFF,
    HTML_Aquamarine              = 0x7FFFD4,
    HTML_Azure                   = 0xF0FFFF,
    HTML_Beige                   = 0xF5F5DC,
    HTML_Bisque                  = 0xFFE4C4,
    HTML_Black                   = 0x000000,
    HTML_BlanchedAlmond          = 0xFFEBCD,
    HTML_Blue                    = 0x0000FF,
    HTML_BlueViolet              = 0x8A2BE2,
    HTML_Brown                   = 0xA52A2A,
    HTML_BurlyWood               = 0xDEB887,
    HTML_CadetBlue               = 0x5F9EA0,
    HTML_Chartreuse              = 0x7FFF00,
    HTML_Chocolate               = 0xD2691E,
    HTML_Coral                   = 0xFF7F50,
    HTML_CornflowerBlue          = 0x6495ED,
    HTML_Cornsilk                = 0xFFF8DC,
    HTML_Crimson                 = 0xDC143C,
    HTML_Cyan                    = 0x00FFFF,
    HTML_DarkBlue                = 0x00008B,
    HTML_DarkCyan                = 0x008B8B,
    HTML_DarkGoldenrod           = 0xB8860B,
    HTML_DarkGray                = 0xA9A9A9,
    HTML_DarkGrey                = 0xA9A9A9,
    HTML_DarkGreen               = 0x006400,
    HTML_DarkKhaki               = 0xBDB76B,
    HTML_DarkMagenta             = 0x8B008B,
    HTML_DarkOliveGreen          = 0x556B2F,
    HTML_DarkOrange              = 0xFF8C00,
    HTML_DarkOrchid              = 0x9932CC,
    HTML_DarkRed                 = 0x8B0000,
    HTML_DarkSalmon              = 0xE9967A,
    HTML_DarkSeaGreen            = 0x8FBC8F,
    HTML_DarkSlateBlue           = 0x483D8B,
    HTML_DarkSlateGray           = 0x2F4F4F,
    HTML_DarkSlateGrey           = 0x2F4F4F,
    HTML_DarkTurquoise           = 0x00CED1,
    HTML_DarkViolet              = 0x9400D3,
    HTML_DeepPink                = 0xFF1493,
    HTML_DeepSkyBlue             = 0x00BFFF,
    HTML_DimGray                 = 0x696969,
    HTML_DimGrey                 = 0x696969,
    HTML_DodgerBlue              = 0x1E90FF,
    HTML_FireBrick               = 0xB22222,
    HTML_FloralWhite             = 0xFFFAF0,
    HTML_ForestGreen             = 0x228B22,
    HTML_Fuchsia                 = 0xFF00FF,
    HTML_Gainsboro               = 0xDCDCDC,
    HTML_GhostWhite              = 0xF8F8FF,
    HTML_Gold                    = 0xFFD700,
    HTML_Goldenrod               = 0xDAA520,
    HTML_Gray                    = 0x808080,
    HTML_Grey                    = 0x808080,
    HTML_Green                   = 0x008000,
    HTML_GreenYellow             = 0xADFF2F,
    HTML_Honeydew                = 0xF0FFF0,
    HTML_HotPink                 = 0xFF69B4,
    HTML_IndianRed               = 0xCD5C5C,
    HTML_Indigo                  = 0x4B0082,
    HTML_Ivory                   = 0xFFFFF0,
    HTML_Khaki                   = 0xF0E68C,
    HTML_Lavender                = 0xE6E6FA,
    HTML_LavenderBlush           = 0xFFF0F5,
    HTML_LawnGreen               = 0x7CFC00,
    HTML_LemonChiffon            = 0xFFFACD,
    HTML_LightBlue               = 0xADD8E6,
    HTML_LightCoral              = 0xF08080,
    HTML_LightCyan               = 0xE0FFFF,
    HTML_LightGoldenrodYellow    = 0xFAFAD2,
    HTML_LightGreen              = 0x90EE90,
    HTML_LightGrey               = 0xD3D3D3,
    HTML_LightPink               = 0xFFB6C1,
    HTML_LightSalmon             = 0xFFA07A,
    HTML_LightSeaGreen           = 0x20B2AA,
    HTML_LightSkyBlue            = 0x87CEFA,
    HTML_LightSlateGray          = 0x778899,
    HTML_LightSlateGrey          = 0x778899,
    HTML_LightSteelBlue          = 0xB0C4DE,
    HTML_LightYellow             = 0xFFFFE0,
    HTML_Lime                    = 0x00FF00,
    HTML_LimeGreen               = 0x32CD32,
    HTML_Linen                   = 0xFAF0E6,
    HTML_Magenta                 = 0xFF00FF,
    HTML_Maroon                  = 0x800000,
    HTML_MediumAquamarine        = 0x66CDAA,
    HTML_MediumBlue              = 0x0000CD,
    HTML_MediumOrchid            = 0xBA55D3,
    HTML_MediumPurple            = 0x9370DB,
    HTML_MediumSeaGreen          = 0x3CB371,
    HTML_MediumSlateBlue         = 0x7B68EE,
    HTML_MediumSpringGreen       = 0x00FA9A,
    HTML_MediumTurquoise         = 0x48D1CC,
    HTML_MediumVioletRed         = 0xC71585,
    HTML_MidnightBlue            = 0x191970,
    HTML_MintCream               = 0xF5FFFA,
    HTML_MistyRose               = 0xFFE4E1,
    HTML_Moccasin                = 0xFFE4B5,
    HTML_NavajoWhite             = 0xFFDEAD,
    HTML_Navy                    = 0x000080,
    HTML_OldLace                 = 0xFDF5E6,
    HTML_Olive                   = 0x808000,
    HTML_OliveDrab               = 0x6B8E23,
    HTML_Orange                  = 0xFFA500,
    HTML_OrangeRed               = 0xFF4500,
    HTML_Orchid                  = 0xDA70D6,
    HTML_PaleGoldenrod           = 0xEEE8AA,
    HTML_PaleGreen               = 0x98FB98,
    HTML_PaleTurquoise           = 0xAFEEEE,
    HTML_PaleVioletRed           = 0xDB7093,
    HTML_PapayaWhip              = 0xFFEFD5,
    HTML_PeachPuff               = 0xFFDAB9,
    HTML_Peru                    = 0xCD853F,
    HTML_Pink                    = 0xFFC0CB,
    HTML_Plaid                   = 0xCC5533,
    HTML_Plum                    = 0xDDA0DD,
    HTML_PowderBlue              = 0xB0E0E6,
    HTML_Purple                  = 0x800080,
    HTML_Red                     = 0xFF0000,
    HTML_RosyBrown               = 0xBC8F8F,
    HTML_RoyalBlue               = 0x4169E1,
    HTML_SaddleBrown             = 0x8B4513,
    HTML_Salmon                  = 0xFA8072,
    HTML_SandyBrown              = 0xF4A460,
    HTML_SeaGreen                = 0x2E8B57,
    HTML_Seashell                = 0xFFF5EE,
    HTML_Sienna                  = 0xA0522D,
    HTML_Silver                  = 0xC0C0C0,
    HTML_SkyBlue                 = 0x87CEEB,
    HTML_SlateBlue               = 0x6A5ACD,
    HTML_SlateGray               = 0x708090,
    HTML_SlateGrey               = 0x708090,
    HTML_Snow                    = 0xFFFAFA,
    HTML_SpringGreen             = 0x00FF7F,
    HTML_SteelBlue               = 0x4682B4,
    HTML_Tan                     = 0xD2B48C,
    HTML_Teal                    = 0x008080,
    HTML_Thistle                 = 0xD8BFD8,
    HTML_Tomato                  = 0xFF6347,
    HTML_Turquoise               = 0x40E0D0,
    HTML_Violet                  = 0xEE82EE,
    HTML_Wheat                   = 0xF5DEB3,
    HTML_White                   = 0xFFFFFF,
    HTML_WhiteSmoke              = 0xF5F5F5,
    HTML_Yellow                  = 0xFFFF00,
    HTML_YellowGreen             = 0x9ACD32,
    HTML_FairyLight              = 0xFFE42D,
    HTML_FairyLightNCC           = 0xFF9D2A
} HTMLColorCode;

#define HTML_2_COLOR(x) \
    { \
        .R = (uint8_t)((x >> 16) & 0xff), \
        .G = (uint8_t)((x >>  8) & 0xff), \
        .B = (uint8_t)((x >>  0) & 0xff)  \
    }


#endif /* COLOR_H_ */

/* eof */
