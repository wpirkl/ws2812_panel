
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

const uint8_t sCRLF[]   = { '\r', '\n' };

const uint8_t sAT[]     = { 'A', 'T'};
const uint8_t sATE0[]   = { 'A', 'T', 'E', '0' };
const uint8_t sAT_RST[] = { 'A', 'T', '+', 'R', 'S', 'T' };
const uint8_t sAT_GMR[] = { 'A', 'T', '+', 'G', 'M', 'R' };

/* Wifi commands */
const uint8_t sAT_CWMODE_CUR[] = { 'A', 'T', '+', 'C', 'W', 'M', 'O', 'D', 'E', '_', 'C', 'U', 'R', '=', '0' };
const uint8_t sAT_CWJAP_CUR[]  = { 'A', 'T', '+', 'C', 'W', 'J', 'A', 'P', '_', 'C', 'U', 'R', '=' };
const uint8_t sAT_CWLAP[]      = { 'A', 'T', '+', 'C', 'W', 'L', 'A', 'P' };
const uint8_t sAT_CWQAP[]      = { 'A', 'T', '+', 'C', 'W', 'Q', 'A', 'P' };
const uint8_t sAT_CWSAP_CUR[]  = { 'A', 'T', '+', 'C', 'W', 'S', 'A', 'P', '_', 'C', 'U', 'R', '=' };
const uint8_t sAT_CWLIF[]      = { 'A', 'T', '+', 'C', 'W', 'L', 'I', 'F' };
const uint8_t sAT_CWDHCP_CUR[] = { 'A', 'T', '+', 'C', 'W', 'D', 'H', 'C', 'P', '_', 'C', 'U', 'R', '=' };

const uint8_t sOK[]    = { '\r', '\n', 'O', 'K', '\r', '\n'};
const uint8_t sERROR[] = { '\r', '\n', 'E', 'R', 'R', 'O', 'R', '\r', '\n' };
const uint8_t sREADY[] = { 'r', 'e', 'a', 'd', 'y' };

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
#endif /* ESP8266_FREERTOS */

    /*! Head index into receive buffer ring */
    size_t          mHead;

    /*! Tail index into receive buffer ring */
    size_t          mTail;

    /*! Receive data ring buffer */
    ts_rx_packet    mRxBuffer[ESP8266_MAX_CHANNEL_RCV_CNT];
};

