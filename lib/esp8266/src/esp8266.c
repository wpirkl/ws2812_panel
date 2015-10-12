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


#define ESP8266_MAX_CONNECTIONS         (5)
#define ESP8266_MAX_RESPONSE_SIZE       (128)
#define ESP8266_MAX_CHANNEL_RCV_SIZE    (2048)
#define ESP8266_MAX_CHANNEL_RCV_CNT     (4)

#define ESP8266_FREERTOS

#if defined(ESP8266_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#endif /* ESP8266_FREERTOS */

const uint8_t sCRLF[]   = { '\r', '\n' };

/* General commands */
const uint8_t sAT[]     = { 'A', 'T' };
const uint8_t sATE0[]   = { 'A', 'T', 'E', '0' };
const uint8_t sAT_RST[] = { 'A', 'T', '+', 'R', 'S', 'T' };
const uint8_t sAT_GMR[] = { 'A', 'T', '+', 'G', 'M', 'R' };

/* Wifi commands */
const uint8_t sAT_CWMODE[]          = { 'A', 'T', '+', 'C', 'W', 'M', 'O', 'D', 'E' };
const uint8_t sAT_CWJAP[]           = { 'A', 'T', '+', 'C', 'W', 'J', 'A', 'P' };
const uint8_t sAT_CWLAP[]           = { 'A', 'T', '+', 'C', 'W', 'L', 'A', 'P' };
const uint8_t sAT_CWQAP[]           = { 'A', 'T', '+', 'C', 'W', 'Q', 'A', 'P' };
const uint8_t sAT_CWSAP[]           = { 'A', 'T', '+', 'C', 'W', 'S', 'A', 'P' };
const uint8_t sAT_CWLIF[]           = { 'A', 'T', '+', 'C', 'W', 'L', 'I', 'F' };
const uint8_t sAT_CWDHCP[]          = { 'A', 'T', '+', 'C', 'W', 'D', 'H', 'C', 'P' };

const uint8_t sCUR[]                = { '_', 'C', 'U', 'R' };

/* TCP/UDP commands */
const uint8_t sAT_CIPMUX[]   = { 'A', 'T', '+', 'C', 'I', 'P', 'M', 'U', 'X' };
const uint8_t sAT_CIPSTART[] = { 'A', 'T', '+', 'C', 'I', 'P', 'S', 'T', 'A', 'R', 'T' };
const uint8_t sAT_CIPCLOSE[] = { 'A', 'T', '+', 'C', 'I', 'P', 'C', 'L', 'O', 'S', 'E' };
const uint8_t sAT_CIPSEND[]  = { 'A', 'T', '+', 'C', 'I', 'P', 'S', 'E', 'N', 'D' };

const uint8_t sOK[]      = { '\r', '\n', 'O', 'K', '\r', '\n'};
const uint8_t sERROR[]   = { '\r', '\n', 'E', 'R', 'R', 'O', 'R', '\r', '\n' };
const uint8_t sFAIL[]    = { '\r', '\n', 'F', 'A', 'I', 'L', '\r', '\n' };
const uint8_t sREADY[]   = { 'r', 'e', 'a', 'd', 'y', '\r', '\n' };
const uint8_t sTCP[]     = { 'T', 'C', 'P' };
const uint8_t sOK_SEND[] = { '\r', '\n', 'O', 'K', '\r', '\n', '>', ' ' };
const uint8_t sSEND_OK[] = { '\r', '\n', 'S', 'E', 'N', 'D', ' ', 'O', 'K', '\r', '\n' };

const uint8_t sALREADY_CON[] = { 'A', 'L', 'R', 'E', 'A', 'D', 'Y', ' ', 'C', 'O', 'N', 'N', 'E', 'C', 'T', 'E', 'D', '\r', '\n' };

/* forward declarations */

static void esp8266_set_sentinel(const uint8_t * const inSentinel, size_t inSentinelLen, const uint8_t * const inErrorSentinel, size_t inErrorSentinelLen);


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
    uint8_t        mSocketId;

    /*! Defines if this socket is in use */
    bool            mUsed;

    /*! Defines the socket timeout in ms */
    uint32_t        mTimeout;

    /*! Head index into receive buffer ring */
    volatile size_t mHead;

    /*! Tail index into receive buffer ring */
    volatile size_t mTail;

    /*! Receive data ring buffer */
    ts_rx_packet    mRxBuffer[ESP8266_MAX_CHANNEL_RCV_CNT];
};

