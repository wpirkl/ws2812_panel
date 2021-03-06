#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "stm32f4xx_conf.h"     /* for assert_param */

#include "esp8266.h"
#include "uart_dma.h"

#define dbg_on(x...)      printf(x)
#define dbg_off(x...)

#define dbg dbg_off


#define dbg_err_on(x...)      printf(x)
#define dbg_err_off(x...)

#define dbg_err dbg_err_on


#define STATIC static

#define ESP8266_MAX_CONNECTIONS         (5)
#define ESP8266_MAX_RESPONSE_SIZE       (128)
#define ESP8266_MAX_CHANNEL_RCV_SIZE    (1460)
#define ESP8266_MAX_CHANNEL_RCV_CNT     (4)
#define ESP8266_MAX_SEND_LEN            (2048)

#define ESP8266_FREERTOS

#if defined(ESP8266_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#endif /* ESP8266_FREERTOS */

/* WEP for testing */
// #undef portMAX_DELAY
// #define portMAX_DELAY (10000 / portTICK_PERIOD_MS)


const uint8_t sCRLF[]   = { '\r', '\n' };

/* General commands */
const uint8_t sAT[]     = { 'A', 'T' };
const uint8_t sATE0[]   = { 'A', 'T', 'E', '0' };
const uint8_t sAT_RST[] = { 'A', 'T', '+', 'R', 'S', 'T' };
const uint8_t sAT_GMR[] = { 'A', 'T', '+', 'G', 'M', 'R' };

/* Wifi commands */
const uint8_t sAT_CWMODE[] = { 'A', 'T', '+', 'C', 'W', 'M', 'O', 'D', 'E' };
const uint8_t sAT_CWJAP[]  = { 'A', 'T', '+', 'C', 'W', 'J', 'A', 'P' };
const uint8_t sAT_CWLAP[]  = { 'A', 'T', '+', 'C', 'W', 'L', 'A', 'P' };
const uint8_t sAT_CWQAP[]  = { 'A', 'T', '+', 'C', 'W', 'Q', 'A', 'P' };
const uint8_t sAT_CWSAP[]  = { 'A', 'T', '+', 'C', 'W', 'S', 'A', 'P' };
const uint8_t sAT_CWLIF[]  = { 'A', 'T', '+', 'C', 'W', 'L', 'I', 'F' };
const uint8_t sAT_CWDHCP[] = { 'A', 'T', '+', 'C', 'W', 'D', 'H', 'C', 'P' };

const uint8_t sCUR[] = { '_', 'C', 'U', 'R' };

/* TCP/UDP commands */
const uint8_t sAT_CIPMUX[]    = { 'A', 'T', '+', 'C', 'I', 'P', 'M', 'U', 'X' };
const uint8_t sAT_CIPSTART[]  = { 'A', 'T', '+', 'C', 'I', 'P', 'S', 'T', 'A', 'R', 'T' };
const uint8_t sAT_CIPCLOSE[]  = { 'A', 'T', '+', 'C', 'I', 'P', 'C', 'L', 'O', 'S', 'E' };
const uint8_t sAT_CIPSEND[]   = { 'A', 'T', '+', 'C', 'I', 'P', 'S', 'E', 'N', 'D' };
const uint8_t sAT_CIPSERVER[] = { 'A', 'T', '+', 'C', 'I', 'P', 'S', 'E', 'R', 'V', 'E', 'R' };
const uint8_t sAT_CIPSTA[]    = { 'A', 'T', '+', 'C', 'I', 'P', 'S', 'T', 'A' };

const uint8_t sAT_PING[] = { 'A', 'T', '+', 'P', 'I', 'N', 'G' };

const uint8_t sOK[]        = { '\r', '\n', 'O', 'K', '\r', '\n'};
const uint8_t sERROR[]     = { '\r', '\n', 'E', 'R', 'R', 'O', 'R', '\r', '\n' };
const uint8_t sFAIL[]      = { '\r', '\n', 'F', 'A', 'I', 'L', '\r', '\n' };
const uint8_t sREADY[]     = { 'r', 'e', 'a', 'd', 'y', '\r', '\n' };
const uint8_t sTCP[]       = { 'T', 'C', 'P' };
const uint8_t sOK_SEND[]   = { '\r', '\n', 'O', 'K', '\r', '\n', '>', ' ' };
const uint8_t sSEND_OK[]   = { '\r', '\n', 'S', 'E', 'N', 'D', ' ', 'O', 'K', '\r', '\n' };
const uint8_t sSEND_FAIL[] = { 'S', 'E', 'N', 'D', ' ', 'F', 'A', 'I', 'L', '\r', '\n' };
const uint8_t sBUSY_S[]    = { '\r', '\n', 'b', 'u', 's', 'y', ' ', 's', '.', '.', '.', '\r', '\n' };

const uint8_t sALREADY_CON[] = { 'A', 'L', 'R', 'E', 'A', 'D', 'Y', ' ', 'C', 'O', 'N', 'N', 'E', 'C', 'T', 'E', 'D', '\r', '\n' };

const uint8_t sIPD[]          = { '\r', '\n', '+', 'I', 'P', 'D', ',' };
const uint8_t sCONNECT[]      = { ',', 'C', 'O', 'N', 'N', 'E', 'C', 'T', '\r', '\n' };
const uint8_t sCLOSED[]       = { ',', 'C', 'L', 'O', 'S', 'E', 'D', '\r', '\n' };
const uint8_t sCONNECT_FAIL[] = { ',', 'C', 'O', 'N', 'N', 'E', 'C', 'T', ' ', 'F', 'A', 'I', 'L', '\r', '\n' };

const uint8_t sWIFI_DIS[] = { 'W', 'I', 'F', 'I', ' ', 'D', 'I', 'S', 'C', 'O', 'N', 'N', 'E', 'C', 'T', '\r', '\n' };
const uint8_t sWIFI_CON[] = { 'W', 'I', 'F', 'I', ' ', 'C', 'O', 'N', 'N', 'E', 'C', 'T', 'E', 'D', '\r', '\n' };
const uint8_t sWIFI_IP[]  = { 'W', 'I', 'F', 'I', ' ', 'G', 'O', 'T', ' ', 'I', 'P', '\r', '\n' };

/* forward declarations */

STATIC void esp8266_set_sentinel(const uint8_t * const inSentinel, size_t inSentinelLen, const uint8_t * const inErrorSentinel, size_t inErrorSentinelLen);


/*! Defines a rx packet
*/
typedef struct s_rx_packet {
    /*! Receive buffer */
    uint8_t mBuffer[ESP8266_MAX_CHANNEL_RCV_SIZE];
    /*! Lenght of the data in the buffer */
    size_t  mLength;
    /*! Last index of read */
    size_t  mIndex;
} ts_rx_packet;

/*! Defines a socket object
*/
struct s_esp8266_socket {
#if defined(ESP8266_FREERTOS)
    /*! Semaphore to notify on reception */
    SemaphoreHandle_t mRxSemaphore;

    /*! Mutex which handles socket access */
    SemaphoreHandle_t mMutex;
#endif /* ESP8266_FREERTOS */

    /*! Defines the esp8622 socket id */
    uint8_t                 mSocketId;

    /*! Defines if this socket is in use */
    bool                    mUsed;

    /*! Defines the socket timeout in ms */
    uint32_t                mTimeout;

    /*! Head index into receive buffer ring */
    volatile size_t         mHead;

    /*! Tail index into receive buffer ring */
    volatile size_t         mTail;

    /*! Server Task handle */
    volatile TaskHandle_t   mTaskHandle;

    /*! Receive data ring buffer */
    ts_rx_packet            mRxBuffer[ESP8266_MAX_CHANNEL_RCV_CNT];
};

/*! Defines a socket command */
typedef enum e_esp8266_socket_cmd {
    ESP8266_SOCKET_OPEN,
    ESP8266_SOCKET_CLOSE,
    ESP8266_SOCKET_CONNECT_FAIL,
} te_esp8266_socket_cmd;

/*! Defines a socket command object */
typedef struct s_esp8266_socket_cmd {

    /*! The command to execute */
    te_esp8266_socket_cmd   mCommand;

    /*! The socket ID */
    size_t                  mSocket;

} ts_esp8266_socket_cmd;

typedef enum e_esp8266_wifi_cmd {
    ESP8266_WIFI_CONNECTED,
    ESP8266_WIFI_GOT_IP,
    ESP8266_WIFI_DISCONNECT,
} te_esp8266_wifi_cmd;

/*! Defines a wifi command object */
typedef struct s_esp8266_wifi_cmd {

    /*! The command to execute */
    te_esp8266_wifi_cmd     mCommand;

} ts_esp8266_wifi_cmd;

/*! Defines the esp8266 object
*/
typedef struct s_esp8266 {
#if defined(ESP8266_FREERTOS)
    /*! Semaphore to notify caller */
    SemaphoreHandle_t           mResponseSema;

    /*! Semaphore to notify close */
    SemaphoreHandle_t           mCloseNotify;

    /*! Socket command queue */
    QueueHandle_t               mSocketCmdQ;

    /*! Wifi command queue */
    QueueHandle_t               mWIFICmdQ;

    /*! Mutex to allow only one caller at a time -- I/O related */
    SemaphoreHandle_t           mMutex;

#endif /* ESP8266_FREERTOS */

    /*! Number of treated characters in the rx buffer */
    size_t                      mTreated;

    /*! Forced refresh */
    bool                        mForcedRefresh;

    /*! Sentinel String */
    const uint8_t             * mSentinel;

    /*! Length of the sentinel */
    size_t                      mSentinelLen;

    /*! Error Sentinel String */
    const uint8_t             * mErrorSentinel;

    /*! Length of the error sentinel string */
    size_t                      mErrorSentinelLen;

    /*! Response Buffer */
    uint8_t                   * mResponseBuffer;

    /*! Maximum length of the response buffer */
    size_t                      mResponseLen;

    /*! Actual receive length */
    size_t                      mResponseRcvLen;

    /*! Return value */
    te_esp8266_cmd_ret          mStatus;

    /*! Multiple TCP connections */
    bool                        mMultipleConnections;

    /*! Wifi Mode */
    te_esp8266_wifi_mode        mWifiMode;

    /*! Server handler */
    t_esp8266_server_handler    mServerHandler;

    /*! Server Task Priority */
    uint32_t                    mServerTaskPriority;

    /*! Server Task Stack Size */
    uint32_t                    mServerTaskStackSize;

    /*! Socket objects */
    struct s_esp8266_socket     mSockets[ESP8266_MAX_CONNECTIONS];
} ts_esp8266;

/*! Local object */
STATIC ts_esp8266               sEsp8266;


/*  ---- functions ---- */

