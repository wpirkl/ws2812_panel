
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "task_priorities.h"

#include "init.h"

#include "ap_config.h"

#include "ws2812.h"
#include "ws2812_anim.h"

#include "esp8266.h"
#include "esp8266_http_server.h"

#include "web_content_handler.h"
#include "main_web_content_handler.h"

#include "MQTTClient.h"


/* place heap into ccm */
uint8_t __attribute__ ((section(".ccmbss"), aligned(8))) ucHeap[ configTOTAL_HEAP_SIZE ];


static volatile uint32_t s100percentIdle = 0;
static volatile uint32_t sLoadCounter = 0;
static volatile uint32_t sCurrentLoad = 0;

void vApplicationIdleHook( void ) {

    sLoadCounter++;
}

void cpu_load_task(void * inParameters) {

    uint32_t lLastCounter;
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount();

    for(;;) {

        lLastCounter = sLoadCounter;

        vTaskDelayUntil(&xLastWakeTime, 1000);

        sCurrentLoad = sLoadCounter - lLastCounter;
    }
}


/*! LED task

    This task runs the ws2812 animation

    \param[in]  inParameters    Unused
*/
static void ws2812_anim_task(void * inParameters) {

    for(;;) {
        ws2812_animation_main();
    }
}

/*! ESP8266 Receive Task

    This task handles asynchronous responses from the esp8266 module

    \param[in]  inParameters    Unused
*/
static void esp8266_rx_task(void * inParameters) {

    for(;;) {
        esp8266_rx_handler();
    }
}

/*! ESP8266 Socket Task

    This task handles asynchronous socket requests from the esp8266 module

    \param[in]  inParameters    Unused
*/
static void esp8266_socket_task(void * inParameters) {

    for(;;) {
        esp8266_socket_handler();
    }
}


/*! ESP8266 Wifi task

    This task handles asynchronous wifi requests from the esp8266 module

    \param[in]  inParameters    Unused
*/
void esp8266_wifi_task(void * inParameters) {

    for(;;) {
        esp8266_wifi_connection_handler();
    }
}


/*! MQTT client task

    This task periodically checks if we're connectd to the mqtt server

    \param[in]  inParameters    Unused
*/
//static void esp8266_mqtt_task(void * inParameters) {

//    for(;;) {

        /* replace this by code */
//        vTaskDelay(1000);
//    }
//}

/*! Configure the esp8266 module

    This function configures the esp8266 module
*/
static void esp8266_configure(void) {

    /* reset */
    if(!esp8266_cmd_rst()) {
        /* check how we can handle errors */
    }

    /* echo off */
    if(!esp8266_cmd_ate0()) {
        /* check how we can handle errors */
    }

    /* set multiple connections */
    if(!esp8266_cmd_set_cipmux(true)) {
        /* check how we can handle errors */
    }

    /* set station + AP mode */
    if(!esp8266_cmd_set_cwmode_cur(/*ESP8266_WIFI_MODE_AP */ ESP8266_WIFI_MODE_STA_AP)) {
        /* check how we can handle errors */
    }

    /* configure AP */
    if(g_AP_SSID && g_AP_PWD) {
        if(!esp8266_cmd_set_cwsap_cur((uint8_t*)g_AP_SSID, strlen(g_AP_SSID), (uint8_t*)g_AP_PWD, strlen(g_AP_PWD), g_AP_CHAN, g_AP_ENC_MODE)) {
            /* check how we can handle errors */
        }
    } else {

        const char * l_AP_SSID = "WP_AP";
        const char * l_AP_PWD = "deadbeef";

        if(!esp8266_cmd_set_cwsap_cur((uint8_t*)l_AP_SSID, strlen(l_AP_SSID), (uint8_t*)l_AP_PWD, strlen(l_AP_PWD), 11, ESP8266_ENC_MODE_WPA_WPA2_PSK)) {
            /* check how we can handle errors */
        }
    }
}

/*! Initialization task

    This task initializes all other tasks

    \param[in]  inParameters    Unused
*/
static void init_task(void * inParameters) {

    TaskHandle_t xHandle = NULL;

    {   /* measure 100 percent idle */
        uint32_t lBackupCounter;

        lBackupCounter = sLoadCounter;
        vTaskDelay(1000);
        s100percentIdle = sLoadCounter - lBackupCounter;
    }

    /* init ws2812 library */
    ws2812_init();

    /* init animation */
    ws2812_animation_init();

    if(!xTaskCreate(cpu_load_task, ( const char * )"cpu", configMINIMAL_STACK_SIZE, NULL, CPU_LOAD_TASK_PRIORITY, &xHandle)) {
        /* check how we can handle errors */
    }

    /* start led task */
    if(!xTaskCreate(ws2812_anim_task, "ws2812_anim", configMINIMAL_STACK_SIZE * 8, NULL, LED_TASK_PRIORITY, &xHandle)) {
        /* check how we can handle errors */
    }

    /* init esp8266 library */
    esp8266_init();

    /* start socket task */
    if(!xTaskCreate(esp8266_socket_task, "esp8266_so", configMINIMAL_STACK_SIZE, NULL, ESP8266_SOCKET_TASK_PRIORITY, &xHandle)) {
        /* check how we can handle errors */
    }

    if(!xTaskCreate(esp8266_wifi_task, "esp8266_wi", configMINIMAL_STACK_SIZE * 2, NULL, ESP8266_WIFI_TASK_PRIORITY, &xHandle)) {
        /* check how we can handle errors */
    }

    /* start rx task */
    if(!xTaskCreate(esp8266_rx_task, "esp8266_rx", configMINIMAL_STACK_SIZE * 6, NULL, ESP8266_RX_TASK_PRIORITY, &xHandle)) {
        /* check how we can handle errors */
    }


    /* configure the esp8266 module */
    esp8266_configure();

    /* initialize web server user data */
    if(!main_web_content_init()) {
        /* check how we can handle errors */
    }

    /* start web server */
    esp8266_http_server_start(ESP8266_HTTP_SERVER_PRIORITY);

    /* start mqtt client task */
    // if(!xTaskCreate(esp8266_mqtt_task, ( const char * )"esp8266_mqtt", configMINIMAL_STACK_SIZE * 32, NULL, ESP8266_MQTT_TASK_PRIORITY, &xHandle)) {
        /* check how we can handle errors */
    // }


    /* delete this task */
    vTaskDelete(NULL);
}



int main(void) {

    init();

    /* create highest priority init task */
    xTaskCreate(init_task, "init", configMINIMAL_STACK_SIZE *  2, NULL, INIT_TASK_PRIORITY, NULL);

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* if we come here, we're dead */
    return 0;
}

/* --------- for development ------------- */

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName) {

    printf("%s(%d): Stack overflow on task %p \"%s\"!\r\n", __FILE__, __LINE__, xTask, pcTaskName);
}

void vApplicationMallocFailedHook( void ) {

    printf("%s(%d): Malloc failed active task: %p\r\n", __FILE__, __LINE__, xTaskGetCurrentTaskHandle());
}

/* eof */