/*! Defines the esp8266 object
*/
typedef struct s_esp8266 {
#if defined(ESP8266_FREERTOS)
    /*! Semaphore to notify caller */
    SemaphoreHandle_t           mResponseSema;

    /*! Mutex to allow only one caller at a time */
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
            sEsp8266.mSockets[lCount].mRxSemaphore = xSemaphoreCreateCounting(1, 0);
            assert_param(sEsp8266.mSockets[lCount].mRxSemaphore != NULL);
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

        /* decode channel */
        lLen = usart_dma_read_until(&lNumberBuffer[0], sizeof(lNumberBuffer)-1, ',');
        lNumberBuffer[lLen] = '\0';

        lChannel = strtoul((char*)lNumberBuffer, NULL, 0);

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

            /* assure the caller forsees enough memory */
            assert_param(lSize - inSentinelLen <= sEsp8266.mResponseLen);

            /* lSize - inSentinelLen <= sEsp8266.mResponseLen */
            sEsp8266.mResponseRcvLen = usart_dma_read(sEsp8266.mResponseBuffer, lSize - inSentinelLen);

            /* Exclude sentinel */
            usart_dma_rx_skip(inSentinelLen);

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

    if(sEsp8266.mSentinel != NULL && sEsp8266.mSentinelLen > 0) {

        bool lReturnValue = false;

        lReturnValue = esp8266_rx_handle_command_response_gen(sEsp8266.mSentinel, sEsp8266.mSentinelLen);
        if(lReturnValue) {
            /* good response found */
            esp8266_set_sentinel(NULL, 0, NULL, 0);

            sEsp8266.mStatus = ESP8266_OK;
        }

        return lReturnValue;
    }

    if(sEsp8266.mErrorSentinel != NULL && sEsp8266.mErrorSentinelLen > 0) {
        bool lReturnValue = false;

        lReturnValue = esp8266_rx_handle_command_response_gen(sEsp8266.mErrorSentinel, sEsp8266.mErrorSentinelLen);
        if(lReturnValue) {
            /* error found */
            esp8266_set_sentinel(NULL, 0, NULL, 0);

            sEsp8266.mStatus = ESP8266_ERROR;
        }

        return lReturnValue;
    }

    return false;
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
        lTreated = lTreated || esp8266_rx_handle_ipd();

        /* handle WIFI ... messages */
        lTreated = lTreated || esp8266_rx_handle_wifi();

        /* handle socket connect / close */
        lTreated = lTreated || esp8266_rx_handle_socket();

        /* handle command response */
        lTreated = lTreated || esp8266_rx_handle_command_response();

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

/*! Execute a command which just response OK or ERROR

    \param[in]  inCommand       The command string to send
    \param[in]  inCommandLen    The length of the command string

    \retval true        OK was returned
    \retval false       something else was returned
*/
static bool esp8266_ok_cmd(const uint8_t * const inCommand, size_t inCommandLen) {

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
        xSemaphoreTake(sEsp8266.mResponseSema, portMAX_DELAY);
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
static bool esp8266_ok_cmd_str(const uint8_t * const inCommand, size_t inCommandLen, uint8_t * inOutData, size_t inDataMaxLen, size_t * outDataLen) {

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
        xSemaphoreTake(sEsp8266.mResponseSema, portMAX_DELAY);
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

bool esp8266_cmd_at(void) {

    return esp8266_ok_cmd(sAT, sizeof(sAT));
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
        xSemaphoreTake(sEsp8266.mResponseSema, portMAX_DELAY);
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

    return esp8266_ok_cmd_str(sAT_GMR, sizeof(sAT_GMR), outVersionInfo, inVersionInfoMaxLen, outVersionInfoLen);
}

bool esp8266_cmd_ate0(void) {

    return esp8266_ok_cmd(sATE0, sizeof(sATE0));
}

bool esp8266_cmd_set_cwmode_cur(te_esp8266_wifi_mode inWifiMode) {

    uint8_t lCommandBuffer[sizeof(sAT_CWMODE_CUR)];

    memcpy(lCommandBuffer, sAT_CWMODE_CUR, sizeof(sAT_CWMODE_CUR) - 1);

    switch(inWifiMode) {
        case ESP8266_WIFI_MODE_STATION:
            lCommandBuffer[sizeof(sAT_CWMODE_CUR)-1] = '1';
            break;
        case ESP8266_WIFI_MODE_AP:
            lCommandBuffer[sizeof(sAT_CWMODE_CUR)-1] = '2';
            break;
        case ESP8266_WIFI_MODE_STA_AP:
            lCommandBuffer[sizeof(sAT_CWMODE_CUR)-1] = '3';
            break;
        default:
            return false;
    }

    return esp8266_ok_cmd(lCommandBuffer, sizeof(lCommandBuffer));
}

bool esp8266_cmd_set_cwjap_cur(uint8_t * inSSID, size_t inSSIDLen, uint8_t * inPWD, size_t inPWDLen) {

    uint8_t lCommandBuffer[sizeof(sAT_CWJAP_CUR) + 1 + 31 + 1 + 1 + 1 + 64 + 1];    /* command length + quote + ssid + quote + comma + quote + password lenght + quote */
    size_t  lLen = sizeof(sAT_CWJAP_CUR);

    if(inSSIDLen > 31 || inPWDLen > 64) {
        return false;
    }

    memcpy(lCommandBuffer, sAT_CWJAP_CUR, sizeof(sAT_CWJAP_CUR));

    lCommandBuffer[lLen++] = '"';
    memcpy(&lCommandBuffer[lLen], inSSID, inSSIDLen);
    lLen += inSSIDLen;
    lCommandBuffer[lLen++] = '"';
    lCommandBuffer[lLen++] = ',';
    lCommandBuffer[lLen++] = '"';
    memcpy(&lCommandBuffer[lLen], inPWD, inPWDLen);
    lCommandBuffer[lLen++] = '"';

    return esp8266_ok_cmd(lCommandBuffer, lLen);
}

bool esp8266_cmd_get_cwlap(uint8_t * outAccessPointList, size_t inAccessPointListMaxLen, size_t * outAccessPointListLen) {

    return esp8266_ok_cmd_str(sAT_CWLAP, sizeof(sAT_CWLAP), outAccessPointList, inAccessPointListMaxLen, outAccessPointListLen);
}

bool esp8266_cmd_cwqap(void) {

    return esp8266_ok_cmd(sAT_CWQAP, sizeof(sAT_CWQAP));
}

bool esp8266_cmd_set_cwsap_cur(uint8_t * inSSID, size_t inSSIDLen, uint8_t * inPWD, size_t inPWDLen, uint8_t inChannel, te_esp8266_encryption_mode inEncryption) {

    uint8_t lCommandBuffer[sizeof(sAT_CWSAP_CUR) + 1 + 31 + 1 + 1 + 1 + 64 + 1 + 1 + 2 + 1 + 1 ];    /* command length + quote + ssid + quote + comma + quote + password lenght + quote + comma + channel + comma + enc mode */
    size_t  lLen = sizeof(sAT_CWSAP_CUR);

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

    memcpy(lCommandBuffer, sAT_CWSAP_CUR, sizeof(sAT_CWSAP_CUR));

    lCommandBuffer[lLen++] = '"';
    memcpy(&lCommandBuffer[lLen], inSSID, inSSIDLen);
    lLen += inSSIDLen;
    lCommandBuffer[lLen++] = '"';
    lCommandBuffer[lLen++] = ',';
    lCommandBuffer[lLen++] = '"';
    memcpy(&lCommandBuffer[lLen], inPWD, inPWDLen);
    lCommandBuffer[lLen++] = '"';

    if(inChannel >= 10) {
        lCommandBuffer[lLen++] = '1';
    }
    lCommandBuffer[lLen++] = '0' + (inChannel % 10);
    lCommandBuffer[lLen++] = ',';
    lCommandBuffer[lLen++] = lMode;

    return esp8266_ok_cmd(lCommandBuffer, lLen);
}

bool esp8266_cmd_get_cwlif(uint8_t * outStationList, size_t inStationListMaxLen, size_t * outStationListLen) {

    return esp8266_ok_cmd_str(sAT_CWLIF, sizeof(sAT_CWLIF), outStationList, inStationListMaxLen, outStationListLen);
}

bool esp8266_cmd_set_cwdhcp(te_esp8266_dhcp_mode inDHCPMode, bool inEnable) {

    uint8_t lCommandBuffer[sizeof(sAT_CWDHCP_CUR) + 1 + 1 + 1];  /* command + mode + comma + ena/disa */
    size_t  lLen = sizeof(sAT_CWDHCP_CUR);
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

    memcpy(lCommandBuffer, sAT_CWDHCP_CUR, sizeof(sAT_CWDHCP_CUR));

    lCommandBuffer[lLen++] = lMode;
    lCommandBuffer[lLen++] = ',';
    if(inEnable) {
        lCommandBuffer[lLen++] = '1';
    } else {
        lCommandBuffer[lLen++] = '0';
    }

    return esp8266_ok_cmd(lCommandBuffer, lLen);
}

/* eof */
