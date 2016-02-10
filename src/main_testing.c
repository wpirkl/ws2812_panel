#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f4xx_gpio.h" // for GPIO_InitTypeDef, GPIO_Init, GPIO_ResetBits and GPIO_SetBits

#include "usbd_cdc_vcp.h"   // for VCP_get_char

#include "init.h"

#include "ws2812.h"
#include "ws2812_anim.h"

#include "esp8266.h"
#include "esp8266_http_server.h"

#include "web_content_handler.h"

#include "uart_dma.h"

#include "MQTTClient.h"

/* place heap into ccm */
uint8_t __attribute__ ((section(".ccmbss"), aligned(8))) ucHeap[ configTOTAL_HEAP_SIZE ];

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName) {

    printf("%s(%d): Stack overflow on task %p \"%s\"!\r\n", __FILE__, __LINE__, xTask, pcTaskName);
}

void vApplicationMallocFailedHook( void ) {

    printf("%s(%d): Malloc failed active task: %p\r\n", __FILE__, __LINE__, xTaskGetCurrentTaskHandle());
}

static volatile uint32_t s100percentIdle = 0;
static volatile uint32_t sLoadCounter = 0;
static volatile uint32_t sCurrentLoad = 0;

void vApplicationIdleHook( void ) {

    sLoadCounter++;
}

void cpu_load_task(void * inParameters) {

    uint32_t lLastCounter;

    for(;;) {

        lLastCounter = sLoadCounter;

        vTaskDelay(1000);

        sCurrentLoad = sLoadCounter - lLastCounter;
    }
}

