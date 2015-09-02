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

#include "ws2812.h"

#include "FreeRTOS.h"
#include "task.h"

// Private variables
volatile uint32_t time_var1, time_var2;
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;

void init();

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName ) {

    taskDISABLE_INTERRUPTS();

    for(;;);
}



void led_task(void * inParameters) {

    /* power on test */
    {
        size_t lColumnNum = ws2812_getLED_PanelNumberOfColumns();
        size_t lColumnCount;
        size_t lPatternCount;

        uint8_t lColor[3];

        for(lPatternCount = 0; lPatternCount < 3; lPatternCount++) {

            lColor[lPatternCount] = 255;

            for(lColumnCount = 0; lColumnCount < lColumnNum; lColumnCount++) {
                switch(lColumnCount % 3) {
                    case 0:
                        ws2812_setLED_Column(lColumnCount, lColor[0], lColor[1], lColor[2]);
                        break;
                    case 1:
                        ws2812_setLED_Column(lColumnCount, lColor[2], lColor[0], lColor[1]);
                        break;
                    case 2:
                        ws2812_setLED_Column(lColumnCount, lColor[1], lColor[2], lColor[0]);
                        break;
                }
            }
            ws2812_updateLED();
            lColor[lPatternCount] = 0;

            vTaskDelay(2000);       /* delay 2 seconds */
        }


        /* turn all leds off */
        ws2812_setLED_All(0, 0, 0);
        ws2812_updateLED();
    }



#if 1
    {   /* LED test pattern */
        uint8_t lRed = 0;
        uint8_t lGreen = 0;
        uint8_t lBlue = 0;
        uint8_t lState = 0;
        for(;;) {

            /* hamilton circle over 3D color cube */
            switch(lState) {
                case 7:
                    // decrement blue
                    if(lBlue > 0) {
                        lBlue--;
                    }
                    if(lBlue == 0) {
                        lState++;
                    }
                    break;
                case 6:
                    // decrement red
                    if(lRed > 0) {
                        lRed--;
                    }
                    if(lRed == 0) {
                        lState++;
                    }
                    break;
                case 5:
                    // decrement green
                    if(lGreen > 0) {
                        lGreen--;
                    }
                    if(lGreen == 0) {
                        lState++;
                    }
                    break;
                case 4:
                    // increment red
                    if(lRed < 255) {
                        lRed++;
                    }
                    if(lRed == 255) {
                        lState++;
                    }
                    break;
                case 3:
                    // Increment Blue
                    if(lBlue < 255) {
                        lBlue++;
                    }
                    if(lBlue == 255) {
                        lState++;
                    }
                    break;
                case 2:
                    // decrement red
                    if(lRed > 0) {
                        lRed--;
                    }
                    if(lRed == 0) {
                        lState++;
                    }
                    break;
                case 1:
                    // increment green
                    if(lGreen < 255) {
                        lGreen ++;
                    }
                    if(lGreen == 255) {
                        lState++;
                    }
                    break;
                case 0:
                    // increment red
                    if(lRed < 255) {
                        lRed++;
                    }
                    if(lRed == 255) {
                        lState++;
                    }
                    break;
            }

            ws2812_setLED_All(lRed,lGreen,lBlue);
            ws2812_updateLED();
        }
    }
#endif
}

int main(void) {

    NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

    init();

    /*
     * Disable STDOUT buffering. Otherwise nothing will be printed
     * before a newline character or when the buffer is flushed.
     */
    setbuf(stdout, NULL);

    ws2812_init();

#if 1
    {
        /* Lounch other tasks in init task! */
        xTaskCreate(led_task, ( const char * )"led", configMINIMAL_STACK_SIZE * 8, NULL, 0, NULL);

        /* Start the scheduler. */
        vTaskStartScheduler();
    }
#endif

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
        uint8_t lRed = 0;
        uint8_t lGreen = 0;
        uint8_t lBlue = 0;
        uint8_t lState = 0;
        for(;;) {

            /* hamilton circle over 3D color cube */
            switch(lState) {
                case 7:
                    // decrement blue
                    if(lBlue > 0) {
                        lBlue--;
                    }
                    if(lBlue == 0) {
                        lState++;
                    }
                    break;
                case 6:
                    // decrement red
                    if(lRed > 0) {
                        lRed--;
                    }
                    if(lRed == 0) {
                        lState++;
                    }
                    break;
                case 5:
                    // decrement green
                    if(lGreen > 0) {
                        lGreen--;
                    }
                    if(lGreen == 0) {
                        lState++;
                    }
                    break;
                case 4:
                    // increment red
                    if(lRed < 255) {
                        lRed++;
                    }
                    if(lRed == 255) {
                        lState++;
                    }
                    break;
                case 3:
                    // Increment Blue
                    if(lBlue < 255) {
                        lBlue++;
                    }
                    if(lBlue == 255) {
                        lState++;
                    }
                    break;
                case 2:
                    // decrement red
                    if(lRed > 0) {
                        lRed--;
                    }
                    if(lRed == 0) {
                        lState++;
                    }
                    break;
                case 1:
                    // increment green
                    if(lGreen < 255) {
                        lGreen ++;
                    }
                    if(lGreen == 255) {
                        lState++;
                    }
                    break;
                case 0:
                    // increment red
                    if(lRed < 255) {
                        lRed++;
                    }
                    if(lRed == 255) {
                        lState++;
                    }
                    break;
            }

            setAllLED(lRed,lGreen,lBlue);
            updateLED();
        }
    }
#endif

    return 0;
}


/* initialize usb */
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