/*! Defines the esp8266 object
*/
typedef struct s_esp8266 {
#if defined(ESP8266_FREERTOS)
    /*! Semaphore to notify caller */
    SemaphoreHandle_t           mResponseSema;

    /*! Mutex to allow only one caller at a time -- I/O related */
    SemaphoreHandle_t           mMutex;

#endif /* ESP8266_FREERTOS */

    /*! Number of treated characters in the rx buffer */
    size_t                      mTreated;

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

    /*! Socket objects */
    struct s_esp8266_socket     mSockets[ESP8266_MAX_CONNECTIONS];
} ts_esp8266;

/*! Local object */
static ts_esp8266               sEsp8266;


/*  ---- functions ---- */

void esp8266_init(void) {

    usart_dma_open();

    /* reset the object */
    memset(&sEsp8266, 0, sizeof(sEsp8266));

#if defined(ESP8266_FREERTOS)
    {
        size_t lCount;

        sEsp8266.mResponseSema = xSemaphoreCreateCounting(1, 0);
        assert_param(sEsp8266.mResponseSema != NULL);

        sEsp8266.mMutex = xSemaphoreCreateRecursiveMutex();
        assert_param(sEsp8266.mMutex != NULL);


        for(lCount = 0; lCount < ESP8266_MAX_CONNECTIONS; lCount++) {

            sEsp8266.mSockets[lCount].mSocketId = lCount;
            sEsp8266.mSockets[lCount].mTimeout  = 1000;

            sEsp8266.mSockets[lCount].mRxSemaphore = xSemaphoreCreateCounting(1, 0);
            assert_param(sEsp8266.mSockets[lCount].mRxSemaphore != NULL);

            sEsp8266.mSockets[lCount].mMutex = xSemaphoreCreateRecursiveMutex();
            assert_param(sEsp8266.mSockets[lCount].mMutex != NULL);
        }
    }
#else /* ESP8266_FREERTOS */
#endif /* ESP8266_FREERTOS */
}

/*!
    This function handles asynchronous incoming data
*/
static bool esp8266_rx_handle_ipd(void) {

    size_t lCount;
    size_t lChannel;
    size_t lRcvLen;
    size_t lLen;

    uint8_t * lPtr;

    uint8_t lNumberBuffer[12];

    if(usart_dma_peek((uint8_t*)"\r\n+IPD,", 7)) {

        /* we already know it start's with \r\n+IPD, */
        usart_dma_rx_skip(7);

        if(sEsp8266.mMultipleConnections) {
            /* decode channel */
            lLen = usart_dma_read_until(&lNumberBuffer[0], sizeof(lNumberBuffer)-1, ',');
            lNumberBuffer[lLen] = '\0';
            lChannel = strtoul((char*)lNumberBuffer, NULL, 0);
        } else {
            lChannel = 4;   /* 4 is default channel on single channel connections */
        }

        assert_param(lChannel < ESP8266_MAX_CONNECTIONS);

        /* get data size */
        lLen = usart_dma_read_until(&lNumberBuffer[0], sizeof(lNumberBuffer)-1, ':');
        lNumberBuffer[lLen] = '\0';

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

#if defined(ESP8266_FREERTOS)
        /* notify socket */
        xSemaphoreGive(sEsp8266.mSockets[lChannel].mRxSemaphore);
#endif /* ESP8266_FREERTOS */

        return true;
    }
    return false;
}

/*!
    This function handles WIFI asynchronous messages
    \todo implement
*/
static bool esp8266_rx_handle_wifi(void) {

    if(usart_dma_match((uint8_t*)"WIFI DISCONNECT\r\n", 17)) {
        
        usart_dma_rx_skip_until('\n');
        return true;
    }
    if(usart_dma_match((uint8_t*)"WIFI CONNECTED\r\n", 16)) {
        
        usart_dma_rx_skip_until('\n');
        return true;
    }
    if(usart_dma_match((uint8_t*)"WIFI GOT IP\r\n", 13)) {
        /* we alrady know it starts with "WIFI " */
        /* usart_dma_rx_skip(5); */

        usart_dma_rx_skip_until('\n');
        return true;
    }
    return false;
}

/*!
    This function handles x,CLOSED or X,CONNECT asynchronous messages
    \todo implement
*/
static bool esp8266_rx_handle_socket(void) {

    if(usart_dma_match((uint8_t*)",CONNECT\r\n", 10) > 0) {

        usart_dma_rx_skip_until('\n');
        return true;
    }
    if(usart_dma_match((uint8_t*)",CLOSED\r\n", 9) > 0) {

        usart_dma_rx_skip_until('\n');
        return true;
    }

    return false;
}

