

#include "FreeRTOS.h"
#include "task.h"

#include "init.h"

#include "ws2812.h"
#include "ws2812_anim.h"

#include "esp8266.h"
#include "esp8266_http_server.h"

#include "web_content_handler.h"

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

    for(;;) {

        lLastCounter = sLoadCounter;

        vTaskDelay(1000);

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

/*! ESP8266 Receive Task

    This task handles asynchronous socket requests from the esp8266 module

    \param[in]  inParameters    Unused
*/
static void esp8266_socket_task(void * inParameters) {

    for(;;) {
        esp8266_socket_handler();
    }
}

/*! MQTT client task

    This task periodically checks if we're connectd to the mqtt server

    \param[in]  inParameters    Unused
*/
static void esp8266_mqtt_task(void * inParameters) {

    for(;;) {

        /* replace this by code */
        vTaskDelay(1000);
    }
}

/*! Configure the esp8266 module

    This function configures the esp8266 module
*/
static void esp8266_configure(void) {

    /* give a bit of time to start all the tasks */
    vTaskDelay(100);
}

/*! Initialize the web server

    This function initializes the http server
*/
static void init_http(void) {


    esp8266_http_server_start();
}

/*! Initialization task

    This task initializes all other tasks

    \param[in]  inParameters    Unused
*/
static void init_task(void * inParameters) {

    TaskHandle_t xHandle = NULL;

    /* init ws2812 library */
    ws2812_init();
    ws2812_animation_init();

    /* start led task */
    if(!xTaskCreate(ws2812_anim_task, "ws2812_anim", configMINIMAL_STACK_SIZE * 8, NULL, configMAX_PRIORITIES - 2, NULL)) {
        /* check how we can handle errors */
    }

    /* init esp8266 library */
    esp8266_init();

    /* start socket task */
    if(!xTaskCreate(esp8266_socket_task, "esp8266_so", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, &xHandle)) {
        /* check how we can handle errors */
    }

    /* start rx task */
    if(!xTaskCreate(esp8266_rx_task, "esp8266_rx", configMINIMAL_STACK_SIZE * 6, NULL, configMAX_PRIORITIES - 3, &xHandle)) {
        /* check how we can handle errors */
    }

    /* configure the esp8266 module */
    esp8266_configure();

    /* setup web server */
    init_http();

    /* start mqtt client task */
    if(!xTaskCreate(esp8266_mqtt_task, ( const char * )"esp8266_mqtt", configMINIMAL_STACK_SIZE * 32, NULL, configMAX_PRIORITIES - 4, &xHandle)) {
        /* check how we can handle errors */
    }


    /* delete this task */
    vTaskDelete(NULL);
}



int main(void) {

    init();

    /* create highest priority init task */
    xTaskCreate(init_task, "init", configMINIMAL_STACK_SIZE *  2, NULL, configMAX_PRIORITIES - 1, NULL);

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
