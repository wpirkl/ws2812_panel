#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_conf.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx.h"
#include "misc.h"
#include "main.h"

#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"

#include "ws2812.h"

#include "esp8266.h"
#include "uart_dma.h"

#include "FreeRTOS.h"
#include "task.h"

// Private variables
volatile uint32_t time_var1, time_var2;
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;

void init();

void led_task(void * inParameters) {

    /* Initialize LEDs */
    ws2812_init();

    /* power on test */
    {
        size_t lColumnNum = ws2812_getLED_PanelNumberOfColumns();
        size_t lColumnCount;
        size_t lPatternCount;
        size_t lCount;

        uint8_t lColor[3] = {0, 0, 0};

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

        vTaskDelay(1000);

        for(lColumnCount = 0; lColumnCount < lColumnNum; lColumnCount++) {
            
            if(lColumnCount > 0) {
                ws2812_setLED_Column(lColumnCount-1, 0, 0, 0);
            }
            ws2812_setLED_Column(lColumnCount, 255, 255, 255);
            
            ws2812_updateLED();
        }
        ws2812_setLED_Column(lColumnCount-1, 0, 0, 0);
        ws2812_updateLED();
        
        for(lCount = 0, lColumnCount = lColumnNum - 1; lCount < lColumnNum; lCount++, lColumnCount--) {
            if(lCount > 0) {
                ws2812_setLED_Column(lColumnCount+1, 0, 0, 0);
            }
            ws2812_setLED_Column(lColumnCount, 255, 255, 255);
            ws2812_updateLED();
        }
        ws2812_setLED_Column(0, 0, 0, 0);
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
                default:
                case 7:
                    // decrement blue
                    if(lBlue > 0) {
                        lBlue--;
                    }
                    if(lBlue == 0) {
                        lState=0;
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
#else
    for(;;) {
        vTaskDelay(1000);
    }
#endif
}

void esp8266_test_task(void * inParameters) {

    uint8_t lBuffer[128];
    size_t lRead;
    size_t lCount;
    uint8_t lChar;

    vTaskDelay(10000);

    usart_dma_open();

    printf("Started...\r\n");

    for(;;) {
        lRead = usart_dma_read(lBuffer, sizeof(lBuffer));
        for(lCount = 0; lCount < lRead; lCount++) {
            if(isprint(lBuffer[lCount])) {
                putchar(lBuffer[lCount]);
            } else if (lBuffer[lCount] == '\n') {
                printf("\\n");
            } else if(lBuffer[lCount] == '\r') {
                printf("\\r");
            } else if(isspace(lBuffer[lCount])) {
                putchar(lBuffer[lCount]);
            } else {
                printf("[0x%02x]", lBuffer[lCount]);
            }
        }

        if(VCP_get_char(&lChar)) {
            usart_dma_write(&lChar, 1);
        }
    }
}

void esp8266_rx_task(void * inParameters) {

    for(;;) {
        esp8266_rx_handler();
    }
}

void esp8266_task(void * inParameters) {

    TaskHandle_t xHandle = NULL;
    BaseType_t lRetVal;

    vTaskDelay(10000);

    esp8266_init();

    /* create rx task */
    lRetVal = xTaskCreate(esp8266_rx_task, ( const char * )"esp8266_rx", configMINIMAL_STACK_SIZE * 4, NULL, configMAX_PRIORITIES - 2, &xHandle);
    if(lRetVal) {
        printf("Successfully started RX Task\r\n");
    } else {
        printf("Failed starting RX Task\r\n");
    }

    vTaskDelay(1000);
    esp8266_setup();

    for(;;) {

        {   /* Test AT */
            printf("Sending down \"AT\"... ");
            if(esp8266_cmd_at()) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }
        }

        {   /* Reset */
            printf("Sending down \"AT+RST\"... ");
            if(esp8266_cmd_rst()) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }
        }

        {   /* Disable echo */
            printf("Sending down \"ATE0\"... ");
            if(esp8266_cmd_ate0()) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }
        }

        {   /* Get Version info */
            uint8_t lBuffer[128];
            size_t lActualSize = 0;

            printf("Sending down \"AT+GMR\"... ");
            if(esp8266_cmd_gmr(lBuffer, sizeof(lBuffer)-1, &lActualSize)) {

                printf("Success!\r\n");
                lBuffer[lActualSize] = '\0';
                printf((char*)lBuffer);

            } else {
                printf("Failed!\r\n");
            }
        }

        {   /* Test wifi modes */
            te_esp8266_wifi_mode lWifiMode;
            size_t lCount;

            for(lCount = 0; lCount < 3; lCount++) {

                switch(lCount) {
                    case 0:
                        lWifiMode = ESP8266_WIFI_MODE_STATION;
                        break;
                    case 1:
                        lWifiMode = ESP8266_WIFI_MODE_AP;
                        break;
                    case 2:
                        lWifiMode = ESP8266_WIFI_MODE_STA_AP;
                        break;
                }

                printf("Set WIFI Mode to %d... ", lWifiMode);
                if(esp8266_cmd_set_cwmode_cur(lWifiMode)) {
                    printf("Success!\r\n");
                } else {
                    printf("Failed!\r\n");
                }

                printf("Get WIFI Mode... ");
                if(esp8266_cmd_get_cwmode_cur(&lWifiMode)) {
                    printf("Success!\r\n");

                    printf("Wifi Mode is: %d\r\n", lWifiMode);

                } else {
                    printf("Failed!\r\n");
                }
            }
        }

        {   /* Join AP */
            uint8_t lSSID[] = "AndroidAP";
            uint8_t lPW[] = "cyvg3835";

            uint8_t lSSID_retrv[32];
            size_t  lSSID_retrv_len;

            static uint8_t lAccessPointList[2048];
            size_t  lAccessPointListLen;

            printf("List Access points... ");
            if(esp8266_cmd_cwlap(lAccessPointList, sizeof(lAccessPointList)-1, &lAccessPointListLen)) {
                printf("Success!\r\n");

                lAccessPointList[lAccessPointListLen] = '\0';
                printf("%s\r\n", lAccessPointList);

            } else {
                printf("Failed!\r\n");
            }

            printf("Connecting to AP... ");
            if(esp8266_cmd_set_cwjap_cur(lSSID, sizeof(lSSID)-1, lPW, sizeof(lPW)-1)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

            printf("Getting connected AP... ");
            if(esp8266_cmd_get_cwjap_cur(lSSID_retrv, sizeof(lSSID_retrv) - 1, &lSSID_retrv_len)) {
                printf("Success!\r\n");

                lSSID_retrv[lSSID_retrv_len] = '\0';
                printf("SSID is: \"%s\"\r\n", lSSID_retrv);

            } else {
                printf("Failed!\r\n");
            }
        }

        {   /* Test TCP/IP */
            ts_esp8266_socket * lSocket;
            uint8_t lAddress[] = "www.google.com";

            printf("Get Socket on www.google.com:80... ");
            if(esp8266_cmd_cipstart_tcp(&lSocket, lAddress, sizeof(lAddress)-1, 80)) {

                printf("Success!\r\n");
                printf("Socket: %p\r\n", lSocket);


                printf("Closing Socket... ");
                if(esp8266_cmd_cipclose(lSocket)) {
                    printf("Success!\r\n");
                } else {
                    printf("Failed!\r\n");
                }
            } else {
                printf("Failed!\r\n");
            }

        }

        {   /* Test multiple TCP/IP connections */
            bool lMultipleConnections;
            ts_esp8266_socket * lSocket1;
            ts_esp8266_socket * lSocket2;
            uint8_t lAddress[] = "www.google.com";

            printf("Set multiple connections... ");
            if(esp8266_cmd_set_cipmux(true)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

            printf("Get multiple connections... ");
            if(esp8266_cmd_get_cipmux(&lMultipleConnections)) {
                printf("Success!\r\n");

                printf("Multiple connections are %s\r\n", (lMultipleConnections)? "enabled" : "disabled");
            } else {
                printf("Failed!\r\n");
            }

            printf("Get Socket1 on www.google.com:80... ");
            if(esp8266_cmd_cipstart_tcp(&lSocket1, lAddress, sizeof(lAddress)-1, 80)) {
                printf("Success!\r\n");
                printf("Socket: %p\r\n", lSocket1);
            } else {
                printf("Failed!\r\n");
            }

            printf("Get Socket2 on www.google.com:80... ");
            if(esp8266_cmd_cipstart_tcp(&lSocket2, lAddress, sizeof(lAddress)-1, 80)) {
                printf("Success!\r\n");
                printf("Socket: %p\r\n", lSocket2);
            } else {
                printf("Failed!\r\n");
            }





            printf("Closing Socket1... ");
            if(esp8266_cmd_cipclose(lSocket1)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

            printf("Closing Socket2... ");
            if(esp8266_cmd_cipclose(lSocket2)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

        }

        {   /* Quit AP */
            printf("Quitting AP... ");
            if(esp8266_cmd_cwqap()) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

        }

        {   /* Test AP */

            uint8_t lSSIDBuffer[32];
            size_t  lSSIDBufferLen;

            uint8_t lPWDBuffer[64];
            size_t  lPWDBufferLen;

            uint8_t lChannel;
            te_esp8266_encryption_mode lEncryption;

            printf("Getting Access Point settings... ");
            if(esp8266_cmd_get_cwsap_cur(lSSIDBuffer, sizeof(lSSIDBuffer)-1, &lSSIDBufferLen,
                                         lPWDBuffer,  sizeof(lPWDBuffer)-1,  &lPWDBufferLen,
                                         &lChannel, &lEncryption)) {
                printf("Success!\r\n");

                lSSIDBuffer[lSSIDBufferLen] = '\0';
                lPWDBuffer[lPWDBufferLen] = '\0';

                printf("SSID: \"%s\", Password: \"%s\", Channel %d, Encryption: %d\r\n", lSSIDBuffer, lPWDBuffer, lChannel, lEncryption);

            } else {
                printf("Failed!\r\n");
            }

            lSSIDBufferLen = snprintf(lSSIDBuffer, "%s", "WP_AP");
            lPWDBufferLen  = snprintf(lPWDBuffer,  "%s", "deadbeef");

            printf("Setting Access Point... ");
            if(esp8266_cmd_set_cwsap_cur(lSSIDBuffer, lSSIDBufferLen, lPWDBuffer, lPWDBufferLen, 11, ESP8266_ENC_MODE_WPA2_PSK)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

            printf("Getting Access Point settings... ");
            if(esp8266_cmd_get_cwsap_cur(lSSIDBuffer, sizeof(lSSIDBuffer)-1, &lSSIDBufferLen,
                                         lPWDBuffer,  sizeof(lPWDBuffer)-1,  &lPWDBufferLen,
                                         &lChannel, &lEncryption)) {
                printf("Success!\r\n");

                lSSIDBuffer[lSSIDBufferLen] = '\0';
                lPWDBuffer[lPWDBufferLen] = '\0';

                printf("SSID: \"%s\", Password: \"%s\", Channel %d, Encryption: %d\r\n", lSSIDBuffer, lPWDBuffer, lChannel, lEncryption);

            } else {
                printf("Failed!\r\n");
            }
        }

        vTaskDelay(10000);
    }
}

int main(void) {

    NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

    init();

    /*
     * Disable STDOUT buffering. Otherwise nothing will be printed
     * before a newline character or when the buffer is flushed.
     */
    setbuf(stdout, NULL);

    {
        xTaskCreate(led_task,          ( const char * )"led",     configMINIMAL_STACK_SIZE *  8, NULL, configMAX_PRIORITIES - 1, NULL);
        xTaskCreate(esp8266_task,      ( const char * )"esp8266", configMINIMAL_STACK_SIZE * 32, NULL, configMAX_PRIORITIES - 2, NULL);
//        xTaskCreate(esp8266_test_task, ( const char * )"test",    configMINIMAL_STACK_SIZE * 8, NULL, configMAX_PRIORITIES - 3, NULL);

        /* Start the scheduler. */
        vTaskStartScheduler();
    }

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