void esp8266_init(void) {

    usart_dma_open();

    /* reset the object */
    memset(&sEsp8266, 0, sizeof(sEsp8266));

#if defined(ESP8266_FREERTOS)
    {
        size_t lCount;

        sEsp8266.mResponseSema = xSemaphoreCreateCounting(1, 0);
        if(!sEsp8266.mResponseSema) {
            dbg_err("%s(%d): sEsp8266.mResponseSema is NULL\r\n", __FILE__, __LINE__);
        }

        sEsp8266.mCloseNotify = xSemaphoreCreateCounting(1, 0);
        if(!sEsp8266.mCloseNotify) {
            dbg_err("%s(%d): sEsp8266.mCloseNotify is NULL\r\n", __FILE__, __LINE__);
        }

        sEsp8266.mMutex = xSemaphoreCreateRecursiveMutex();
        if(!sEsp8266.mMutex) {
            dbg_err("%s(%d): sEsp8266.mResponseSema is NULL\r\n", __FILE__, __LINE__);
        }

        /* We are able to store a CONNECT and CLOSE command per socket */
        sEsp8266.mSocketCmdQ = xQueueCreate(ESP8266_MAX_CONNECTIONS * 2, sizeof(ts_esp8266_socket_cmd));
        if(!sEsp8266.mSocketCmdQ) {
            dbg_err("%s(%d): sEsp8266.mSocketCmdQ is NULL\r\n", __FILE__, __LINE__);
        }

        /* 3 commands possibls */
        sEsp8266.mWIFICmdQ = xQueueCreate(4, sizeof(ts_esp8266_wifi_cmd));
        if(!sEsp8266.mWIFICmdQ) {
            dbg_err("%s(%d): sEsp8266.mWIFICmdQ is NULL\r\n", __FILE__, __LINE__);
        }

        for(lCount = 0; lCount < ESP8266_MAX_CONNECTIONS; lCount++) {

            sEsp8266.mSockets[lCount].mSocketId = lCount;
            sEsp8266.mSockets[lCount].mTimeout  = 1000;

            sEsp8266.mSockets[lCount].mRxSemaphore = xSemaphoreCreateCounting(1, 0);
            if(!sEsp8266.mSockets[lCount].mRxSemaphore) {
                dbg_err("%s(%d): sEsp8266.mSockets[%d].mRxSemaphore is NULL\r\n", __FILE__, __LINE__, lCount);
            }
            

            sEsp8266.mSockets[lCount].mMutex = xSemaphoreCreateRecursiveMutex();
            if(!sEsp8266.mSockets[lCount].mMutex) {
                dbg_err("%s(%d): sEsp8266.mSockets[%d].mMutex is NULL\r\n", __FILE__, __LINE__, lCount);
            }
        }

        /* just above idle task priority */
        sEsp8266.mServerTaskPriority = 1;
        sEsp8266.mServerTaskStackSize = configMINIMAL_STACK_SIZE * 4;
    }
#else /* ESP8266_FREERTOS */
#endif /* ESP8266_FREERTOS */
}

/*! Server Task

    \param[in]  inParameters    Socket ID
*/
STATIC void esp8266_server_handler_task(void * inParameters) {

    uintptr_t lSocketID = (uintptr_t)inParameters;
    ts_esp8266_socket * lSocket;

    dbg("Socket ID: %d\r\n", lSocketID);

    if(lSocketID >= ESP8266_MAX_CONNECTIONS) {
        dbg_err("%s(%d): lSocketID (%d) >= ESP8266_MAX_CONNECTIONS(%d)\r\n", __FILE__, __LINE__, lSocketID, ESP8266_MAX_CONNECTIONS);
    }

    lSocket = &sEsp8266.mSockets[lSocketID];

    /* if there's a handler installed, call the handler */
    if(sEsp8266.mServerHandler) {
        sEsp8266.mServerHandler(lSocket);
    }

    dbg("Closing Socket... ");
    if(esp8266_cmd_cipclose(lSocket)) {
        dbg("Success!\r\n");
    } else {
        dbg("Failed\r\n");
    }

    dbg("Task Delete\r\n");
    lSocket->mTaskHandle = NULL;

    /* notify rx task which could wait for the close */
    xSemaphoreGive(sEsp8266.mCloseNotify);

    /* delete this task */
    vTaskDelete(NULL);
}

/*! Cleanup socket

    This function requires mutex protection

    \param[in]  inSocket    Socket to clean-up
*/
STATIC void esp8266_socket_cleanup(ts_esp8266_socket * inSocket) {

    size_t lCount;

    inSocket->mUsed = false;
    inSocket->mHead = 0;
    inSocket->mTail = 0;

    for(lCount = 0; lCount < ESP8266_MAX_CHANNEL_RCV_CNT; lCount++) {
        inSocket->mRxBuffer[lCount].mLength = 0;
        inSocket->mRxBuffer[lCount].mIndex = 0;
    }
}

/*! This function handles asynchronous incoming data
*/
STATIC bool esp8266_rx_handle_ipd(void) {

    size_t lCount;
    size_t lChannel;
    size_t lRcvLen;
    size_t lLen;

    uint8_t * lPtr;

    uint8_t lNumberBuffer[12];

    if(usart_dma_peek(sIPD, sizeof(sIPD))) {

        /* we already know it start's with \r\n+IPD, */
        usart_dma_rx_skip(sizeof(sIPD));

        if(sEsp8266.mMultipleConnections) {
            /* decode channel */
            uint8_t lTmp;
            usart_dma_read(&lTmp, 1);

            usart_dma_rx_skip(1);   /* skip ',' */

            lChannel = lTmp - '0';
        } else {
            lChannel = 4;   /* 4 is default channel on single channel connections */
        }

//        dbg("Received +IPD on channel %d\r\n", lChannel);

        if(!sEsp8266.mSockets[lChannel].mUsed) {
            dbg_err("%s(%d): Socket %p is not open!\r\n", __FILE__, __LINE__, &sEsp8266.mSockets[lChannel]);
        }

        if(lChannel >= ESP8266_MAX_CONNECTIONS) {
            dbg_err("%s(%d): lSocketID (%d) >= ESP8266_MAX_CONNECTIONS(%d)\r\n", __FILE__, __LINE__, lChannel, ESP8266_MAX_CONNECTIONS);
        }

        if(((sEsp8266.mSockets[lChannel].mHead + 1) & (ESP8266_MAX_CHANNEL_RCV_CNT - 1)) == sEsp8266.mSockets[lChannel].mTail) {
            dbg_err("%s(%d): Input overrun on socket %p!\r\n", __FILE__, __LINE__, &sEsp8266.mSockets[lChannel]);
        }

        /* get data size */
        lLen = usart_dma_read_until(&lNumberBuffer[0], sizeof(lNumberBuffer)-1, ':');
        lNumberBuffer[lLen] = '\0';

        lRcvLen = strtoul((char*)lNumberBuffer, NULL, 0);

        dbg("%s(%d): Receiving %d bytes on socket %p\r\n", __FILE__, __LINE__, lRcvLen, &sEsp8266.mSockets[lChannel]);

        if(lRcvLen >= ESP8266_MAX_CHANNEL_RCV_SIZE) {
            dbg_err("%s(%d): lRcvLen(%d) >= ESP8266_MAX_CHANNEL_RCV_SIZE(%d)\r\n", __FILE__, __LINE__, lRcvLen, ESP8266_MAX_CHANNEL_RCV_SIZE);
        }

        lPtr = &sEsp8266.mSockets[lChannel].mRxBuffer[sEsp8266.mSockets[lChannel].mHead].mBuffer[0];
        sEsp8266.mSockets[lChannel].mRxBuffer[sEsp8266.mSockets[lChannel].mHead].mLength = lRcvLen;
        sEsp8266.mSockets[lChannel].mRxBuffer[sEsp8266.mSockets[lChannel].mHead].mIndex = 0;

        /* receive lRcvLen bytes from the device */
        for(lCount = 0; lCount < lRcvLen;) {
#if defined(ESP8266_FREERTOS)
            if(usart_dma_rx_num() == 0) {

                dbg("%s(%d): usart_dma_rx_wait\r\n", __FILE__, __LINE__);
                /* wait if there's not enough data */
                if(!usart_dma_rx_wait()) {
                    dbg_err("%s(%d): usart_dma_rx_wait timed out\r\n", __FILE__, __LINE__);
                }
            }
#endif /* ESP8266_FREERTOS */
            lLen = usart_dma_read(&lPtr[lCount], lRcvLen - lCount);
            lCount += lLen;
        }

        /* increment receive buffer ring */
        sEsp8266.mSockets[lChannel].mHead = (sEsp8266.mSockets[lChannel].mHead + 1) & (ESP8266_MAX_CHANNEL_RCV_CNT - 1);

#if defined(ESP8266_FREERTOS)
        /* notify socket */
        xSemaphoreGive(sEsp8266.mSockets[lChannel].mRxSemaphore);
#endif /* ESP8266_FREERTOS */

//        dbg("%s(%d): Treated by IPD\r\n", __FILE__, __LINE__);
        return true;
    }
    return false;
}

/*! This function handles WIFI asynchronous messages
    \todo implement
*/
STATIC bool esp8266_rx_handle_wifi(void) {


    if(usart_dma_peek(sWIFI_DIS, sizeof(sWIFI_DIS))) {

        ts_esp8266_wifi_cmd lWifiCommand = {
            .mCommand = ESP8266_WIFI_DISCONNECT,
        };

        dbg("%s(%d): Wifi disconnected\r\n", __FILE__, __LINE__);
        usart_dma_rx_skip(sizeof(sWIFI_DIS));

        if(!xQueueSend(sEsp8266.mWIFICmdQ, &lWifiCommand, 1000 / portTICK_PERIOD_MS)) {
            dbg_err("%s(%d): sending to queue failed!\r\n", __FILE__, __LINE__);
        }

        return true;
    }

    if(usart_dma_peek(sWIFI_CON, sizeof(sWIFI_CON))) {

        ts_esp8266_wifi_cmd lWifiCommand = {
            .mCommand = ESP8266_WIFI_CONNECTED,
        };

        dbg("%s(%d): Wifi connected\r\n", __FILE__, __LINE__);
        usart_dma_rx_skip(sizeof(sWIFI_CON));

        if(!xQueueSend(sEsp8266.mWIFICmdQ, &lWifiCommand, 1000 / portTICK_PERIOD_MS)) {
            dbg_err("%s(%d): sending to queue failed!\r\n", __FILE__, __LINE__);
        }

        return true;
    }

    if(usart_dma_peek(sWIFI_IP, sizeof(sWIFI_IP))) {

        ts_esp8266_wifi_cmd lWifiCommand = {
            .mCommand = ESP8266_WIFI_GOT_IP,
        };

        dbg("%s(%d): Wifi got IP\r\n", __FILE__, __LINE__);
        usart_dma_rx_skip(sizeof(sWIFI_IP));

        if(!xQueueSend(sEsp8266.mWIFICmdQ, &lWifiCommand, 1000 / portTICK_PERIOD_MS)) {
            dbg_err("%s(%d): sending to queue failed!\r\n", __FILE__, __LINE__);
        }

        return true;
    }

    return false;
}

#undef dbg
#define dbg dbg_off

