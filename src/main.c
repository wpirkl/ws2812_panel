#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "main.h"

#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"

// #include "stm32_ub_ws2812_8ch.h"
#include "ws2812.h"

// Private variables
volatile uint32_t time_var1, time_var2;
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;


// Private function prototypes
void Delay(volatile uint32_t nCount);
void init();
void calculation_test();

int main(void) {

    NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

    init();

    /*
     * Disable STDOUT buffering. Otherwise nothing will be printed
     * before a newline character or when the buffer is flushed.
     */
    setbuf(stdout, NULL);

    ws2812_init();

#if 0
    for(;;) {

        uint8_t lCharacter;
        size_t lRow;
        size_t lColumn;
        uint8_t lRed;
        uint8_t lGreen;
        uint8_t lBlue;

        lCharacter = getchar();

        switch(lCharacter) {
            case 'a':
                lRed   = getchar();
                lGreen = getchar();
                lBlue  = getchar();

                setAllLED(lRed,lGreen,lBlue);
                break;
            case 'l':
                lRow    = getchar();
                lColumn = getchar() << 8;
                lColumn = lColumn | getchar();

                lRed   = getchar();
                lGreen = getchar();
                lBlue  = getchar();

                setLED(lRow, lColumn, lRed, lGreen, lBlue);
                break;
            case 'u':
                updateLED();
                break;
        }


    }

#endif

#if 0
    {
        uint8_t lCount = 0;
        uint8_t lForward = 0;
        for(;;lCount++) {
            if(lCount == 0) {
                lForward = (lForward + 1) & 1;
            }

            if(lForward) {
                setAllLED(lCount, lCount, lCount);
            } else {
                setAllLED(255-lCount, 255-lCount, 255-lCount);
            }
            updateLED();
        }
    }
#endif

#if 1
    {
        uint8_t lRed = 0;
        uint8_t lGreen = 255;
        uint8_t lBlue = 0;
        uint8_t lState = 0;
        for(;;) {

            switch(lState) {
                case 2:
                    // increment green, decrement blue
                    if(lGreen < 255) {
                        lGreen ++;
                        lBlue --;
                    }
                    if(lGreen == 255) {
                        lState = 0;
                    }
                    break;
                case 1:
                    // increment blue, decrement red
                    if(lBlue < 255) {
                        lBlue ++;
                        lRed --;
                    }
                    if(lBlue == 255) {
                        lState = 2;
                    }
                    break;
                case 0:
                default:
                    // increment red, decrement green
                    if(lRed < 255) {
                        lRed++;
                                        lGreen--;
                    }
                    if(lRed == 255) {
                         lState = 1;
                    }
                    break;
            }

            setAllLED(lRed,lGreen,lBlue);
            updateLED();
        }
    }
#endif

#if 0
    {
        size_t lLedColumn = 0;

        for(;;) {

            setLED(lLedColumn, 0, 0, 0);
            lLedColumn = (lLedColumn + 1) % 172;
            setLED(lLedColumn, 255,255,255);
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
            updateLED();
        }

    }

#endif

    return 0;
}


void init() {
    GPIO_InitTypeDef  GPIO_InitStructure;

    // ---------- GPIO -------- //
    // GPIOD Periph clock enable
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    // Configure PD12, PD13, PD14 and PD15 in output pushpull mode
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // ------------- USB -------------- //
    USBD_Init(&USB_OTG_dev,
              USB_OTG_FS_CORE_ID,
              &USR_desc,
              &USBD_CDC_cb,
              &USR_cb);
}

void _init(void) {
    
}

/* eof */
