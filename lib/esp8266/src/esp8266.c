
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>

#include "stm32f4xx_conf.h"     /* for assert_param */

#include "esp8266.h"
#include "uart_dma.h"


#define ESP8266_MAX_CONNECTIONS         (5)
#define ESP8266_MAX_RESPONSE_SIZE       (128)
#define ESP8266_MAX_CHANNEL_RCV_SIZE    (2048)

#define ESP8266_FREERTOS

#if defined(ESP8266_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#endif /* ESP8266_FREERTOS */


static void esp8266_rx_task(void * pvTaskParameters);
/*
static uint8_t * esp8266_ltrim(uint8_t * inString);
*/

struct s_esp8266_socket {
#if defined(ESP8266_FREERTOS)
    SemaphoreHandle_t mRxSemaphore;
#else   /* ESP8266_FREERTOS */
#endif /* ESP8266_FREERTOS */
    uint8_t mRxBuffer[ESP8266_MAX_CHANNEL_RCV_SIZE];
    size_t  mRxHead;
    size_t  mRxTail;
};


typedef struct {
#if defined(ESP8266_FREERTOS)
    SemaphoreHandle_t           mResponseSema;
    SemaphoreHandle_t           mMutex;
#else /* ESP8266_FREERTOS */
#endif /* ESP8266_FREERTOS */
    uint8_t                   * mResponseTail;
    size_t                      mResponseTailLen;
    uint8_t                   * mResponseBuffer;
    size_t                      mResponseBufferLength;
    size_t                      mAwaitingResponseLength;
    struct s_esp8266_socket     mSockets[ESP8266_MAX_CONNECTIONS];
} ts_esp8266;


/* Local object */
static ts_esp8266               sEsp8266;

void esp8266_init(void) {

    usart_dma_open();

#if defined(ESP8266_FREERTOS)
    {
        size_t lCount;

        sEsp8266.mResponseSema = xSemaphoreCreateCounting(1, 0);
        assert_param(sEsp8266.mResponseSema != NULL);

        sEsp8266.mMutex = xSemaphoreCreateRecursiveMutex();
        assert_param(sEsp8266.mMutex != NULL);

        for(lCount = 0; lCount < ESP8266_MAX_CONNECTIONS; lCount++) {
            sEsp8266.mSockets[lCount].mRxSemaphore = xSemaphoreCreateCounting(1, 0);
            assert_param(sEsp8266.mSockets[lCount].mRxSemaphore != NULL);
        }
    }
#else /* ESP8266_FREERTOS */
#endif /* ESP8266_FREERTOS */
}

#if defined(ESP8266_FREERTOS)
static void esp8266_rx_task(void * pvTaskParameters) {

    for(;;) {
        esp8266_rx_handler();
    }
}
#endif /* ESP8266_FREERTOS */


void esp8266_rx_handler(void) {

    size_t lLen;
    uint8_t * lPtr;

#if defined(ESP8266_FREERTOS)
    for(;usart_dma_rx_num() == 0;) {
        usart_dma_rx_wait();
    }
#endif /* ESP8266_FREERTOS */

    /* if it starts with +IPD,x,y: */
    if(usart_dma_peek("\r\n+IPD,", 7)) {

        /* we already know it start's with \r\n+IPD, */
        usart_dma_rx_skip(7);

        /* handle receive */
        

        /* notify socket */
        

/*  } else if( usart_dma_peek(...)) { */    /* handle further asynchronous messages */
    } else {

        /* read until sentinel string */

        /* notify response handler */
    }
    
}

/*
static uint8_t * esp8266_ltrim(uint8_t * inString) {

    uint8_t * lPtr;

    for(lPtr = inString; isspace(*lPtr); lPtr++);

    return lPtr;
}
*/



/* eof */