/*! This function handles x,CLOSED or X,CONNECT asynchronous messages
    \todo implement
*/
STATIC bool esp8266_rx_handle_socket(void) {

    /* 0,CONNECT\r\n */
    /* skip the first character */
    if(usart_dma_peek_skip(sCONNECT, sizeof(sCONNECT), 1)) {

        uintptr_t lChannel;
        uint8_t lTmp = '0';

#if 0
        /* WEP: Warning, this could remove data which is needed elsewhere */
        if(lLen > sizeof(sCONNECT) + 1) {

            uint8_t lBuffer[32];
            size_t lBufferLen;
            size_t lReadLen;

            dbg_err("%s(%d): WARNING! Skipping %d unhandled bytes\r\n", __FILE__, __LINE__, lLen - (sizeof(sCONNECT) + 1));
            //usart_dma_rx_skip(lLen - (sizeof(sCONNECT) + 1));  /* skip what's before the match */

            if(sizeof(lBuffer)-1 < lLen - (sizeof(sCONNECT) + 1)) {
                lReadLen = sizeof(lBuffer)-1;
            } else {
                lReadLen = lLen - (sizeof(sCONNECT) + 1);
            }
            lBufferLen = usart_dma_read(lBuffer, lReadLen);
            usart_dma_rx_skip(lLen - (sizeof(sCONNECT) + 1 + lReadLen));

            lBuffer[lBufferLen] = '\0';
            dbg_err("%s(%d): skipped content: \"%s\"\r\n", __FILE__, __LINE__, lBuffer);
        }
#endif

        usart_dma_read(&lTmp, 1);               /* read the channel */
        usart_dma_rx_skip(sizeof(sCONNECT));    /* skip the rest */

        lTmp -= '0';
        lChannel = lTmp;

        if(lChannel < ESP8266_MAX_CONNECTIONS) {

            dbg("%s(%d): connect [%d] %p\r\n", __FILE__, __LINE__, lChannel, &sEsp8266.mSockets[lChannel]);

            ts_esp8266_socket_cmd lSocketCommand = {
                .mCommand = ESP8266_SOCKET_OPEN,
                .mSocket = lChannel
            };

            if(!xQueueSend(sEsp8266.mSocketCmdQ, &lSocketCommand, 1000 / portTICK_PERIOD_MS)) {
                dbg_err("%s(%d): sending to queue failed!\r\n", __FILE__, __LINE__);
            }

        } else {

            dbg_err("%s(%d): Socket id %d is too big!\r\n", __FILE__, __LINE__, lChannel);
        }

        return true;
    }

    /* 0,CLOSED\r\n */
    if(usart_dma_peek_skip(sCLOSED, sizeof(sCLOSED), 1)) {

        uint8_t lChannel = '0';

#if 0
        /* WEP: Warning, this could remove data which is needed elsewhere */
        if(lLen > sizeof(sCLOSED) + 1) {

            uint8_t lBuffer[32];
            size_t lBufferLen;
            size_t lReadLen;

            dbg_err("%s(%d): WARNING! Skipping %d unhandled bytes\r\n", __FILE__, __LINE__, lLen - (sizeof(sCLOSED) + 1));
            //usart_dma_rx_skip(lLen - (sizeof(sCLOSED) + 1));  /* skip what's before the match */

            if(sizeof(lBuffer)-1 < lLen - (sizeof(sCLOSED) + 1)) {
                lReadLen = sizeof(lBuffer)-1;
            } else {
                lReadLen = lLen - (sizeof(sCLOSED) + 1);
            }
            lBufferLen = usart_dma_read(lBuffer, lReadLen);
            usart_dma_rx_skip(lLen - (sizeof(sCLOSED) + 1 + lReadLen));

            lBuffer[lBufferLen] = '\0';
            dbg_err("%s(%d): skipped content: \"%s\"\r\n", __FILE__, __LINE__, lBuffer);
        }
#endif
        usart_dma_read(&lChannel, 1);           /* read the channel */
        usart_dma_rx_skip(sizeof(sCLOSED));     /* skip the rest */

        lChannel -= '0';

        if(lChannel < ESP8266_MAX_CONNECTIONS) {

            ts_esp8266_socket_cmd lSocketCommand = {
                .mCommand = ESP8266_SOCKET_CLOSE,
                .mSocket = lChannel
            };

            dbg("%s(%d): close [%d] %p\r\n", __FILE__, __LINE__, lChannel, &sEsp8266.mSockets[lChannel]);

            if(!xQueueSend(sEsp8266.mSocketCmdQ, &lSocketCommand, 1000 / portTICK_PERIOD_MS)) {
                dbg_err("%s(%d): sending to queue failed!\r\n", __FILE__, __LINE__);
            }

        } else {
            dbg_err("%s(%d): Socket %d is too big!\r\n", __FILE__, __LINE__, lChannel);
        }

//        dbg("%s(%d): %s treated\r\n", __FILE__, __LINE__, __func__);
        return true;
    }

    if(usart_dma_peek_skip(sCONNECT_FAIL, sizeof(sCONNECT_FAIL), 1)) {

        uint8_t lChannel = '0';

#if 0
        /* WEP: Warning, this could remove data which is needed elsewhere */
        if(lLen > sizeof(sCLOSED) + 1) {

            uint8_t lBuffer[32];
            size_t lBufferLen;
            size_t lReadLen;

            dbg_err("%s(%d): WARNING! Skipping %d unhandled bytes\r\n", __FILE__, __LINE__, lLen - (sizeof(sCONNECT_FAIL) + 1));
            //usart_dma_rx_skip(lLen - (sizeof(sCONNECT_FAIL) + 1));  /* skip what's before the match */

            if(sizeof(lBuffer)-1 < lLen - (sizeof(sCONNECT_FAIL) + 1)) {
                lReadLen = sizeof(lBuffer)-1;
            } else {
                lReadLen = lLen - (sizeof(sCONNECT_FAIL) + 1);
            }
            lBufferLen = usart_dma_read(lBuffer, lReadLen);
            usart_dma_rx_skip(lLen - (sizeof(sCONNECT_FAIL) + 1 + lReadLen));

            lBuffer[lBufferLen] = '\0';
            dbg_err("%s(%d): skipped content: \"%s\"\r\n", __FILE__, __LINE__, lBuffer);
        }
#endif
        usart_dma_read(&lChannel, 1);           /* read the channel */
        usart_dma_rx_skip(sizeof(sCONNECT_FAIL));     /* skip the rest */

        lChannel -= '0';

        if(lChannel < ESP8266_MAX_CONNECTIONS) {

            ts_esp8266_socket_cmd lSocketCommand = {
                .mCommand = ESP8266_SOCKET_CONNECT_FAIL,
                .mSocket = lChannel
            };

            dbg("%s(%d): connect fail [%d] %p\r\n", __FILE__, __LINE__, lChannel, &sEsp8266.mSockets[lChannel]);

            if(!xQueueSend(sEsp8266.mSocketCmdQ, &lSocketCommand, 1000 / portTICK_PERIOD_MS)) {
                dbg_err("%s(%d): sending to queue failed!\r\n", __FILE__, __LINE__);
            }

        } else {
            dbg_err("%s(%d): Socket %d is too big!\r\n", __FILE__, __LINE__, lChannel);
        }

        return true;
    }

    return false;
}

#undef dbg
#define dbg dbg_off

/*! Handles generic command responses

    \param[in]  inSentinel      The sentinel to match
    \param[in]  inSentinelLen   The length of the sentinel

    \retval true    Sentinel found and treated
    \retval false   Sentinel not found
*/
STATIC bool esp8266_rx_handle_command_response_gen(const uint8_t * const inSentinel, size_t inSentinelLen) {

    /* received command response */
    size_t lSize = 0;

    usart_dma_match(inSentinel, inSentinelLen, &lSize);
    if(lSize > 0) {

        dbg("%s(%d): Sentinel found!\r\n", __FILE__, __LINE__);

        if(sEsp8266.mResponseBuffer == NULL || sEsp8266.mResponseLen == 0) {

            if(lSize > inSentinelLen) {
#if 0
                uint8_t lBuffer[64];
                size_t lBufferLen;
                size_t lReadLen;
#endif
                dbg_err("%s(%d): WARNING: sentinel lenght: %d, received %d\r\n", __FILE__, __LINE__, inSentinelLen, lSize);
#if 0

                lReadLen = (inSentinelLen < sizeof(lBuffer)-1)? inSentinelLen : (sizeof(lBuffer)-1);
                memcpy(lBuffer, inSentinel, lReadLen);
                lBuffer[lReadLen] = '\0';
                dbg_err("%s(%d): sentinel was: \"%s\"\r\n", __FILE__, __LINE__, lBuffer);

                if(sizeof(lBuffer)-1 < lSize - inSentinelLen) {
                    lReadLen = sizeof(lBuffer)-1;
                } else {
                    lReadLen = lSize - inSentinelLen;
                }
                lBufferLen = usart_dma_read(lBuffer, lReadLen);
                usart_dma_rx_skip(lSize - lReadLen);

                lBuffer[lBufferLen] = '\0';
                dbg_err("%s(%d): skipped content: \"%s\"\r\n", __FILE__, __LINE__, lBuffer);

            } else {
                usart_dma_rx_skip(lSize);
#endif
            }

#if 1
            /* nobody want's the data, so drop it */
            usart_dma_rx_skip(lSize);
#endif
        } else {

            size_t lReadSize;

            if(sEsp8266.mResponseLen < lSize - inSentinelLen) {
                lReadSize = sEsp8266.mResponseLen;
            } else {
                lReadSize = lSize - inSentinelLen;
            }

            /* lSize - inSentinelLen <= sEsp8266.mResponseLen */
            sEsp8266.mResponseRcvLen = usart_dma_read(sEsp8266.mResponseBuffer, lReadSize);

            /* drop the rest including the sentinel */
            usart_dma_rx_skip(lSize - lReadSize);

            /* reset response buffer */
            sEsp8266.mResponseBuffer = NULL;
            sEsp8266.mResponseLen = 0;
        }

#if defined(ESP8266_FREERTOS)
        /* notify caller */
        xSemaphoreGive(sEsp8266.mResponseSema);
#endif /* ESP8266_FREERTOS */

        return true;
    }

    return false;
}

/*! This function handles incoming command responses
*/
STATIC bool esp8266_rx_handle_command_response(void) {

    bool lReturnValue = false;

    if(sEsp8266.mSentinel != NULL && sEsp8266.mSentinelLen > 0) {


        lReturnValue = esp8266_rx_handle_command_response_gen(sEsp8266.mSentinel, sEsp8266.mSentinelLen);
        if(lReturnValue) {
            /* good response found */
            esp8266_set_sentinel(NULL, 0, NULL, 0);

            sEsp8266.mStatus = ESP8266_OK;

            dbg("%s(%d): %s ok sentinel\r\n", __FILE__, __LINE__, __func__);
            return true;
        }
    }

    if(sEsp8266.mErrorSentinel != NULL && sEsp8266.mErrorSentinelLen > 0) {
        bool lReturnValue = false;

        lReturnValue = esp8266_rx_handle_command_response_gen(sEsp8266.mErrorSentinel, sEsp8266.mErrorSentinelLen);
        if(lReturnValue) {
            /* error found */
            esp8266_set_sentinel(NULL, 0, NULL, 0);

            sEsp8266.mStatus = ESP8266_ERROR;

            dbg("%s(%d): %s error sentinel\r\n", __FILE__, __LINE__, __func__);
            return true;
        }
    }

    return lReturnValue;
}

