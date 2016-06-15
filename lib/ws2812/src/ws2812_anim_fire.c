#include <stdlib.h>
#include <string.h>

#include "ws2812.h"

#include "ws2812_anim_obj.h"
#include "ws2812_anim_fire.h"

#include "mt_random.h"
#include "mt_arithm.h"


#define MAX_HEAT    (240)
#define MAX_HEAT_UP (30)
#define MIN_HEAT_UP (10)
#define MAX_COOLING (60)
#define MIN_COOLING (20)


/*! This function cools everything a bit down */
static void ws2812_anim_fire_update_cool(tu_ws2812_anim * pThis) {

    size_t lCountX;
    size_t lCountY;

    uint8_t lRandom;
    uint8_t lHeat;

    /* all rows except last one (which gets replaced by shift up) */
    for(lCountY = 0; lCountY < WS2812_NR_ROWS-1; lCountY++) {
        for(lCountX = 0; lCountX < WS2812_NR_COLUMNS; lCountX++) {

            /* get a random cool down value */
            lRandom = xorshift8_range(MIN_COOLING, MAX_COOLING);
            lHeat = pThis->mFire.mHeat[lCountY * WS2812_NR_COLUMNS + lCountX];

            lHeat = sub8_f(lHeat, lRandom);

            pThis->mFire.mHeat[lCountY * WS2812_NR_COLUMNS + lCountX] = lHeat;
        }
    }
}


/*! This function shifts the heat up by 1 row and fades it by using a wighted average */
static void ws2812_anim_fire_update_fade(tu_ws2812_anim * pThis) {

    size_t lCountX;
    size_t lCountY;

    uint32_t lSum;

    /*  This should be a simplification of the following matrix:
        | 0 0 0 |
        | 1 2 1 |

        hn(x, y) = (1 * h(n-1)(x-1, y-1) + 
                    2 * h(n-1)(x, y-1) +
                    1 * h(n-1)(x+1, y-1)) / 4
    */

    /* all rows except first one */
    for(lCountY = WS2812_NR_ROWS-1; lCountY > 0; lCountY--) {
        for(lCountX = 0; lCountX < WS2812_NR_COLUMNS; lCountX++) {

            /* 2 * h(n-1)(x, y-1) */
            lSum = (pThis->mFire.mHeat[(lCountY - 1) * WS2812_NR_COLUMNS + lCountX] << 1);

            if(lCountX < WS2812_NR_COLUMNS-1) {
                /* 1 * h(n-1)(x+1, y-1) */
                lSum += pThis->mFire.mHeat[(lCountY - 1) * WS2812_NR_COLUMNS + lCountX + 1];
            } else if(lCountX > 0) {
                /* 1 * h(n-1)(x-1), y-1) */
                lSum += pThis->mFire.mHeat[(lCountY - 1) * WS2812_NR_COLUMNS + lCountX - 1];
            }

            /* divide by 4 */
            lSum >>= 2;

            pThis->mFire.mHeat[lCountY * WS2812_NR_COLUMNS + lCountX] = lSum;
        }
    }
}


/*! Add new fire */
static void ws2812_anim_fire_update_burn(tu_ws2812_anim * pThis) {

    size_t lCountX;
    uint8_t lHeat;
    uint8_t lRandom;

    uint8_t lHeatBackup[3];

    /* only on first row */
    for(lCountX = 0; lCountX < WS2812_NR_COLUMNS; lCountX++) {

        lHeat = pThis->mFire.mHeat[lCountX];
        lRandom = xorshift8_range(MIN_HEAT_UP, MAX_HEAT_UP);

        if(xorshift8() < 128) {

            /* heat up */
            lHeat = add8_cl(lHeat, lRandom, MAX_HEAT);

        } else {

            /* cool down */
            lHeat = sub8_f(lHeat, lRandom);
        }
        pThis->mFire.mHeat[lCountX] = lHeat;
    }

    /* blurr the first line to limit high variation */
    lHeatBackup[0] = 0;
    lHeatBackup[1] = pThis->mFire.mHeat[0];
    lHeatBackup[2] = pThis->mFire.mHeat[1];

    for(lCountX = 0; lCountX < WS2812_NR_COLUMNS; lCountX++) {

        lHeat = (uint8_t)(((uint32_t)lHeatBackup[0] + ((uint32_t)lHeatBackup[1] << 1) + (uint32_t)lHeatBackup[2]) >> 2);

        lHeatBackup[0] = lHeatBackup[1];
        lHeatBackup[1] = lHeatBackup[2];
        if(lCountX < WS2812_NR_COLUMNS-1) {
            lHeatBackup[2] = pThis->mFire.mHeat[lCountX + 1];
        } else {
            lHeatBackup[2] = 0;
        }
    }
}


/*! draw from palette */
static void ws2812_anim_fire_update_draw(tu_ws2812_anim * pThis) {

    size_t lCount;
    color lColor;

    for(lCount = 0; lCount < WS2812_NR_ROWS * WS2812_NR_COLUMNS; lCount++) {

        color_palette_get_e(pThis->mFire.mPalette, &lColor, pThis->mFire.mHeat[lCount]);

        pThis->mBase.mPanel[lCount].R = lColor.R;
        pThis->mBase.mPanel[lCount].G = lColor.G;
        pThis->mBase.mPanel[lCount].B = lColor.B;
    }
}


static void ws2812_anim_fire_update(tu_ws2812_anim * pThis) {

    if(pThis->mFire.mHeat) {

        /* 1st cool down */
        ws2812_anim_fire_update_cool(pThis);

        /* 2nd drift up with blurring */
        ws2812_anim_fire_update_fade(pThis);

        /* 3rd add new fire */
        ws2812_anim_fire_update_burn(pThis);

        /* 4th draw from palette */
        ws2812_anim_fire_update_draw(pThis);
    }
}


void ws2812_anim_fire_init(tu_ws2812_anim * pThis, tu_ws2812_anim_param * pParam) {

    pThis->mBase.mfUpdate   = ws2812_anim_fire_update;
    pThis->mFire.mPalette   = pParam->mFire.mPalette;

    pThis->mFire.mHeat = (uint8_t*)malloc(WS2812_NR_ROWS * WS2812_NR_COLUMNS);
    if(pThis->mFire.mHeat) {
        memset(pThis->mFire.mHeat, 0, WS2812_NR_ROWS * WS2812_NR_COLUMNS);
    }
}


void ws2812_anim_fire_clean(tu_ws2812_anim * pThis) {

    free(pThis->mFire.mHeat);
}


/* eof */