/*!
    Handles generic command responses

    \param[in]  inSentinel      The sentinel to match
    \param[in]  inSentinelLen   The length of the sentinel

    \retval true    Sentinel found and treated
    \retval false   Sentinel not found
*/
static bool esp8266_rx_handle_command_response_gen(const uint8_t * const inSentinel, size_t inSentinelLen) {

    /* received command response */
    size_t lSize = usart_dma_match(inSentinel, inSentinelLen);
    if(lSize > 0) {

        if(sEsp8266.mResponseBuffer == NULL || sEsp8266.mResponseLen == 0) {

            /* nobody want's the data, so drop it */
            usart_dma_rx_skip(lSize);
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

/*!
    This function handles incoming command responses
*/
static bool esp8266_rx_handle_command_response(void) {

    bool lReturnValue = false;

    if(sEsp8266.mSentinel != NULL && sEsp8266.mSentinelLen > 0) {


        lReturnValue = esp8266_rx_handle_command_response_gen(sEsp8266.mSentinel, sEsp8266.mSentinelLen);
        if(lReturnValue) {
            /* good response found */
            esp8266_set_sentinel(NULL, 0, NULL, 0);

            sEsp8266.mStatus = ESP8266_OK;

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

            return true;
        }
    }

    return lReturnValue;
}

void esp8266_rx_handler(void) {

    bool lTreated;

#if defined(ESP8266_FREERTOS)
    for(;usart_dma_rx_num() == sEsp8266.mTreated;) {

        /* only wait if there's data we didn't treat before */
        usart_dma_rx_wait();
    }
#endif /* ESP8266_FREERTOS */

    do {
        lTreated = false;

        /* handle socket receive */
        lTreated = esp8266_rx_handle_ipd() || lTreated;

        /* handle WIFI ... messages */
//        lTreated = esp8266_rx_handle_wifi() || lTreated;

        /* handle socket connect / close */
        lTreated = esp8266_rx_handle_socket() || lTreated;

        /* handle command response */
        lTreated = esp8266_rx_handle_command_response() || lTreated;

    } while(lTreated);  /* treat as long as we can */

    /* update treated */
    sEsp8266.mTreated = usart_dma_rx_num();
}

void esp8266_setup(void) {

    
}

/*! Retreive the length of the response

    \return the length of the response string, excluding the terminating NULL character
*/
static size_t getResponseLength(void) {

    return sEsp8266.mResponseRcvLen;
}

/*! This function sets the response buffer

    \param[in]  inResponseBuffer      The sentinel string
    \param[in]  inResponseBufferLen   The length of the sentinel string
*/
static void esp8266_set_response_buffer(uint8_t * inResponseBuffer, size_t inResponseBufferLen) {

    sEsp8266.mResponseBuffer = inResponseBuffer;
    sEsp8266.mResponseLen    = inResponseBufferLen;
    sEsp8266.mResponseRcvLen = 0;
}

/*! This function sets the sentinel

    \param[in]  inSentinel      The sentinel string
    \param[in]  inSentinelLen   The length of the sentinel string
*/
static void esp8266_set_sentinel(const uint8_t * const inSentinel, size_t inSentinelLen, const uint8_t * const inErrorSentinel, size_t inErrorSentinelLen) {

    sEsp8266.mSentinel    = inSentinel;
    sEsp8266.mSentinelLen = inSentinelLen;

    sEsp8266.mErrorSentinel    = inErrorSentinel;
    sEsp8266.mErrorSentinelLen = inErrorSentinelLen;
}

#undef dbg
#define dbg dbg_off

/*! Execute a command which just response OK or ERROR

    \param[in]  inCommand       The command string to send
    \param[in]  inCommandLen    The length of the command string

    \retval true        OK was returned
    \retval false       something else was returned
*/
static bool esp8266_ok_cmd(const uint8_t * const inCommand, size_t inCommandLen, uint32_t inTimeoutMs) {

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
static bool esp8266_ok_cmd_fail(const uint8_t * const inCommand, size_t inCommandLen, uint32_t inTimeoutMs) {

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
static bool esp8266_ok_cmd_str(const uint8_t * const inCommand, size_t inCommandLen, uint8_t * inOutData, size_t inDataMaxLen, size_t * outDataLen, uint32_t inTimeoutMs) {

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
    }
#endif

    return lReturnValue;
}

#undef dbg
#define dbg dbg_off

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

bool esp8266_cmd_set_cwsap_cur(uint8_t * inSSID, size_t inSSIDLen, uint8_t * inPWD, size_t inPWDLen, uint8_t inChannel, te_esp8266_encryption_mode inEncryption) {

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
        lCommandBuffer[lLen++] = '1';
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

#undef dbg
#define dbg dbg_off

/*! Allocate a socket

    \param[out] outSocket       Returns the allocated socket

    \retval true    Successful completion
    \retval false   Failed
*/
static bool esp8266_allocate_socket(ts_esp8266_socket ** outSocket) {

    size_t lCount;
    size_t lIndex;
    bool lFound = false;

    assert_param(outSocket != NULL);

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
static bool isSocket(ts_esp8266_socket * inSocket) {

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
    uint8_t lCommandBuffer[sizeof(sAT_CIPSTART) + 1 + 1 + 1 + 1 + 3 + 1 + 1 + 1 + 128 + 1 + 1 + 5];  /* command + '=' + <id> + ',' + '"' + 'TCP' + '"' + ',' + '"' + <address> + '"' + ',' + <port> + \0 for debugging */
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

        /* AT+CIPSTART=4,"TCP","www.google.com",80 */
        /* ALREADY CONNECTED\r\n\r\nERROR\r\n */
        /* no ip\r\n */
        /* Link typ ERROR\r\n */
        /* DNS Fail\r\n */

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

            /* mark it as unused */
            inSocket->mUsed = false;

            memcpy(lCommandBuffer, sAT_CIPCLOSE, sizeof(sAT_CIPCLOSE));
            if(sEsp8266.mMultipleConnections) {         /* \todo this has to be mutex protected, until the command sending */
                lCommandBuffer[lLen++]    = '=';
                lCommandBuffer[lLen++] = inSocket->mSocketId + '0';
            }

            /* do this within the mutex to prevent somebody to use it */
            lCommandReturnCode = esp8266_ok_cmd(lCommandBuffer, lLen, 1000);
        } else {
            dbg("Socket is not used\r\n");
        }

        xSemaphoreGiveRecursive(inSocket->mMutex);
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

    size_t lRead = 0;

    if(!isSocket(inSocket)) {
        dbg("Socket check failed\r\n");
        return false;
    }

    xTicksToWait = inSocket->mTimeout / portTICK_PERIOD_MS;
    vTaskSetTimeOutState( &xTimeOut );

    if(xSemaphoreTakeRecursive(inSocket->mMutex, portMAX_DELAY)) {

        if(inSocket->mUsed) {

            for(lRead = 0; lRead < inBufferMaxLen;) {

                /* get head and tail pointers */
                for(;inSocket->mHead == inSocket->mTail;) {

                    if(!xSemaphoreTake(inSocket->mRxSemaphore, xTicksToWait)) {
                        lTimeout = true;
                        break;  /* timeout */
                    }

                    if(xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait)) {
                        lTimeout = true;
                        break;  /* timeout */
                    }
                }

                if(lTimeout) {
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
        }

        xSemaphoreGiveRecursive(inSocket->mMutex);
    }

    return true;
}

bool esp8266_cmd_cipsend_tcp(ts_esp8266_socket * inSocket, uint8_t * inBuffer, size_t inBufferLen) {

    uint8_t  lCommandBuffer[sizeof(sAT_CIPSEND) + 1 + 1 + 1 + 4];       /* command + '=' + <id> + ',' + <len> */
    size_t   lLen;
    uint32_t lBufferLenStr;
    uint32_t lCount;

    bool lStarted;
    bool lReturnValue = false;

    if(!isSocket(inSocket)) {
        dbg("Socket check failed\r\n");
        return false;
    }

    if(xSemaphoreTakeRecursive(inSocket->mMutex, portMAX_DELAY)) {

        if(inSocket->mUsed) {

            memcpy(lCommandBuffer, sAT_CIPSEND, sizeof(sAT_CIPSEND));
            lLen = sizeof(sAT_CIPSEND);
            lCommandBuffer[lLen++] = '=';
            if(sEsp8266.mMultipleConnections) {
                /* add socket id */
                lCommandBuffer[lLen++] = inSocket->mSocketId;
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

                        dbg("Timeout\r\n");

                    } else if(sEsp8266.mStatus == ESP8266_OK) {

                        /* now send down the data */
                        sEsp8266.mStatus = ESP8266_IN_PROGRESS;

                        /* set the sentinel to OK / ERROR */
                        esp8266_set_sentinel(sSEND_OK, sizeof(sSEND_OK), sERROR, sizeof(sERROR));

                        /* set the response buffer */
                        esp8266_set_response_buffer(NULL, 0);

                        /* send command to esp8266 */
                        usart_dma_write(inBuffer, inBufferLen);

                        if(!xSemaphoreTake(sEsp8266.mResponseSema, 1000 / portTICK_PERIOD_MS)) {
                            esp8266_set_sentinel(NULL, 0, NULL, 0);
                            sEsp8266.mStatus = ESP8266_TIMEOUT;
                        } else {
                            lReturnValue = true;
                        }
                    } else {
                        dbg("Status was %d\r\n", sEsp8266.mStatus);
                    }

                xSemaphoreGiveRecursive(sEsp8266.mMutex);
            }
        } else {
            dbg("Socket is not allocated\r\n");
        }

        xSemaphoreGiveRecursive(inSocket->mMutex);
    }

    return lReturnValue;
}

/* eof */