void esp8266_rx_handler(void) {

    bool lTreated;

#if defined(ESP8266_FREERTOS)
    for(;usart_dma_rx_num() == sEsp8266.mTreated && !sEsp8266.mForcedRefresh;) {

#if 0
        if(usart_dma_rx_num() > 0) {   /* peek what's in the buffer */

            uint8_t lDbgBuffer[64];
            size_t lDbgLen;

            dbg_err("%s(%d): going to sleep %d bytes left in buffer\r\n", __FILE__, __LINE__, usart_dma_rx_num());

            lDbgLen = usart_dma_read_peek(lDbgBuffer, sizeof(lDbgBuffer)-1);
            lDbgBuffer[lDbgLen] = '\0';
            dbg_err("%s(%d): Buffer Content: \"%s\"\r\n", __FILE__, __LINE__, lDbgBuffer);
        }
#endif

        /* only wait if there's data we didn't treat before */
        if(!usart_dma_rx_wait()) {
            dbg_err("%s(%d): usart_dma_rx_wait timed out\r\n", __FILE__, __LINE__);
        }

        dbg("%s(%d): waking up\r\n", __FILE__, __LINE__);
    }
#endif /* ESP8266_FREERTOS */

    dbg("%s(%d): in rx buffer: %d bytes\r\n", __FILE__, __LINE__, usart_dma_rx_num());

    do {

        lTreated = false;

        if(usart_dma_peek(sIPD, sizeof(sIPD))) {

            lTreated = esp8266_rx_handle_ipd();

        } else if(usart_dma_peek(sWIFI_DIS, sizeof(sWIFI_DIS)) ||
                  usart_dma_peek(sWIFI_CON, sizeof(sWIFI_CON)) || 
                  usart_dma_peek(sWIFI_IP, sizeof(sWIFI_IP))) {

            lTreated = esp8266_rx_handle_wifi();
        } else if(usart_dma_peek_skip(sCONNECT, sizeof(sCONNECT), 1) ||
                  usart_dma_peek_skip(sCLOSED, sizeof(sCLOSED), 1) ||
                  usart_dma_peek_skip(sCONNECT_FAIL, sizeof(sCONNECT_FAIL), 1)) {

            lTreated = esp8266_rx_handle_socket();
        } else {
            lTreated = esp8266_rx_handle_command_response();
        }

#if 0
        /* handle socket receive */
        lTreated = esp8266_rx_handle_ipd() || lTreated;

        /* handle socket connect / close */
        lTreated = esp8266_rx_handle_socket() || lTreated;

        /* handle WIFI ... messages */
        lTreated = esp8266_rx_handle_wifi() || lTreated;
#endif

        /* handle command response */
//        lTreated = esp8266_rx_handle_command_response() || lTreated;

    } while(lTreated);  /* treat as long as we can */

    /* update treated */
    sEsp8266.mTreated = usart_dma_rx_num();
    sEsp8266.mForcedRefresh = false;
}

void esp8266_socket_handler(void) {

    ts_esp8266_socket_cmd lSocketCommand;

    if(xQueueReceive(sEsp8266.mSocketCmdQ, &lSocketCommand, portMAX_DELAY)) {

        switch(lSocketCommand.mCommand) {
            case ESP8266_SOCKET_OPEN: {

                    dbg("%s(%d): Open socket %p\r\n", __FILE__, __LINE__, &sEsp8266.mSockets[lSocketCommand.mSocket]);

                    sEsp8266.mSockets[lSocketCommand.mSocket].mUsed = true;

                    if(sEsp8266.mSockets[lSocketCommand.mSocket].mTaskHandle == NULL) {   /* spawn off the server task */

                        BaseType_t lRetVal;
                        TaskHandle_t lTaskHandle = NULL;

                        lRetVal = xTaskCreate(esp8266_server_handler_task, ( const char * )"esp8266_sh", configMINIMAL_STACK_SIZE * 10, (void*)lSocketCommand.mSocket, sEsp8266.mServerTaskPriority, &lTaskHandle);
                        sEsp8266.mSockets[lSocketCommand.mSocket].mTaskHandle = lTaskHandle;    // avoid compiler warning about volatile
                        if(lRetVal) {
                            dbg("%s(%d): successfully created server task %p\r\n", __FILE__, __LINE__, sEsp8266.mSockets[lSocketCommand.mSocket].mTaskHandle);
                        } else {
                            dbg_err("%s(%d): failed creating server task for socket %p\r\n", __FILE__, __LINE__, &sEsp8266.mSockets[lSocketCommand.mSocket]);
                        }
                    } else {
                        dbg_err("%s(%d): Server Handler Task for socket %p already spawned\r\n", __FILE__, __LINE__, &sEsp8266.mSockets[lSocketCommand.mSocket]);
                    }
                }
                break;
            case ESP8266_SOCKET_CLOSE:
            case ESP8266_SOCKET_CONNECT_FAIL: {

                    /* unmark the socket from being used */
                    esp8266_socket_cleanup(&sEsp8266.mSockets[lSocketCommand.mSocket]);

                    /* notify everybody who's waiting on the socket */
                    xSemaphoreGive(sEsp8266.mSockets[lSocketCommand.mSocket].mRxSemaphore);

                    /* wait for socket to close */
                    for(;sEsp8266.mSockets[lSocketCommand.mSocket].mTaskHandle != NULL;) {

                        if(!xSemaphoreTake(sEsp8266.mCloseNotify, 5000 / portTICK_PERIOD_MS)) {
                            dbg_err("%s(%d): Semaphore timeout!\r\n", __FILE__, __LINE__);
                            break;
                        }
                    }
                }
                break;
        }
    } else {
        dbg_err("%s(%d): timeout waitng on socket command\r\n", __FILE__, __LINE__);
    }
}

void esp8266_wifi_connection_handler(void) {

    ts_esp8266_wifi_cmd lWifiCommand;

    /* timeout every 10 seconds */
    if(xQueueReceive(sEsp8266.mWIFICmdQ, &lWifiCommand, portMAX_DELAY /* 10000 / portTICK_PERIOD_MS*/)) {

        switch(lWifiCommand.mCommand) {
            case ESP8266_WIFI_CONNECTED:
                break;
            case ESP8266_WIFI_GOT_IP:
                break;
            case ESP8266_WIFI_DISCONNECT:

                /*
                    if ESP8266 is in station + ap mode, loosing it's station connection will
                    make the AP unusable!

                    So on disconnect, we'll disable the station and regularely try to re-enable
                */
                if(sEsp8266.mWifiMode == ESP8266_WIFI_MODE_STA_AP) {

                    if(!esp8266_cmd_cwqap()) {
                        dbg_err("%s(%d): Could not quit access point!\r\n", __FILE__, __LINE__);
                    }
                }

                break;
        }
    } else {
        dbg_err("%s(%d): timeout waiting on wifi command\r\n", __FILE__, __LINE__);
    }
}

/*! Retreive the length of the response

    \return the length of the response string, excluding the terminating NULL character
*/
STATIC size_t getResponseLength(void) {

    return sEsp8266.mResponseRcvLen;
}

/*! This function sets the response buffer

    \param[in]  inResponseBuffer      The sentinel string
    \param[in]  inResponseBufferLen   The length of the sentinel string
*/
STATIC void esp8266_set_response_buffer(uint8_t * inResponseBuffer, size_t inResponseBufferLen) {

    sEsp8266.mResponseBuffer = inResponseBuffer;
    sEsp8266.mResponseLen    = inResponseBufferLen;
    sEsp8266.mResponseRcvLen = 0;
}

/*! This function sets the sentinel

    \param[in]  inSentinel      The sentinel string
    \param[in]  inSentinelLen   The length of the sentinel string
*/
STATIC void esp8266_set_sentinel(const uint8_t * const inSentinel, size_t inSentinelLen, const uint8_t * const inErrorSentinel, size_t inErrorSentinelLen) {

    sEsp8266.mSentinel    = inSentinel;
    sEsp8266.mSentinelLen = inSentinelLen;

    sEsp8266.mErrorSentinel    = inErrorSentinel;
    sEsp8266.mErrorSentinelLen = inErrorSentinelLen;
}

/*! Force the rx handler to re-process
*/
STATIC void esp8266_request_rx_handler_refresh(void) {

    dbg("%s(%d): Forcing refresh\r\n", __FILE__, __LINE__);

    /* re-evalutate handlers */
    sEsp8266.mForcedRefresh = true;

    /* fire rx semaphore to wake the rx task up */
    usart_dma_rx_force_wakeup();
}

/*! Execute a command which just response OK or ERROR

    \param[in]  inCommand       The command string to send
    \param[in]  inCommandLen    The length of the command string

    \retval true        OK was returned
    \retval false       something else was returned
*/
STATIC bool esp8266_ok_cmd(const uint8_t * const inCommand, size_t inCommandLen, uint32_t inTimeoutMs) {

    bool lReturnValue = false;

#if defined(ESP8266_FREERTOS)
    if(xSemaphoreTakeRecursive(sEsp8266.mMutex, portMAX_DELAY)) {
#endif

        sEsp8266.mStatus = ESP8266_IN_PROGRESS;

        /* set the sentinel to OK / ERROR */
        esp8266_set_sentinel(sOK, sizeof(sOK), sERROR, sizeof(sERROR));

        /* set the response buffer */
        esp8266_set_response_buffer(NULL, 0);

        /* send command to esp8266 */
        usart_dma_write(inCommand, inCommandLen);

        /* send cr + lf */
        usart_dma_write(sCRLF, sizeof(sCRLF));

        /* wait on the answer */
#if defined(ESP8266_FREERTOS)
        if(!xSemaphoreTake(sEsp8266.mResponseSema, inTimeoutMs / portTICK_PERIOD_MS)) {
            esp8266_set_sentinel(NULL, 0, NULL, 0);
            sEsp8266.mStatus = ESP8266_TIMEOUT;
            dbg("Timeout\r\n");
        }
#endif

        if(sEsp8266.mStatus == ESP8266_OK) {
            lReturnValue = true;
        }

#if defined(ESP8266_FREERTOS)
        xSemaphoreGiveRecursive(sEsp8266.mMutex);
    } else {
        dbg_err("%s(%d): sEsp8266.mMutex Timeout\r\n", __FILE__, __LINE__);
    }
#endif

    return lReturnValue;
}

/*! Execute a command which just response OK or FAIL

    \param[in]  inCommand       The command string to send
    \param[in]  inCommandLen    The length of the command string

    \retval true        OK was returned
    \retval false       something else was returned
*/
STATIC bool esp8266_ok_cmd_fail(const uint8_t * const inCommand, size_t inCommandLen, uint32_t inTimeoutMs) {

    bool lReturnValue = false;

#if defined(ESP8266_FREERTOS)
    if(xSemaphoreTakeRecursive(sEsp8266.mMutex, portMAX_DELAY)) {
#endif

        sEsp8266.mStatus = ESP8266_IN_PROGRESS;

        /* set the sentinel to OK / ERROR */
        esp8266_set_sentinel(sOK, sizeof(sOK), sFAIL, sizeof(sFAIL));

        /* set the response buffer */
        esp8266_set_response_buffer(NULL, 0);

        /* send command to esp8266 */
        usart_dma_write(inCommand, inCommandLen);

        /* send cr + lf */
        usart_dma_write(sCRLF, sizeof(sCRLF));

        /* wait on the answer */
#if defined(ESP8266_FREERTOS)
        if(!xSemaphoreTake(sEsp8266.mResponseSema, inTimeoutMs / portTICK_PERIOD_MS)) {
            esp8266_set_sentinel(NULL, 0, NULL, 0);
            sEsp8266.mStatus = ESP8266_TIMEOUT;
            dbg("Timeout\r\n");
        }
#endif

        if(sEsp8266.mStatus == ESP8266_OK) {
            lReturnValue = true;
        }

#if defined(ESP8266_FREERTOS)
        xSemaphoreGiveRecursive(sEsp8266.mMutex);
    } else {
        dbg_err("%s(%d): sEsp8266.mMutex Timeout\r\n", __FILE__, __LINE__);
    }
#endif

    return lReturnValue;
}

