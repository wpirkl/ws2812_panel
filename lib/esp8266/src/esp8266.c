
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

#include "stm32f4xx_conf.h"     /* for assert_param */

#include "esp8266.h"
#include "uart_dma.h"


#define ESP8266_MAX_CONNECTIONS         (5)
#define ESP8266_MAX_RESPONSE_SIZE       (128)
#define ESP8266_MAX_CHANNEL_RCV_SIZE    (2048)
#define ESP8266_MAX_CHANNEL_RCV_CNT     (4)

#define ESP8266_FREERTOS

#if defined(ESP8266_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#endif /* ESP8266_FREERTOS */



typedef struct {
    /*! Receive buffer */
    uint8_t mBuffer[ESP8266_MAX_CHANNEL_RCV_SIZE];
    /*! Lenght of the data in the buffer */
    size_t  mLength;
    /*! Last index of read */
    size_t  mIndex;
} ts_rx_packet;

struct s_esp8266_socket {
#if defined(ESP8266_FREERTOS)
    /*! Semaphore to notify on reception */
    SemaphoreHandle_t mRxSemaphore;
#endif /* ESP8266_FREERTOS */

    /*! Head index into receive buffer ring */
    size_t          mHead;

    /*! Tail index into receive buffer ring */
    size_t          mTail;

    /*! Receive data ring buffer */
    ts_rx_packet    mRxBuffer[ESP8266_MAX_CHANNEL_RCV_CNT];
};

typedef struct {
#if defined(ESP8266_FREERTOS)
    /*! Semaphore to notify caller */
    SemaphoreHandle_t           mResponseSema;

    /*! Mutex to allow only one caller at a time */
    SemaphoreHandle_t           mMutex;
#endif /* ESP8266_FREERTOS */

    /*! Sentinel String */
    uint8_t                   * mSentinel;

    /*! Length of the sentinel */
    size_t                      mSentinelLen;

    /*! Response Buffer */
    uint8_t                   * mResponseBuffer;

    /*! Maximum length of the response buffer */
    size_t                      mResponseLen;

    /*! Actual receive length */
    size_t                      mResponseRcvLen;

    struct s_esp8266_socket     mSockets[ESP8266_MAX_CONNECTIONS];
} ts_esp8266;

/*! Local object */
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


void esp8266_rx_handler(void) {

#if defined(ESP8266_FREERTOS)
    for(;usart_dma_rx_num() == 0;) {
        /* only wait if there's no data */
        usart_dma_rx_wait();
    }
#endif /* ESP8266_FREERTOS */

    /* if it starts with +IPD,x,y: */
    if(usart_dma_peek((uint8_t*)"\r\n+IPD,", 7)) {

        size_t lCount;
        size_t lChannel;
        size_t lRcvLen;
        size_t lLen;

        uint8_t * lPtr;

        uint8_t lNumberBuffer[12];

        /* we already know it start's with \r\n+IPD, */
        usart_dma_rx_skip(7);

        /* decode channel */
        lLen = usart_dma_read_until(&lNumberBuffer[0], sizeof(lNumberBuffer)-1, ',');
        lNumberBuffer[lLen - 1] = '\0';

        lChannel = strtoul((char*)lNumberBuffer, NULL, 0);

        assert_param(lChannel < ESP8266_MAX_CONNECTIONS);

        /* get data size */
        lLen = usart_dma_read_until(&lNumberBuffer[0], sizeof(lNumberBuffer)-1, ':');
        lNumberBuffer[lLen - 1] = '\0';

        lRcvLen = strtoul((char*)lNumberBuffer, NULL, 0);

        assert_param(lRcvLen < ESP8266_MAX_CHANNEL_RCV_SIZE);

        lPtr = &sEsp8266.mSockets[lChannel].mRxBuffer[sEsp8266.mSockets[lChannel].mHead].mBuffer[0];
        sEsp8266.mSockets[lChannel].mRxBuffer[sEsp8266.mSockets[lChannel].mHead].mLength = lRcvLen;
        sEsp8266.mSockets[lChannel].mRxBuffer[sEsp8266.mSockets[lChannel].mHead].mIndex = 0;

        /* receive lRcvLen bytes from the device */
        for(lCount = 0; lCount < lRcvLen;) {
#if defined(ESP8266_FREERTOS)
            if(usart_dma_rx_num() == 0) {
                /* wait if there's not enough data */
                usart_dma_rx_wait();
            }
#endif /* ESP8266_FREERTOS */
            lLen = usart_dma_read(&lPtr[lCount], lRcvLen - lCount);
            lCount += lLen;
        }

        /* increment receive buffer ring */
        sEsp8266.mSockets[lChannel].mHead = (sEsp8266.mSockets[lChannel].mHead + 1) & (ESP8266_MAX_CHANNEL_RCV_CNT - 1);

        /* notify socket */
        xSemaphoreGive(sEsp8266.mSockets[lChannel].mRxSemaphore);

/*  } else if( usart_dma_peek(...)) { */    /* handle further asynchronous messages */

    } else if(sEsp8266.mSentinel != NULL && sEsp8266.mSentinelLen > 0) {
        /* received command response */

        size_t lSize = usart_dma_match(sEsp8266.mSentinel, sEsp8266.mSentinelLen);
        if(lSize > 0) {
            /* the sentinel was found in the buffer */

            if(sEsp8266.mResponseBuffer == NULL || sEsp8266.mResponseLen == 0) {

                /* nobody want's the data, so drop it */
                usart_dma_rx_skip(lSize);
            } else {

                /* assure the caller forsees enough memory */
                assert_param(lSize <= sEsp8266.mResponseLen);

                /* lSize <= sEsp8266.mResponseLen */
                sEsp8266.mResponseRcvLen = usart_dma_read(sEsp8266.mResponseBuffer, lSize);
            }

            /* notify caller */
            xSemaphoreGive(sEsp8266.mResponseSema);
        }

    } else {

        /* nobody asked to receive something or not enough data */
    }
    
}


void esp8266_setup(void) {
    
    
}

/* eof */