void led_task(void * inParameters) {

    /* Initialize LEDs */
    ws2812_init();

    /* Initialize animation */
    ws2812_animation_init();


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

#if 0
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
        ws2812_animation_main();
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

void esp8266_test_server_handler_task(ts_esp8266_socket * inSocket) {

    uint8_t lBuffer[128];
    size_t lRcvLen;

    printf("%s(%d): alive on socket %p\r\n", __func__, __LINE__, inSocket);

    for(;;) {
        if(esp8266_receive(inSocket, lBuffer, sizeof(lBuffer), &lRcvLen)) {
            printf("Received %d bytes\r\n", lRcvLen);

            if(!esp8266_cmd_cipsend_tcp(inSocket, lBuffer, lRcvLen)) {
                printf("Send failed!\r\n");
                break;
            }
        } else {
            printf("Receive failed!\r\n");
            break;
        }
    }
}

void esp8266_rx_task(void * inParameters) {

    for(;;) {
        esp8266_rx_handler();
    }
}

void esp8266_socket_task(void * inParameters) {

    for(;;) {
        esp8266_socket_handler();
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

    /* create socket task */
    lRetVal = xTaskCreate(esp8266_socket_task, ( const char * )"esp8266_so", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, &xHandle);
    if(lRetVal) {
        printf("Successfully started RX Task\r\n");
    } else {
        printf("Failed starting RX Task\r\n");
    }

    vTaskDelay(1000);

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

        {   /* Test PING */
            uint8_t lAddress[] = "www.google.com";
            uint32_t lPingTime;

            printf("Ping www.google.com... ");
            if(esp8266_cmd_ping(lAddress, sizeof(lAddress), &lPingTime)) {
                printf("Success!\r\n");
                printf("Ping response time: %" PRIu32 "\r\n", lPingTime);
            } else {
                printf("Failed!\r\n");
            }
        }

        {   /* Reset cipmux */

            printf("Set multiple connections to false... ");
            if(esp8266_cmd_set_cipmux(false)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }
        }

        {   /* Test TCP/IP */
            ts_esp8266_socket * lSocket1;
            uint8_t lAddress[] = "www.google.com";

            uint8_t lHTTPGet[] = "GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n\r\n";

            static uint8_t lBuffer[1025];
            size_t lBufferLen;

            size_t lCount;

            printf("Get Socket on www.google.com:80... ");
            if(esp8266_cmd_cipstart_tcp(&lSocket1, lAddress, sizeof(lAddress)-1, 80)) {

                printf("Success!\r\n");
                printf("Socket: %p\r\n", lSocket1);
            } else {
                printf("Failed!\r\n");
            }

            printf("Send GET... ");
            if(esp8266_cmd_cipsend_tcp(lSocket1, lHTTPGet, sizeof(lHTTPGet)-1)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

            printf("Receiving Data... ");
            if(esp8266_receive(lSocket1, lBuffer, sizeof(lBuffer)-1, &lBufferLen)) {
                printf("Success!\r\n");
                printf("Received %d bytes\r\n", lBufferLen);
                lBuffer[lBufferLen] = '\0';
                printf("Buffer:\r\n%s\r\nEnd of buffer\r\n", lBuffer);
            } else {
                printf("Failed!\r\n");
            }

            printf("Closing Socket... ");
            if(esp8266_cmd_cipclose(lSocket1)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

            printf("Test again with small blocks\r\n");
            printf("Get Socket on www.google.com:80... ");
            if(esp8266_cmd_cipstart_tcp(&lSocket1, lAddress, sizeof(lAddress)-1, 80)) {

                printf("Success!\r\n");
                printf("Socket: %p\r\n", lSocket1);
            } else {
                printf("Failed!\r\n");
            }

            printf("Send GET... ");
            if(esp8266_cmd_cipsend_tcp(lSocket1, lHTTPGet, sizeof(lHTTPGet)-1)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

            for(lCount = 0;;lCount++) {

                uint8_t lSmallBuffer[129];
                size_t lSmallBufferLen = 0;

                printf("Junk %d\r\n", lCount);
                if(esp8266_receive(lSocket1, lSmallBuffer, sizeof(lSmallBuffer)-1, &lSmallBufferLen)) {
                    printf("Received %d bytes\r\n", lSmallBufferLen);
                    lSmallBuffer[lSmallBufferLen] = '\0';
                    printf("Buffer:\r\n%s\r\nEnd of buffer\r\n", lSmallBuffer);
                    if(lSmallBufferLen < sizeof(lSmallBuffer)-1) {
                        break;
                    }
                } else {
                    printf("Failed!\r\n");
                    break;
                }
            }

            printf("Closing Socket... ");
            if(esp8266_cmd_cipclose(lSocket1)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }


        }

        {   /* Test multiple TCP/IP connections */
            bool lMultipleConnections;
            ts_esp8266_socket * lSocket1;
            ts_esp8266_socket * lSocket2;
            uint8_t lAddress[] = "www.google.com";

            uint8_t lHTTPGet[] = "GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n\r\n";

            static uint8_t lBuffer[1024];
            size_t  lBufferLen;

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


            printf("Send GET on socket 1... ");
            if(esp8266_cmd_cipsend_tcp(lSocket1, lHTTPGet, sizeof(lHTTPGet)-1)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

            printf("Send GET on socket 2... ");
            if(esp8266_cmd_cipsend_tcp(lSocket2, lHTTPGet, sizeof(lHTTPGet)-1)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }

            printf("Receiving Data on socket 1... ");
            if(esp8266_receive(lSocket1, lBuffer, sizeof(lBuffer)-1, &lBufferLen)) {
                printf("Success!\r\n");
                printf("Received %d bytes\r\n", lBufferLen);
                lBuffer[lBufferLen] = '\0';
                printf("Buffer:\r\n%s\r\nEnd of buffer\r\n", lBuffer);
            } else {
                printf("Failed!\r\n");
            }

            printf("Receiving Data on socket 2... ");
            if(esp8266_receive(lSocket2, lBuffer, sizeof(lBuffer)-1, &lBufferLen)) {
                printf("Success!\r\n");
                printf("Received %d bytes\r\n", lBufferLen);
                lBuffer[lBufferLen] = '\0';
                printf("Buffer:\r\n%s\r\nEnd of buffer\r\n", lBuffer);
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

            lSSIDBufferLen = snprintf((char*)lSSIDBuffer, sizeof(lSSIDBuffer), "%s", "WP_AP");
            lPWDBufferLen  = snprintf((char*)lPWDBuffer,  sizeof(lPWDBuffer),  "%s", "deadbeef");

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

        {   /* list ap stations */
            uint8_t lStationList[1024];
            size_t lStationListLen;
            size_t lCount;

            for(lCount = 0; lCount < 10; lCount++) {
                printf("[%d] Getting connected stations... ", lCount);
                if(esp8266_cmd_get_cwlif(lStationList, sizeof(lStationList)-1, &lStationListLen)) {
                    printf("Success!\r\n");

                    lStationList[lStationListLen] = '\0';
                    printf("%s\r\n", lStationList);
                } else {
                    printf("Failed!\r\n");
                }

                /* sleep 2 sec */
                vTaskDelay(2000);
            }
        }

        {   /* Test TCP Server */
            printf("Starting Echo server on port 23... ");
            if(esp8266_cmd_cipserver(23, esp8266_test_server_handler_task, configMAX_PRIORITIES - 3, configMINIMAL_STACK_SIZE * 4)) {
                printf("Success!\r\n");
            } else {
                printf("Failed!\r\n");
            }
        }

        {   /* test GOOGLE in a loop */
            size_t lCount;

            for(lCount = 0; lCount < 1000; lCount++) {

                ts_esp8266_socket * lSocket1;

                printf("Loop %d\r\n", lCount);
                uint8_t lAddress[] = "www.google.com";

                uint8_t lHTTPGet[] = "GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n\r\n";

                static uint8_t lBuffer[1024];
                size_t lBufferLen;

                if(!esp8266_cmd_cipstart_tcp(&lSocket1, lAddress, sizeof(lAddress)-1, 80)) {
                    printf("Connect Failed!\r\n");
                    continue;
                }

                if(!esp8266_cmd_cipsend_tcp(lSocket1, lHTTPGet, sizeof(lHTTPGet)-1)) {
                    printf("Send Failed!\r\n");
                    esp8266_cmd_cipclose(lSocket1);
                    continue;
                }

                if(!esp8266_receive(lSocket1, lBuffer, sizeof(lBuffer), &lBufferLen)) {
                    printf("Receive Failed!\r\n");
                }

                esp8266_cmd_cipclose(lSocket1);
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

        /* sleep forever */
        for(;;) {
            vTaskDelay(10000);
        }
    }
}


/*! HTTP server data */
typedef struct {

    /*! Mutex which handles data structure access */
    SemaphoreHandle_t mMutex;

    /*! A counter just for testing */
    uint32_t    mCounter;

    /*! is SSID updated */
    size_t      mSSIDLen;

    /*! is Password updated */
    size_t      mPassLen;

    /*! Buffer for SSID */
    uint8_t     mSSID[32];

    /*! Buffer for Password */
    uint8_t     mPass[65];

    /*! Animation received */
    bool        mAnimationReceived;

    /*! Animation */
    size_t      mAnimation;

    /*! Color */
    color       mColor;

} ts_myUserData;

static ts_myUserData sUserData = {
    .mCounter = 0,
    .mSSIDLen = 0,
    .mPassLen = 0,
    .mAnimationReceived = false
};

bool esp8266_http_test_web_content_get_status_ssid(void * inUserData, char * outBuffer, size_t inBufferSize, size_t * outBufferLen) {

    uint8_t lSSID_retrv[32];
    size_t  lSSID_retrv_len;

//    ts_myUserData * lUserData = (ts_myUserData*)inUserData;

//    printf("%s(%d)\r\n", __func__, __LINE__);

    if(esp8266_cmd_get_cwjap_cur(lSSID_retrv, sizeof(lSSID_retrv) - 1, &lSSID_retrv_len)) {

        lSSID_retrv[lSSID_retrv_len] = '\0';
        *outBufferLen = snprintf(outBuffer, inBufferSize, "%s", lSSID_retrv);

    } else {
        *outBufferLen = snprintf(outBuffer, inBufferSize, "NONE");
    }

    return true;
}

bool esp8266_http_test_web_content_get_counter(void * inUserData, char * outBuffer, size_t inBufferSize, size_t * outBufferLen) {

    ts_myUserData * lUserData = (ts_myUserData*)inUserData;

//    printf("%s(%d)\r\n", __func__, __LINE__);

    *outBufferLen = snprintf(outBuffer, inBufferSize, "%lu", lUserData->mCounter++);

    return true;
}

bool esp8266_http_test_web_content_get_ver(void * inUserData, char * outBuffer, size_t inBufferSize, size_t * outBufferLen) {

//    printf("%s(%d)\r\n", __func__, __LINE__);

    *outBufferLen = snprintf(outBuffer, inBufferSize, "1.0");

    return true;
}

bool esp8266_http_test_web_content_get_cpu(void * inUserData, char * outBuffer, size_t inBufferSize, size_t * outBufferLen) {

//    printf("%s(%d)\r\n", __func__, __LINE__);

    *outBufferLen = snprintf(outBuffer, inBufferSize, "%lu", 100 - ((100 * sCurrentLoad) / s100percentIdle));

    return true;
}

bool esp8266_http_test_web_content_set_var(void * inUserData, const char * const inValue, size_t inValueLength) {

    char lBuffer[16];
    size_t lCopyLen = (inValueLength > sizeof(lBuffer)-1)? (sizeof(lBuffer) - 1) : inValueLength;

    memcpy(lBuffer, inValue, lCopyLen);
    lBuffer[lCopyLen] = '\0';

    printf("%s(%d): \"%s\"\r\n", __func__, __LINE__, lBuffer);

    return true;
}

bool esp8266_http_test_web_content_set_ssid(void * inUserData, const char * const inValue, size_t inValueLength) {

    ts_myUserData * lUserData = (ts_myUserData*)inUserData;

    printf("%s(%d)\r\n", __func__, __LINE__);

    if(inValueLength < sizeof(lUserData->mSSID) -1) {

        memcpy(lUserData->mSSID, inValue, inValueLength);
        lUserData->mSSIDLen = inValueLength;
    }

    return true;
}

bool esp8266_http_test_web_content_set_password(void * inUserData, const char * const inValue, size_t inValueLength) {

    ts_myUserData * lUserData = (ts_myUserData*)inUserData;

    printf("%s(%d)\r\n", __func__, __LINE__);

    if(inValueLength < sizeof(lUserData->mPass) - 1) {

        memcpy(lUserData->mPass, inValue, inValueLength);
        lUserData->mPassLen = inValueLength;
    }

    return true;
}

bool esp8266_http_test_web_content_set_animation(void * inUserData, const char * const inValue, size_t inValueLength) {

    ts_myUserData * lUserData = (ts_myUserData*)inUserData;

    char lBuffer[12];
    size_t lLen;

    lLen = ((sizeof(lBuffer)-1) < inValueLength)? (sizeof(lBuffer)-1) : inValueLength;

    memcpy(lBuffer, inValue, lLen);
    lBuffer[lLen] = '\0';

    lUserData->mAnimationReceived = true;

    lUserData->mAnimation = strtoul(lBuffer, NULL, 10);

    printf("%s(%d): %s\r\n", __func__, __LINE__, lBuffer);

    return true;
}

static uint8_t hex_decode(char c) {

    if(c >= '0' && c <= '9') {
        return c - '0';
    } else if(c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if(c >= 'A' && c <= 'F') {
        return c - 'a' + 10;
    } else {
        return 0;
    }
}

bool esp8266_http_test_web_content_set_color(void * inUserData, const char * const inValue, size_t inValueLength) {

    ts_myUserData * lUserData = (ts_myUserData*)inUserData;

    char lBuffer[8];
    size_t lLen;

    lLen = ((sizeof(lBuffer)-1) < inValueLength)? (sizeof(lBuffer)-1) : inValueLength;

    memcpy(lBuffer, inValue, lLen);
    lBuffer[lLen] = '\0';

    printf("%s(%d): %s\r\n", __func__, __LINE__, lBuffer);

    if(inValueLength == 7) {
        /* first character is # */
        lUserData->mColor.R = (hex_decode(inValue[1]) << 4) | hex_decode(inValue[2]);
        lUserData->mColor.G = (hex_decode(inValue[3]) << 4) | hex_decode(inValue[4]);
        lUserData->mColor.B = (hex_decode(inValue[5]) << 4) | hex_decode(inValue[6]);
    }

    printf("%s(%d): decoded color R: %02x, G: %02x, B: %02x\r\n", __func__, __LINE__, lUserData->mColor.R, lUserData->mColor.G, lUserData->mColor.B);

    return true;
}

bool esp8266_http_test_web_content_set_transition(void * inUserData, const char * const inValue, size_t inValueLength) {

    char lBuffer[12];
    size_t lLen;

    lLen = ((sizeof(lBuffer)-1) < inValueLength)? (sizeof(lBuffer)-1) : inValueLength;

    memcpy(lBuffer, inValue, lLen);
    lBuffer[lLen] = '\0';

    printf("%s(%d): %s\r\n", __func__, __LINE__, lBuffer);

    return true;
}

bool esp8266_http_test_web_content_set_transition_time(void * inUserData, const char * const inValue, size_t inValueLength) {

    char lBuffer[12];
    size_t lLen;

    lLen = ((sizeof(lBuffer)-1) < inValueLength)? (sizeof(lBuffer)-1) : inValueLength;

    memcpy(lBuffer, inValue, lLen);
    lBuffer[lLen] = '\0';

    printf("%s(%d): %s\r\n", __func__, __LINE__, lBuffer);

    return true;
}

void esp8266_http_test_web_content_start_parse(void * inUserData) {

    ts_myUserData * lUserData = (ts_myUserData*)inUserData;

    printf("%s(%d)\r\n", __func__, __LINE__);

    xSemaphoreTakeRecursive(lUserData->mMutex, portMAX_DELAY);
}

void esp8266_http_test_web_content_done_parse(void * inUserData) {

    ts_myUserData * lUserData = (ts_myUserData*)inUserData;

    printf("%s(%d)\r\n", __func__, __LINE__);

    /* wifi form */
    if(lUserData->mSSIDLen > 0 && lUserData->mPassLen > 0) {

        printf("Connecting to AP... ");
        if(esp8266_cmd_set_cwjap_cur(lUserData->mSSID, lUserData->mSSIDLen, lUserData->mPass, lUserData->mPassLen)) {
            printf("Success!\r\n");
        } else {
            printf("Failed!\r\n");
        }

        /* clear ssid received */
        lUserData->mSSIDLen = 0;
        lUserData->mPassLen = 0;
    }

    /* animation form */
    if(lUserData->mAnimationReceived) {

        switch(lUserData->mAnimation) {
            case 0:     /* constant color */

                printf("%s(%d): Animation constant color\r\n", __FILE__, __LINE__);
                ws2812_anim_const_color(lUserData->mColor.R, lUserData->mColor.G, lUserData->mColor.B);
                break;
            default:    /* unkonwn animation */
                break;
        }

        /* clear animation received */
        lUserData->mAnimationReceived = false;
    }

    xSemaphoreGiveRecursive(lUserData->mMutex);
}

static const ts_web_content_handlers sTestWebContent = {

    .mHandlerCount = 10,
    .mParsingStart = esp8266_http_test_web_content_start_parse,
    .mParsingDone  = esp8266_http_test_web_content_done_parse,
    .mUserData = (void*)&sUserData,
    .mHandler = {
        {
            .mToken = "ver",
            .mGet = esp8266_http_test_web_content_get_ver,
            .mSet = esp8266_http_test_web_content_set_var,
        },
        {
            .mToken = "counter",
            .mGet = esp8266_http_test_web_content_get_counter,
            .mSet = NULL,
        },
        {
            .mToken = "statusssid",
            .mGet = esp8266_http_test_web_content_get_status_ssid,
            .mSet = NULL,
        },
        {
            .mToken = "cpuload",
            .mGet = esp8266_http_test_web_content_get_cpu,
            .mSet = NULL,
        },
        {
            .mToken = "ssid",
            .mGet = NULL,
            .mSet = esp8266_http_test_web_content_set_ssid,
        },
        {
            .mToken = "password",
            .mGet = NULL,
            .mSet = esp8266_http_test_web_content_set_password,
        },
        {
            .mToken = "ani",
            .mGet = NULL,
            .mSet = esp8266_http_test_web_content_set_animation,
        },
        {
            .mToken = "ancol",
            .mGet = NULL,
            .mSet = esp8266_http_test_web_content_set_color,
        },
        {
            .mToken = "tra",
            .mGet = NULL,
            .mSet = esp8266_http_test_web_content_set_transition,
        },
        {
            .mToken = "trtime",
            .mGet = NULL,
            .mSet = esp8266_http_test_web_content_set_transition_time,
        }
    }
};

void esp8266_http_test(void * inParameters) {

    TaskHandle_t xHandle = NULL;
    BaseType_t lRetVal;

    {   /* measure 100 percent idle */
        uint32_t lBackupCounter;

        lBackupCounter = sLoadCounter;
        vTaskDelay(1000);
        s100percentIdle = sLoadCounter - lBackupCounter;
    }

    xTaskCreate(cpu_load_task, ( const char * )"cpu", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);

    vTaskDelay(10000);

    printf("Initialize user data\r\n");
    sUserData.mMutex = xSemaphoreCreateRecursiveMutex();
    if(!sUserData.mMutex) {
        printf("Mutex creation failed!\r\n");
    }

    printf("Initialize ESP8266... ");
    esp8266_init();
    printf("done\r\n");

    /* create rx task */
    lRetVal = xTaskCreate(esp8266_rx_task, ( const char * )"esp8266_rx", configMINIMAL_STACK_SIZE * 6, NULL, configMAX_PRIORITIES - 2, &xHandle);
    if(lRetVal) {
        printf("Successfully started RX Task %p\r\n", xHandle);
    } else {
        printf("Failed starting RX Task\r\n");
    }

    /* create socket task */
    lRetVal = xTaskCreate(esp8266_socket_task, ( const char * )"esp8266_so", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, &xHandle);
    if(lRetVal) {
        printf("Successfully started Socket Task %p\r\n", xHandle);
    } else {
        printf("Failed starting Socket Task\r\n");
    }

    vTaskDelay(1000);

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

    {   /* Set multiple connections */
        printf("Set multiple connections... ");
        if(esp8266_cmd_set_cipmux(true)) {
            printf("Success!\r\n");
        } else {
            printf("Failed!\r\n");
        }
    }

    {   /* set station mode */
        printf("Set WIFI Mode to %d... ", ESP8266_WIFI_MODE_AP /* ESP8266_WIFI_MODE_STA_AP */);
        if(esp8266_cmd_set_cwmode_cur(ESP8266_WIFI_MODE_AP /* ESP8266_WIFI_MODE_STA_AP */)) {
            printf("Success!\r\n");
        } else {
            printf("Failed!\r\n");
        }
    }

#if 0
    {   /* Join AP */
        uint8_t lSSID[] = "AndroidAP";
        uint8_t lPW[] = "cyvg3835";

        printf("Connecting to AP... ");
        if(esp8266_cmd_set_cwjap_cur(lSSID, sizeof(lSSID)-1, lPW, sizeof(lPW)-1)) {
            printf("Success!\r\n");
        } else {
            printf("Failed!\r\n");
        }
    }
#endif

    {   /* configure AP */
        uint8_t lSSIDBuffer[32];
        size_t  lSSIDBufferLen;

        uint8_t lPWDBuffer[64];
        size_t  lPWDBufferLen;

        lSSIDBufferLen = snprintf((char*)lSSIDBuffer, sizeof(lSSIDBuffer), "%s", "WP_AP");
        lPWDBufferLen  = snprintf((char*)lPWDBuffer,  sizeof(lPWDBuffer),  "%s", "deadbeef");

        printf("SSID: \"%s\"\r\n", lSSIDBuffer);
        printf("PASS: \"%s\"\r\n", lPWDBuffer);

        printf("Setting Access Point... ");
        if(esp8266_cmd_set_cwsap_cur(lSSIDBuffer, lSSIDBufferLen, lPWDBuffer, lPWDBufferLen, 11, ESP8266_ENC_MODE_WPA2_PSK)) {
            printf("Success!\r\n");
        } else {
            printf("Failed!\r\n");
        }
    }

    {   /* set web content handlers */
        printf("Set web content handlers\r\n");
        web_content_set_handlers(&sTestWebContent);
    }

    {   /* start http server */
        printf("Start HTTP server\r\n");
        esp8266_http_server_start();
    }

    /* delete this task */
    vTaskDelete(NULL);
}


/*
void esp8266_mqtt_message_arrived(MessageData* data) {

    printf("Message arrived on topic %.*s: %.*s\r\n",
        data->topicName->lenstring.len,
        data->topicName->lenstring.data,
        data->message->payloadlen,
        (char*)data->message->payload);
}
*/

static const char sOn[]  = { 'O', 'N' };
static const char sOff[] = { 'O', 'F', 'F' };

void esp8266_mqtt_blueled_arrived(MessageData* data) {

    printf("Message arrived on topic %.*s: %.*s\r\n",
        data->topicName->lenstring.len,
        data->topicName->lenstring.data,
        data->message->payloadlen,
        (char*)data->message->payload);

    if(data->message->payloadlen == sizeof(sOn) && memcmp(data->message->payload, sOn, data->message->payloadlen) == 0) {

        GPIO_SetBits(GPIOD, GPIO_Pin_15);
    } else if(data->message->payloadlen == sizeof(sOff) && memcmp(data->message->payload, sOff, data->message->payloadlen) == 0) {

        GPIO_ResetBits(GPIOD, GPIO_Pin_15);
    }
}

void esp8266_mqtt_task(void * inParameters) {

    TaskHandle_t xHandle = NULL;
    BaseType_t lRetVal;

    vTaskDelay(10000);

    printf("Initialize ESP8266... ");
    esp8266_init();
    printf("done\r\n");

    /* create rx task */
    lRetVal = xTaskCreate(esp8266_rx_task, ( const char * )"esp8266_rx", configMINIMAL_STACK_SIZE * 4, NULL, configMAX_PRIORITIES - 2, &xHandle);
    if(lRetVal) {
        printf("Successfully started RX Task\r\n");
    } else {
        printf("Failed starting RX Task\r\n");
    }

    /* create socket task */
    lRetVal = xTaskCreate(esp8266_socket_task, ( const char * )"esp8266_so", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, &xHandle);
    if(lRetVal) {
        printf("Successfully started Socket Task %p\r\n", xHandle);
    } else {
        printf("Failed starting Socket Task\r\n");
    }

    vTaskDelay(1000);

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

    {   /* set station mode */
        printf("Set WIFI Mode to %d... ", ESP8266_WIFI_MODE_STATION);
        if(esp8266_cmd_set_cwmode_cur(ESP8266_WIFI_MODE_STATION)) {
            printf("Success!\r\n");
        } else {
            printf("Failed!\r\n");
        }
    }

    {   /* connect to AP */
        uint8_t lSSID[] = "AndroidAP";
        uint8_t lPW[] = "cyvg3835";

        printf("Connecting to AP... ");
        if(esp8266_cmd_set_cwjap_cur(lSSID, sizeof(lSSID)-1, lPW, sizeof(lPW)-1)) {
            printf("Success!\r\n");
        } else {
            printf("Failed!\r\n");
        }
    }

    {   /* init blue led on PD15 */
        GPIO_InitTypeDef GPIO_InitStructure;

        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_15;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        GPIO_ResetBits(GPIOD, GPIO_Pin_15);
    }

    {   /* do mqtt */
        MQTTClient client;
        Network network;
        int rc = 0;
        size_t lCount;
        unsigned char sendbuf[80], readbuf[80];
        char* address = "iot.eclipse.org";

        MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

        NetworkInit(&network);
        MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

        if ((rc = NetworkConnect(&network, address, 1883)) != 0) {
            printf("Return code from network connect is %d\r\n", rc);
        }

#if defined(MQTT_TASK)
        if ((rc = MQTTStartTask(&client)) != pdPASS) {
            printf("Return code from start tasks is %d\n\n", rc);
        }
#endif

        connectData.MQTTVersion = 3;
        connectData.clientID.cstring = "FreeRTOS_sample";

        if ((rc = MQTTConnect(&client, &connectData)) != 0) {
            printf("Return code from MQTT connect is %d\r\n", rc);
        }
        else {
            printf("MQTT Connected\r\n");
        }

/*
        if((rc = MQTTSubscribe(&client, "FreeRTOS/sample/wep/cnt", 2, esp8266_mqtt_message_arrived)) != 0) {
            printf("Return code from MQTT subscribe is %d\r\n", rc);
        }
*/
        if((rc = MQTTSubscribe(&client, "FreeRTOS/sample/wep/blueled", 2, esp8266_mqtt_blueled_arrived)) != 0) {
            printf("Return code from MQTT subscribe is %d\r\n", rc);
        }

        for(lCount = 0; ; lCount++) {
/*
            MQTTMessage message;
            char payload[30];

            message.qos = 1;
            message.retained = 0;
            message.payload = payload;
            message.payloadlen = snprintf(payload, sizeof(payload), "%d", lCount);

            if ((rc = MQTTPublish(&client, "FreeRTOS/sample/wep/cnt", &message)) != 0) {
                printf("Return code from MQTT publish is %d\r\n", rc);
            }
*/

#if !defined(MQTT_TASK)
            if ((rc = MQTTYield(&client, 100)) != 0) {
                printf("Return code from yield is %d\r\n", rc);
            }
#endif
//            vTaskDelay(1000);
        }
    }
}

int main(void) {

    init();

    {   /* create tasks */
        xTaskCreate(led_task,          ( const char * )"led",          configMINIMAL_STACK_SIZE *  8, NULL, configMAX_PRIORITIES - 2, NULL);
//        xTaskCreate(esp8266_mqtt_task, ( const char * )"esp8266_mqtt", configMINIMAL_STACK_SIZE * 32, NULL, configMAX_PRIORITIES - 3, NULL);
        xTaskCreate(esp8266_http_test, ( const char * )"http",         configMINIMAL_STACK_SIZE * 32, NULL, configMAX_PRIORITIES - 3, NULL);
//        xTaskCreate(esp8266_task,      ( const char * )"esp8266",    configMINIMAL_STACK_SIZE * 32, NULL, configMAX_PRIORITIES - 3, NULL);
//        xTaskCreate(esp8266_test_task, ( const char * )"test",       configMINIMAL_STACK_SIZE * 8, NULL, configMAX_PRIORITIES - 4, NULL);

        /* Start the scheduler. */
        vTaskStartScheduler();
    }

    return 0;
}

/* eof */