/*! Execute a command which just response OK or ERROR

    \param[in]  inCommand       The command string to send
    \param[in]  inCommandLen    The length of the command string

    \retval true        OK was returned
    \retval false       something else was returned
*/
STATIC bool esp8266_ok_cmd_str(const uint8_t * const inCommand, size_t inCommandLen, uint8_t * inOutData, size_t inDataMaxLen, size_t * outDataLen, uint32_t inTimeoutMs) {

    bool lReturnValue = false;

#if defined(ESP8266_FREERTOS)
    if(xSemaphoreTakeRecursive(sEsp8266.mMutex, portMAX_DELAY)) {
#endif

        sEsp8266.mStatus = ESP8266_IN_PROGRESS;

        /* set the sentinel to OK / ERROR */
        esp8266_set_sentinel(sOK, sizeof(sOK), sERROR, sizeof(sERROR));

        /* set the response buffer */
        esp8266_set_response_buffer(inOutData, inDataMaxLen);

        /* send command to esp8266 */
        usart_dma_write(inCommand, inCommandLen);

        /* send cr + lf */
        usart_dma_write(sCRLF, sizeof(sCRLF));

        /* wait on the answer */
#if defined(ESP8266_FREERTOS)
        if(!xSemaphoreTake(sEsp8266.mResponseSema, inTimeoutMs / portTICK_PERIOD_MS)) {
            esp8266_set_sentinel(NULL, 0, NULL, 0);
            sEsp8266.mStatus = ESP8266_TIMEOUT;
            dbg("Timeout\r\n");
        }
#endif

        /* get the length of the response */
        *outDataLen = getResponseLength();

        if(sEsp8266.mStatus == ESP8266_OK) {

            lReturnValue = true;
        }

#if defined(ESP8266_FREERTOS)
        xSemaphoreGiveRecursive(sEsp8266.mMutex);
    } else {
        dbg_err("%s(%d): sEsp8266.mMutex Timeout\r\n", __FILE__, __LINE__);
    }
#endif

    return lReturnValue;
}

bool esp8266_cmd_at(void) {

    return esp8266_ok_cmd(sAT, sizeof(sAT), 1000);
}

bool esp8266_cmd_rst(void) {

    bool lReturnValue = false;

#if defined(ESP8266_FREERTOS)
    if(xSemaphoreTakeRecursive(sEsp8266.mMutex, portMAX_DELAY)) {
#endif

        sEsp8266.mStatus = ESP8266_IN_PROGRESS;

        /* set the sentinel to OK / ERROR */
        esp8266_set_sentinel(sREADY, sizeof(sREADY), NULL, 0);

        /* set the response buffer */
        esp8266_set_response_buffer(NULL, 0);

        /* send command to esp8266 */
        usart_dma_write(sAT_RST, sizeof(sAT_RST));

        /* send cr + lf */
        usart_dma_write(sCRLF, sizeof(sCRLF));

        /* wait on the answer */
#if defined(ESP8266_FREERTOS)
        if(!xSemaphoreTake(sEsp8266.mResponseSema, 2000 / portTICK_PERIOD_MS)) {
            esp8266_set_sentinel(NULL, 0, NULL, 0);
            sEsp8266.mStatus = ESP8266_TIMEOUT;
        }
#endif

        if(sEsp8266.mStatus == ESP8266_OK) {
            lReturnValue = true;
        }

#if defined(ESP8266_FREERTOS)
        xSemaphoreGiveRecursive(sEsp8266.mMutex);
    } else {
        dbg_err("%s(%d): sEsp8266.mMutex Timeout\r\n", __FILE__, __LINE__);
    }
#endif

    return lReturnValue;
}

bool esp8266_cmd_gmr(uint8_t * outVersionInfo, size_t inVersionInfoMaxLen, size_t * outVersionInfoLen) {

    return esp8266_ok_cmd_str(sAT_GMR, sizeof(sAT_GMR), outVersionInfo, inVersionInfoMaxLen, outVersionInfoLen, 1000);
}

bool esp8266_cmd_ate0(void) {

    return esp8266_ok_cmd(sATE0, sizeof(sATE0), 1000);
}

bool esp8266_cmd_set_cwmode_cur(te_esp8266_wifi_mode inWifiMode) {

    uint8_t lCommandBuffer[sizeof(sAT_CWMODE) + sizeof(sCUR) + 1 + 1];
    size_t lLen = sizeof(sAT_CWMODE);

    memcpy(lCommandBuffer, sAT_CWMODE, sizeof(sAT_CWMODE));
    lLen = sizeof(sAT_CWMODE);
    memcpy(&lCommandBuffer[lLen], sCUR, sizeof(sCUR));
    lLen += sizeof(sCUR);

    lCommandBuffer[lLen++] = '=';
    
    switch(inWifiMode) {
        case ESP8266_WIFI_MODE_STATION:
            lCommandBuffer[lLen++] = '1';
            break;
        case ESP8266_WIFI_MODE_AP:
            lCommandBuffer[lLen++] = '2';
            break;
        case ESP8266_WIFI_MODE_STA_AP:
            lCommandBuffer[lLen++] = '3';
            break;
        default:
            return false;
    }

    sEsp8266.mWifiMode = inWifiMode;

    return esp8266_ok_cmd(lCommandBuffer, lLen, 1000);
}

bool esp8266_cmd_get_cwmode_cur(te_esp8266_wifi_mode * outWifiMode) {

    uint8_t lCommandBuffer[sizeof(sAT_CWMODE) + sizeof(sCUR) + 1];
    uint8_t lReturnBuffer[sizeof(sAT_CWMODE) + sizeof(sCUR) - 2 + 1 + 1 + 1 + 1];     /* "+CWMODE_CUR" + : + <mode> + \r + \n */
    size_t  lReturnLen;
    size_t lLen;

    if(!outWifiMode) {
        return false;
    }

    memcpy(lCommandBuffer, sAT_CWMODE, sizeof(sAT_CWMODE));
    lLen = sizeof(sAT_CWMODE);
    memcpy(&lCommandBuffer[lLen], sCUR, sizeof(sCUR));
    lLen += sizeof(sCUR);

    lCommandBuffer[lLen++] = '?';

    if(esp8266_ok_cmd_str(lCommandBuffer, lLen, lReturnBuffer, sizeof(lReturnBuffer), &lReturnLen, 1000)) {

        if(memcmp(&sAT_CWMODE[2], lReturnBuffer, sizeof(sAT_CWMODE) - 2) == 0) {  /* we found +CWMODE */

            switch(lReturnBuffer[sizeof(sAT_CWMODE) + sizeof(sCUR) - 2 + 1]) { /* skip +CWMODE_CUR: */
                case '1':
                    *outWifiMode = ESP8266_WIFI_MODE_STATION;
                    break;
                case '2':
                    *outWifiMode = ESP8266_WIFI_MODE_AP;
                    break;
                case '3':
                    *outWifiMode = ESP8266_WIFI_MODE_STA_AP;
                    break;
                default:
                    return false;
            }

        } else {

            return false;
        }

        sEsp8266.mWifiMode = *outWifiMode;

        return true;
    } else {
        return false;
    }
}

bool esp8266_cmd_set_cwjap_cur(uint8_t * inSSID, size_t inSSIDLen, uint8_t * inPWD, size_t inPWDLen) {

    uint8_t lCommandBuffer[sizeof(sAT_CWJAP) + sizeof(sCUR) + 1 + 1 + 31 + 1 + 1 + 1 + 64 + 1];    /* command length + = + quote + ssid + quote + comma + quote + password lenght + quote */
    size_t  lLen;

    if(inSSIDLen > 31 || inPWDLen > 64) {
        return false;
    }

    memcpy(lCommandBuffer, sAT_CWJAP, sizeof(sAT_CWJAP));
    lLen = sizeof(sAT_CWJAP);
    memcpy(&lCommandBuffer[lLen], sCUR, sizeof(sCUR));
    lLen += sizeof(sCUR);
    lCommandBuffer[lLen++] = '=';
    lCommandBuffer[lLen++] = '"';
    memcpy(&lCommandBuffer[lLen], inSSID, inSSIDLen);
    lLen += inSSIDLen;
    lCommandBuffer[lLen++] = '"';
    lCommandBuffer[lLen++] = ',';
    lCommandBuffer[lLen++] = '"';
    memcpy(&lCommandBuffer[lLen], inPWD, inPWDLen);
    lLen += inPWDLen;
    lCommandBuffer[lLen++] = '"';

    /* AT+CWJAP="AndroidAP","cyvg3835" */

    return esp8266_ok_cmd_fail(lCommandBuffer, lLen, 20000);
}

bool esp8266_cmd_get_cwjap_cur(uint8_t * outSSID, size_t inSSIDMaxLen, size_t * outSSIDLen) {

/* +CWJAP_CUR:"AndroidAP","cc:3a:61:32:9b:eb",6,-49 */

    uint8_t lCommandBuffer[sizeof(sAT_CWJAP) + sizeof(sCUR) + 1];
    uint8_t lReturnBuffer[sizeof(sAT_CWJAP) + sizeof(sCUR) - 2 + 1 + 1 + 31 + 1 + 1 + 1 + 17 + 1 + 1 + 2 + 1 + 3 + 1 + 1 ];     /* "+CWJAP_CUR" + : + " + <SSID> + " + , + " + <mac> + " + , + <channel> + , + <dB> + \r + \n */
    size_t  lReturnLen;
    size_t  lLen;

    if(!outSSID || !outSSIDLen) {
        return false;
    }

    memcpy(lCommandBuffer, sAT_CWJAP, sizeof(sAT_CWJAP));
    lLen = sizeof(sAT_CWJAP);
    memcpy(&lCommandBuffer[lLen], sCUR, sizeof(sCUR));
    lLen += sizeof(sCUR);

    lCommandBuffer[lLen++]   = '?';

    if(esp8266_ok_cmd_str(lCommandBuffer, sizeof(lCommandBuffer), lReturnBuffer, sizeof(lReturnBuffer)-1, &lReturnLen, 1000)) {

        if(memcmp(&sAT_CWJAP[2], lReturnBuffer, sizeof(sAT_CWJAP) - 2) == 0) {
            size_t lCount;

            for(lCount = 0; lCount < 31; lCount++) {

                if(lReturnBuffer[lCount + sizeof(sAT_CWJAP) + sizeof(sCUR) - 2 + 1 + 1] == (uint8_t)'"') {
                    break;
                }
                outSSID[lCount] = lReturnBuffer[lCount + sizeof(sAT_CWJAP) + sizeof(sCUR) - 2 + 1 + 1];
            }
            *outSSIDLen = lCount;
            return true;
        }
    }

    return false;
}

bool esp8266_cmd_cwlap(uint8_t * outAccessPointList, size_t inAccessPointListMaxLen, size_t * outAccessPointListLen) {

    return esp8266_ok_cmd_str(sAT_CWLAP, sizeof(sAT_CWLAP), outAccessPointList, inAccessPointListMaxLen, outAccessPointListLen, 20000);
}

bool esp8266_cmd_cwqap(void) {

    return esp8266_ok_cmd(sAT_CWQAP, sizeof(sAT_CWQAP), 1000);
}

bool esp8266_cmd_set_cwsap_cur(const uint8_t * const inSSID, const size_t inSSIDLen, const uint8_t * const inPWD, const size_t inPWDLen, const uint8_t inChannel, const te_esp8266_encryption_mode inEncryption) {

    uint8_t lCommandBuffer[sizeof(sAT_CWSAP) + sizeof(sCUR) + 1 + 1 + 31 + 1 + 1 + 1 + 64 + 1 + 1 + 2 + 1 + 1];    /* command length + equal + quote + ssid + quote + comma + quote + password lenght + quote + comma + channel + comma + enc mode */
    size_t  lLen;

    uint8_t lMode;

    if(inSSIDLen > 31 || inPWDLen > 64) {
        return false;
    }

    if(inChannel < 1 || inChannel > 14) {
        return false;
    }

    switch(inEncryption) {
        case ESP8266_ENC_MODE_OPEN:
            lMode = '0';
            break;
        case ESP8266_ENC_MODE_WPA_PSK:
            lMode = '2';
            break;
        case ESP8266_ENC_MODE_WPA2_PSK:
            lMode = '3';
            break;
        case ESP8266_ENC_MODE_WPA_WPA2_PSK:
            lMode = '4';
            break;
        default:
            return false;
    }

    memcpy(lCommandBuffer, sAT_CWSAP, sizeof(sAT_CWSAP));
    lLen = sizeof(sAT_CWSAP);
    memcpy(&lCommandBuffer[lLen], sCUR, sizeof(sCUR));
    lLen += sizeof(sCUR);
    lCommandBuffer[lLen++] = '=';

    lCommandBuffer[lLen++] = '"';
    memcpy(&lCommandBuffer[lLen], inSSID, inSSIDLen);
    lLen += inSSIDLen;
    lCommandBuffer[lLen++] = '"';
    lCommandBuffer[lLen++] = ',';
    lCommandBuffer[lLen++] = '"';
    memcpy(&lCommandBuffer[lLen], inPWD, inPWDLen);
    lLen += inPWDLen;
    lCommandBuffer[lLen++] = '"';
    lCommandBuffer[lLen++] = ',';

    if(inChannel >= 10) {
        lCommandBuffer[lLen++] = '1';
    }
    lCommandBuffer[lLen++] = '0' + (inChannel % 10);
    lCommandBuffer[lLen++] = ',';
    lCommandBuffer[lLen++] = lMode;

    /* AT+CWSAP="WP_AP","deadbeef",11,3 */

    return esp8266_ok_cmd(lCommandBuffer, lLen, 1000);
}

bool esp8266_cmd_get_cwsap_cur(uint8_t * outSSID, size_t inSSIDMaxLen, size_t * outSSIDLen, uint8_t * outPWD, size_t inPWDMaxLen, size_t * outPWDLen, uint8_t * outChannel, te_esp8266_encryption_mode * outEncryption) {

    uint8_t lCommandBuffer[sizeof(sAT_CWSAP) + sizeof(sCUR) + 1];
    size_t lLen;

    uint8_t lReturnBuffer[sizeof(sAT_CWSAP) + sizeof(sCUR) - 2 + 1 + 1 + 31 + 1 + 1 + 1 + 64 + 1 + 1 + 2 + 1 + 1 + 1 + 1];     /* "+CWSAP_CUR:" + " + <SSID> + " + , + " + <password> + " + , + <channel> + , + <encryption> + \r + \n */
    size_t  lReturnLen;

    if(!outSSID || !outSSIDLen || !outPWD || !outPWDLen || !outChannel || !outEncryption) {
        dbg("NULL\r\n");
        return false;
    }

    /* +CWSAP_CUR:"AI-THINKER_0164A6","1234567890",1,4 */

    memcpy(lCommandBuffer, sAT_CWSAP, sizeof(sAT_CWSAP));
    lLen = sizeof(sAT_CWSAP);
    memcpy(&lCommandBuffer[lLen], sCUR, sizeof(sCUR));
    lLen += sizeof(sCUR);
    lCommandBuffer[lLen++] = '?';

    if(esp8266_ok_cmd_str(lCommandBuffer, lLen, lReturnBuffer, sizeof(lReturnBuffer)-1, &lReturnLen, 1000)) {

        if(memcmp(&sAT_CWSAP[2], lReturnBuffer, sizeof(sAT_CWSAP)-2) == 0) {

            size_t lCount;
            size_t lIndex = sizeof(sAT_CWSAP) + sizeof(sCUR) - 2 + 1 + 1;      /* skip +AT_CWSAP_CUR:" */
            uint8_t lChannel;

            /* decode SSID */
            for(lCount = 0; lCount < 31; lCount++) {

                if(lReturnBuffer[lCount + lIndex] == (uint8_t)'"') {
                    break;
                }
                outSSID[lCount] = lReturnBuffer[lCount + lIndex];
            }
            *outSSIDLen = lCount;

            lIndex = lIndex + lCount+3;  /* lCount points to second " of SSID. Skip "," so increment by 3 */

            /* decode Password */
            for(lCount = 0; lCount < 64; lCount++) {

                if(lReturnBuffer[lCount + lIndex] == (uint8_t)'"') {
                    break;
                }
                outPWD[lCount] = lReturnBuffer[lCount + lIndex];
            }
            *outPWDLen = lCount;

            lIndex = lIndex + lCount + 2;   /* lCount points to second " of password. Skip ", so increment by 2 */

            /* decode channel */
            lChannel = lReturnBuffer[lIndex++] - '0';
            if(isdigit(lReturnBuffer[lIndex])) {
                lChannel = lChannel * 10 + (lReturnBuffer[lIndex++] - '0');
            }
            *outChannel = lChannel;

            lIndex++;   /* skip , */

            /* decode encoding */
            switch(lReturnBuffer[lIndex]) {
                case '0':
                default:
                    *outEncryption = ESP8266_ENC_MODE_OPEN;
                    break;
                case '2':
                    *outEncryption = ESP8266_ENC_MODE_WPA_PSK;
                    break;
                case '3':
                    *outEncryption = ESP8266_ENC_MODE_WPA2_PSK;
                    break;
                case '4':
                    *outEncryption = ESP8266_ENC_MODE_WPA_WPA2_PSK;
                    break;
            }

            return true;
        } else {
            dbg("Decode failed\r\n");
        }
    } else {
        dbg("Sending failed\r\n");
    }

    return false;
}

bool esp8266_cmd_get_cwlif(uint8_t * outStationList, size_t inStationListMaxLen, size_t * outStationListLen) {

    return esp8266_ok_cmd_str(sAT_CWLIF, sizeof(sAT_CWLIF), outStationList, inStationListMaxLen, outStationListLen, 1000);
}

bool esp8266_cmd_set_cwdhcp_cur(te_esp8266_dhcp_mode inDHCPMode, bool inEnable) {

    uint8_t lCommandBuffer[sizeof(sAT_CWDHCP) + sizeof(sCUR) + 1 + 1 + 1 + 1];  /* command + equal + mode + comma + ena/disa */
    size_t  lLen;
    uint8_t lMode;

    switch(inDHCPMode) {
        case ESP8266_DHCP_MODE_AP:
            lMode = '0';
            break;
        case ESP8266_DHCP_MODE_STATION:
            lMode = '1';
            break;
        case ESP8266_DHCP_MODE_AP_STA:
            lMode = '2';
            break;
        default:
            return false;
    }

    memcpy(lCommandBuffer, sAT_CWDHCP, sizeof(sAT_CWDHCP));
    lLen = sizeof(sAT_CWDHCP);
    memcpy(&lCommandBuffer, sCUR, sizeof(sCUR));
    lLen += sizeof(sCUR);
    lCommandBuffer[lLen++] = '=';
    lCommandBuffer[lLen++] = lMode;
    lCommandBuffer[lLen++] = ',';
    if(inEnable) {
        lCommandBuffer[lLen++] = '1';
    } else {
        lCommandBuffer[lLen++] = '0';
    }

    return esp8266_ok_cmd(lCommandBuffer, lLen, 1000);
}

bool esp8266_cmd_set_cipmux(bool inMultipleConnections) {

    uint8_t lCommandBuffer[sizeof(sAT_CIPMUX) + 1 + 1];   /* command + '=' + 1|0 */
    size_t lLen;
    bool lReturnValue;

    memcpy(lCommandBuffer, sAT_CIPMUX, sizeof(sAT_CIPMUX));
    lLen = sizeof(sAT_CIPMUX);
    lCommandBuffer[lLen++] = '=';

    if(inMultipleConnections) {
        lCommandBuffer[lLen++] = '1';
    } else {
        lCommandBuffer[lLen++] = '0';
    }

    lReturnValue = esp8266_ok_cmd(lCommandBuffer, lLen, 1000);
    if(lReturnValue) {
        sEsp8266.mMultipleConnections = inMultipleConnections;          /* \todo this has to be mutex protected */
    }

    return lReturnValue;
}

bool esp8266_cmd_get_cipmux(bool * outMultipleConnections) {

    uint8_t lCommandBuffer[sizeof(sAT_CIPMUX) + 1];

    uint8_t lReturnBuffer[sizeof(sAT_CIPMUX) - 2 + 1 + 1];     /* "+CIPMUX" + ':' + 0|1 */
    size_t  lReturnLen;

    if(!outMultipleConnections) {
        return false;
    }

    memcpy(lCommandBuffer, sAT_CIPMUX, sizeof(sAT_CIPMUX));
    lCommandBuffer[sizeof(sAT_CIPMUX)] = '?';

    if(esp8266_ok_cmd_str(lCommandBuffer, sizeof(lCommandBuffer), lReturnBuffer, sizeof(lReturnBuffer), &lReturnLen, 1000)) {
        if(memcmp(&sAT_CIPMUX[2], lReturnBuffer, sizeof(sAT_CIPMUX) - 2) == 0) {  /* we found +CIPMUX */
            switch(lReturnBuffer[sizeof(sAT_CIPMUX) - 2 + 1]) {  /* skip +CIPMUX: */
                case '0':
                    *outMultipleConnections = false;
                    break;
                case '1':
                    *outMultipleConnections = true;
                    break;
                default:
                    return false;
            }
            return true;
        }
    }

    return false;
}

bool esp8266_cmd_ping(uint8_t * inAddress, size_t inAddressLength, uint32_t * outResponseTime) {

    uint8_t lCommandBuffer[sizeof(sAT_CIPSTART) + 1 + 1 + 128 + 1];  /* command + '=' + '"' + <address> + '"' */
    size_t lLen;
    uint8_t lReturnBuffer[1 + 6 + 1 + 1];   /* +<time> */
    size_t  lReturnLen;

    if(!outResponseTime) {
        return false;
    }

    if(inAddressLength > 128) {
        dbg("Address is too long\r\n");
        return false;
    }

    memcpy(lCommandBuffer, sAT_PING, sizeof(sAT_PING));
    lLen = sizeof(sAT_PING);
    lCommandBuffer[lLen++] = '=';
    lCommandBuffer[lLen++] = '"';

    memcpy(&lCommandBuffer[lLen], inAddress, inAddressLength);
    lLen += inAddressLength;

    lCommandBuffer[lLen++] = '"';

    if(esp8266_ok_cmd_str(lCommandBuffer, lLen, lReturnBuffer, sizeof(lReturnBuffer)-1, &lReturnLen, 10000)) {

        size_t lCount;
        uint32_t lResponseTime = 0;

        /* decode time */
        for(lCount = 1; isdigit(lReturnBuffer[lCount]) && lCount < lReturnLen; lCount++) {

            lResponseTime = lResponseTime * 10 + lReturnBuffer[lCount] - '0';
        }
        
        *outResponseTime = lResponseTime;
        return true;
    }


    return false;
}

bool esp8266_cmd_cipsta(uint8_t * outIpAddress, size_t inIpAddrMaxLen, size_t * outIpAddrLen) {

    uint8_t lCommandBuffer[sizeof(sAT_CIPSTA) + sizeof(sCUR) + 1];
    size_t lLen;
    uint8_t lReturnBuffer[12 + 3 + 1 + 3 + 1 + 3 + 1 + 3 + 1];  /* +CIPSTA:ip:"192.168.1.35" +CIP */
    size_t lReturnLen;
    size_t lCount;

    memcpy(&lCommandBuffer[0], sAT_CIPSTA, sizeof(sAT_CIPSTA));
    lLen = sizeof(sAT_CIPSTA);

    memcpy(&lCommandBuffer[lLen], sCUR, sizeof(sCUR));

    lCommandBuffer[lLen++] = '?';

    if(esp8266_ok_cmd_str(lCommandBuffer, lLen, lReturnBuffer, sizeof(lReturnBuffer), &lReturnLen, 2000)) {

        for(lCount = 12; lCount < lReturnLen; lCount++) {
            if(lReturnBuffer[lCount] == '"') {

                memcpy(outIpAddress, &lReturnBuffer[12], (lCount - 12 < inIpAddrMaxLen)? (lCount - 12) : inIpAddrMaxLen);
                if(outIpAddrLen) {
                    *outIpAddrLen = (lCount - 12 < inIpAddrMaxLen)? (lCount - 12) : inIpAddrMaxLen;
                }

                return true;
            }
        }
    }

    return false;
}

/*! Allocate a socket

    \param[out] outSocket       Returns the allocated socket

    \retval true    Successful completion
    \retval false   Failed
*/
STATIC bool esp8266_allocate_socket(ts_esp8266_socket ** outSocket) {

    size_t lCount;
    size_t lIndex;
    bool lFound = false;

    if(!outSocket) {
        dbg_err("%s(%d): outSocket is NULL\r\n", __FILE__, __LINE__);
    }

    for(lCount = ESP8266_MAX_CONNECTIONS; lCount > 0; lCount--) {
        lIndex = lCount-1;

        /* try to get the mutex without delay */
        if(xSemaphoreTakeRecursive(sEsp8266.mSockets[lIndex].mMutex, 0)) {

            if(!sEsp8266.mSockets[lIndex].mUsed) {

                /* mark it as used */
                sEsp8266.mSockets[lIndex].mUsed = true;
                lFound = true;
            }

            xSemaphoreGiveRecursive(sEsp8266.mSockets[lIndex].mMutex);
        } else {
            dbg_err("%s(%d): Mutex take failed\r\n", __FILE__, __LINE__);
        }

        if(lFound) {
            *outSocket = &sEsp8266.mSockets[lIndex];
            return true;
        } else if(!sEsp8266.mMultipleConnections) {
            break;
        }
    }

    return false;
}

/*! Verify socket structure

    \param[in]  inSocket    The socket pointer to verify

    \retval true    Valid socket
    \retval false   Invalid socket

*/
STATIC bool isSocket(ts_esp8266_socket * inSocket) {

    size_t lCount;

    if(inSocket == NULL) {
        return false;
    }

    for(lCount = 0; lCount < ESP8266_MAX_CONNECTIONS; lCount++) {
        if(inSocket == &sEsp8266.mSockets[lCount]) {
            return true;
        }
    }

    return false;
}

bool esp8266_cmd_cipstart_tcp(ts_esp8266_socket ** outSocket, uint8_t * inAddress, size_t inAddressLength, uint16_t inPort) {

    ts_esp8266_socket * lSocket;
    uint8_t lCommandBuffer[sizeof(sAT_CIPSTART) + 1 + 1 + 1 + 1 + 3 + 1 + 1 + 1 + 128 + 1 + 1 + 5 + 1];  /* command + '=' + <id> + ',' + '"' + 'TCP' + '"' + ',' + '"' + <address> + '"' + ',' + <port> + \0 for debugging */
    size_t lLen;

    uint8_t lReturnBuffer[sizeof(sALREADY_CON) + 1];
    size_t lReturnLen;

    bool lReturnCode;

    uint32_t lCount;
    uint32_t lPort;
    bool lStarted;

    /* invalid parameter */
    if(outSocket == NULL || inAddress == NULL || inAddressLength == 0) {
        dbg("Parameter's wrong\r\n");
        return false;
    }

    if(inAddressLength > 128) {
        dbg("Address is too long\r\n");
        return false;
    }

    for(;;) {

        /* no more free sockets */
        if(!esp8266_allocate_socket(&lSocket)) {
            dbg("Allocate socket failed\r\n");
            *outSocket = NULL;
            return false;
        }

        lLen = sizeof(sAT_CIPSTART);
        memcpy(lCommandBuffer, sAT_CIPSTART, sizeof(sAT_CIPSTART));
        lCommandBuffer[lLen++] = '=';

        if(sEsp8266.mMultipleConnections) {         /* \todo this has to be mutex protected, until the command sending */
            /* set connection ID */
            lCommandBuffer[lLen++] = lSocket->mSocketId + '0';
            lCommandBuffer[lLen++] = ',';
        }

        /* add "TCP" */
        lCommandBuffer[lLen++] = '"';
        memcpy(&lCommandBuffer[lLen], sTCP, sizeof(sTCP));
        lLen += sizeof(sTCP);
        lCommandBuffer[lLen++] = '"';
        lCommandBuffer[lLen++] = ',';

        /* add "<Address>" */
        lCommandBuffer[lLen++] = '"';
        memcpy(&lCommandBuffer[lLen], inAddress, inAddressLength);
        lLen += inAddressLength;
        lCommandBuffer[lLen++] = '"';
        lCommandBuffer[lLen++] = ',';

        /* add port */
        lPort = inPort;
        lStarted = false;
        for(lCount = 10000; lCount > 0; lCount /= 10) {
            uint32_t lNum = lPort / lCount;
            if(lNum != 0 || lStarted) {
                lCommandBuffer[lLen++] = lNum + '0';
                lStarted = true;
            }

            lPort = lPort % lCount;
        }

        lCommandBuffer[lLen] = '\0';
        dbg("Command buffer is: %s\r\n", lCommandBuffer);

        /* AT+CIPSTART=4,"TCP","www.google.com",80 */
        /* ALREADY CONNECTED\r\n\r\nERROR\r\n */
        /* no ip\r\n */
        /* Link typ ERROR\r\n */
        /* DNS Fail\r\n */
        /* 4,CONNECT\r\n\r\nOK\r\n */

        lReturnCode = esp8266_ok_cmd_str(lCommandBuffer, lLen, lReturnBuffer, sizeof(lReturnBuffer)-1, &lReturnLen, 10000);
        if(!lReturnCode) {

            lReturnBuffer[lReturnLen] = '\0';
            dbg("Returned: \"%s\"\r\n", lReturnBuffer);

            if(lReturnLen == sizeof(lReturnBuffer)) {
                /* check if ALREADY CONNECTED was returned */
                if(memcmp(lReturnBuffer, sALREADY_CON, sizeof(sALREADY_CON)) != 0) {
                    /* something else happened */
                    dbg("Something else -- 1 --\r\n");
                    lSocket->mUsed = false;
                    return false;
                }

                /* aparently, the server was faster */

                /* retry allocation */
                dbg("Retry allocation\r\n");

            } else {
                /* something else happened */
                lSocket->mUsed = false;
                return false;
            }
        } else {
            *outSocket = lSocket;
            return true;
        }
    }
}

bool esp8266_cmd_cipclose(ts_esp8266_socket * inSocket) {

    uint8_t lCommandBuffer[sizeof(sAT_CIPCLOSE) + 1 + 1];       /* command + '=' + <id> */
    size_t lLen = sizeof(sAT_CIPCLOSE);
    bool lCommandReturnCode = false;

    if(!isSocket(inSocket)) {
        dbg("Socket check failed\r\n");
        return false;
    }

    /*
        - give rx semaphore to wake up sleeping task
    */

    if(xSemaphoreTakeRecursive(inSocket->mMutex, portMAX_DELAY)) {

        if(inSocket->mUsed) {

            /* clean this socket up */
            esp8266_socket_cleanup(inSocket);

            memcpy(lCommandBuffer, sAT_CIPCLOSE, sizeof(sAT_CIPCLOSE));
            if(sEsp8266.mMultipleConnections) {         /* \todo this has to be mutex protected, until the command sending */
                lCommandBuffer[lLen++]    = '=';
                lCommandBuffer[lLen++] = inSocket->mSocketId + '0';
            }

            /* do this within the mutex to prevent somebody to use it */
            lCommandReturnCode = esp8266_ok_cmd(lCommandBuffer, lLen, 1000);
            if(!lCommandReturnCode) {
                dbg("Command failed!\r\n");
            }
        } else {
            dbg("Socket is not used\r\n");
        }

        xSemaphoreGiveRecursive(inSocket->mMutex);
    } else {
        dbg_err("%s(%d): inSocket->mMutex Timeout\r\n", __FILE__, __LINE__);
    }

    return lCommandReturnCode;
}

bool esp8266_set_socket_timeout(ts_esp8266_socket * inSocket, uint32_t inTimeoutMS) {

    if(!isSocket(inSocket)) {
        dbg("Socket check failed\r\n");
        return false;
    }

    inSocket->mTimeout = inTimeoutMS;

    return true;
}

bool esp8266_receive(ts_esp8266_socket * inSocket, uint8_t * outBuffer, size_t inBufferMaxLen, size_t * outBufferLen) {

    TickType_t xTicksToWait;
    TimeOut_t xTimeOut;

    size_t lSize;
    size_t lTail;

    bool lTimeout = false;
    bool lRetVal = true;

    size_t lRead = 0;

    if(!isSocket(inSocket)) {
        dbg_err("%s(%d): Socket check failed\r\n", __FILE__, __LINE__);
        return false;
    }

    if(!outBuffer || !outBufferLen) {
        dbg_err("%s(%d): NULL\r\n", __FILE__, __LINE__);
        return false;
    }

    xTicksToWait = inSocket->mTimeout / portTICK_PERIOD_MS;
    vTaskSetTimeOutState( &xTimeOut );

    if(xSemaphoreTakeRecursive(inSocket->mMutex, portMAX_DELAY)) {

        if(inSocket->mUsed) {

            for(lRead = 0; lRead < inBufferMaxLen;) {

                /* get head and tail pointers */
                for(;inSocket->mHead == inSocket->mTail && lRetVal;) {

                    if(!xSemaphoreTake(inSocket->mRxSemaphore, xTicksToWait)) {
                        lTimeout = true;
                        break;  /* timeout */
                    }

                    if(xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait)) {
                        lTimeout = true;
                        break;  /* timeout */
                    }

                    /* socket got closed from external */
                    if(!inSocket->mUsed) {
                        dbg_err("%s(%d): socket %p got closed\r\n", __FILE__, __LINE__, inSocket);
                        lRetVal = false;
                    }
                }

                if(lTimeout) {
                    dbg("%s(%d): Timeout on socket %p\r\n", __FILE__, __LINE__, inSocket);
                    break;
                }

                if(!lRetVal) {
                    break;
                }

                lTail = inSocket->mTail;

                /* get the data of the tail packet */
                lSize = inSocket->mRxBuffer[lTail].mLength - inSocket->mRxBuffer[lTail].mIndex;
                if(lSize > inBufferMaxLen - lRead) {
                    lSize = inBufferMaxLen - lRead;
                }

                /* copy data */
                memcpy(&outBuffer[lRead], &inSocket->mRxBuffer[lTail].mBuffer[inSocket->mRxBuffer[lTail].mIndex], lSize);
                lRead += lSize;
                inSocket->mRxBuffer[lTail].mIndex += lSize;

                /* if we consumed all data in the tail packet, circular increment the buffer */
                if(inSocket->mRxBuffer[lTail].mLength - inSocket->mRxBuffer[lTail].mIndex == 0) {
                    /* circular increment tail pointer */
                    inSocket->mTail = (lTail + 1) & (ESP8266_MAX_CHANNEL_RCV_CNT - 1);
                }
            }

            *outBufferLen = lRead;
        } else {
            dbg_err("%s(%d): Socket %p is not open\r\n", __FILE__, __LINE__, inSocket);
            /* socket is not open! */
            lRetVal = false;
        }

        xSemaphoreGiveRecursive(inSocket->mMutex);
    } else {
        dbg_err("%s(%d): inSocket->mMutex Timeout\r\n", __FILE__, __LINE__);
    }

    dbg("%s(%d): %p Returning: %s\r\n", __FILE__, __LINE__, inSocket, (lRetVal)? "true" : "false");
    return lRetVal;
}

bool esp8266_cmd_cipsend_tcp(ts_esp8266_socket * inSocket, uint8_t * inBuffer, size_t inBufferLen) {

    uint8_t  lCommandBuffer[sizeof(sAT_CIPSEND) + 1 + 1 + 1 + 4];       /* command + '=' + <id> + ',' + <len> */
    size_t   lLen;
    uint32_t lBufferLenStr;
    uint32_t lCount;

    bool lStarted;
    bool lReturnValue = false;

    dbg("%s(%d): sending %d bytes on socket %p\r\n", __FILE__, __LINE__, inBufferLen, inSocket);

    if(!isSocket(inSocket)) {
        dbg_err("%s(%d): Socket check failed\r\n", __FILE__, __LINE__);
        return false;
    }

    if(!inBuffer || inBufferLen > ESP8266_MAX_SEND_LEN) {
        dbg_err("%s(%d): Exceeding max length\r\n", __FILE__, __LINE__);
        return false;
    }

    /* if the buffer to send is 0, do as if it would have been sent */
    if(inBufferLen == 0) {
        return true;
    }

    if(xSemaphoreTakeRecursive(inSocket->mMutex, portMAX_DELAY)) {

        if(inSocket->mUsed) {

            memcpy(lCommandBuffer, sAT_CIPSEND, sizeof(sAT_CIPSEND));
            lLen = sizeof(sAT_CIPSEND);
            lCommandBuffer[lLen++] = '=';
            if(sEsp8266.mMultipleConnections) {
                /* add socket id */
                lCommandBuffer[lLen++] = inSocket->mSocketId + '0';
                lCommandBuffer[lLen++] = ',';
            }

            /* decode length in string */
            lBufferLenStr = inBufferLen;
            lStarted = false;
            for(lCount = 1000; lCount > 0; lCount /= 10) {
                uint32_t lNum = lBufferLenStr / lCount;
                if(lNum != 0 || lStarted) {
                    lCommandBuffer[lLen++] = lNum + '0';
                    lStarted = true;
                }

                lBufferLenStr = lBufferLenStr % lCount;
            }

            /* send command down and wait for "\r\nOK\r\n> " */
            if(xSemaphoreTakeRecursive(sEsp8266.mMutex, portMAX_DELAY)) {

                    sEsp8266.mStatus = ESP8266_IN_PROGRESS;

                    dbg("%s(%d): request OK send\r\n", __FILE__, __LINE__);

                    /* set the sentinel to OK / ERROR */
                    esp8266_set_sentinel(sOK_SEND, sizeof(sOK_SEND), sERROR, sizeof(sERROR));

                    /* set the response buffer */
                    esp8266_set_response_buffer(NULL, 0);

                    /* send command to esp8266 */
                    usart_dma_write(lCommandBuffer, lLen);

                    /* send cr + lf */
                    usart_dma_write(sCRLF, sizeof(sCRLF));

                    /* wait on the answer */
                    if(!xSemaphoreTake(sEsp8266.mResponseSema, 1000 / portTICK_PERIOD_MS)) {
                        esp8266_set_sentinel(NULL, 0, NULL, 0);
                        sEsp8266.mStatus = ESP8266_TIMEOUT;

                        dbg_err("%s(%d): Timeout\r\n", __FILE__, __LINE__);

                    } else if(sEsp8266.mStatus == ESP8266_OK) {

                        uint8_t lBuffer[20];  /* \r\nRecv 1234 bytes\r\n */
                        size_t lBufferLen;

                        lBufferLen = snprintf((char*)lBuffer, sizeof(lBuffer), "\r\nRecv %d bytes\r\n", inBufferLen);

                        dbg("%s(%d): request Recv %d bytes\r\n", __FILE__, __LINE__, inBufferLen);

                        /* now send down the data */
                        sEsp8266.mStatus = ESP8266_IN_PROGRESS;

                        /* set sentinel to Recv 1234 bytes\r\n */
                        esp8266_set_sentinel(lBuffer, lBufferLen, NULL, 0);

//                        /* set the sentinel to OK / ERROR */
//                        esp8266_set_sentinel(sSEND_OK, sizeof(sSEND_OK), sERROR, sizeof(sERROR));

                        /* set the response buffer */
                        esp8266_set_response_buffer(NULL, 0);

                        /* send command to esp8266 */
                        usart_dma_write(inBuffer, inBufferLen);

                        if(!xSemaphoreTake(sEsp8266.mResponseSema, 1000 / portTICK_PERIOD_MS)) {

                            esp8266_set_sentinel(NULL, 0, NULL, 0);
                            sEsp8266.mStatus = ESP8266_TIMEOUT;
                            dbg_err("%s(%d): Timeout: %p\r\n", __FILE__, __LINE__, inSocket);

                        } else if(sEsp8266.mStatus == ESP8266_OK) {

                            {   /* WEP: catch busy s... */

                                sEsp8266.mStatus = ESP8266_IN_PROGRESS;

                                /* set the sentinel to busy s... */
                                esp8266_set_sentinel(sBUSY_S, sizeof(sBUSY_S), NULL, 0);

                                /* set the response buffer */
                                esp8266_set_response_buffer(NULL, 0);

                                /* re-request the rx handler */
                                esp8266_request_rx_handler_refresh();

                                /* receive and clear it ... */
                                if(!xSemaphoreTake(sEsp8266.mResponseSema, 100 / portTICK_PERIOD_MS)) {
                                    esp8266_set_sentinel(NULL, 0, NULL, 0);
                                } else {
                                    dbg_err("%s(%d): busy s... received %p\r\n", __FILE__, __LINE__, inSocket);
                                }
                            }

                            dbg("%s(%d): request ok / err\r\n", __FILE__, __LINE__);

                            /* now wait for OK / ERROR */
                            sEsp8266.mStatus = ESP8266_IN_PROGRESS;

                            /* set the sentinel to OK / ERROR */
                            esp8266_set_sentinel(sSEND_OK, sizeof(sSEND_OK), sERROR, sizeof(sERROR));

                            /* set the response buffer */
                            esp8266_set_response_buffer(NULL, 0);

                            /* should find sSEND_OK */
                            esp8266_request_rx_handler_refresh();

                            if(!xSemaphoreTake(sEsp8266.mResponseSema, 4000 / portTICK_PERIOD_MS)) {

                                /* get 'SEND FAIL' out of the buffer */
                                esp8266_set_sentinel(NULL, 0, sSEND_FAIL, sizeof(sSEND_FAIL));
                                esp8266_request_rx_handler_refresh();
                                if(!xSemaphoreTake(sEsp8266.mResponseSema, 100 / portTICK_PERIOD_MS)) {
                                    dbg_err("%s(%d): Didn't get send fail: %p\r\n", __FILE__, __LINE__, inSocket);
                                    esp8266_set_sentinel(NULL, 0, NULL, 0);
                                } else {
                                    dbg_err("%s(%d): Send fail [%d] %p\r\n", __FILE__, __LINE__, inSocket->mSocketId, inSocket);
                                }

                                sEsp8266.mStatus = ESP8266_TIMEOUT;
                                dbg_err("%s(%d): Timeout: %p\r\n", __FILE__, __LINE__, inSocket);

                            } else if(sEsp8266.mStatus == ESP8266_OK) {

                                lReturnValue = true;
                                dbg("%s(%d): OK\r\n", __FILE__, __LINE__);
                            } else {
                                dbg_err("%s(%d): Status was %d\r\n", __FILE__, __LINE__, sEsp8266.mStatus);
                            }
                        } else {
                            dbg_err("%s(%d): Status was %d\r\n", __FILE__, __LINE__, sEsp8266.mStatus);
                        }
                    } else {
                        dbg_err("%s(%d): Status was %d\r\n", __FILE__, __LINE__, sEsp8266.mStatus);
                    }

                xSemaphoreGiveRecursive(sEsp8266.mMutex);
            } else {
                dbg_err("%s(%d): Mutex take failed\r\n", __FILE__, __LINE__);
            }
        } else {
            dbg_err("%s(%d): Socket %p is not allocated\r\n", __FILE__, __LINE__, inSocket);
        }

        xSemaphoreGiveRecursive(inSocket->mMutex);
    } else {
        dbg_err("%s(%d): inSocket->mMutex Timeout\r\n", __FILE__, __LINE__);
    }

    return lReturnValue;
}

bool esp8266_cmd_cipserver(uint16_t inPort, t_esp8266_server_handler inServerHandler, uint32_t inServerTaskPriority, uint32_t inServerTaskStackSize) {

    uint8_t lCommandBuffer[sizeof(sAT_CIPSERVER) + 1 + 1 + 1 + 5];  /* command + '=' + '1' + ',' + <port> + \0 for debugging */
    size_t lLen;

    uint32_t lCount;
    uint32_t lPort;
    bool lStarted;
    bool lReturnValue;

    if(!sEsp8266.mMultipleConnections) {
        return false;
    }

    /* can only have one server running */
    if(sEsp8266.mServerHandler != NULL) {
        return false;
    }

    /* there's no handler specified, which is an invalid operation */
    if(inServerHandler == NULL) {
        return false;
    }

    if(inServerTaskPriority >= configMAX_PRIORITIES) {
        return false;
    }

    memcpy(lCommandBuffer, sAT_CIPSERVER, sizeof(sAT_CIPSERVER));
    lLen = sizeof(sAT_CIPSERVER);
    lCommandBuffer[lLen++] = '=';
    lCommandBuffer[lLen++] = '1';   /* create server */
    lCommandBuffer[lLen++] = ',';

    /* add port */
    lPort = inPort;
    lStarted = false;
    for(lCount = 10000; lCount > 0; lCount /= 10) {
        uint32_t lNum = lPort / lCount;
        if(lNum != 0 || lStarted) {
            lCommandBuffer[lLen++] = lNum + '0';
            lStarted = true;
        }

        lPort = lPort % lCount;
    }

    /* AT+CIPSERVER=1,23 */

    lReturnValue = esp8266_ok_cmd(lCommandBuffer, lLen, 1000);
    if(lReturnValue) {
        sEsp8266.mServerHandler       = inServerHandler;
        sEsp8266.mServerTaskPriority  = inServerTaskPriority;
        sEsp8266.mServerTaskStackSize = inServerTaskStackSize;
    }

    return lReturnValue;
}

/* eof */
