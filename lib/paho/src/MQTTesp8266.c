#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "MQTTesp8266.h"

#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

static int FreeRTOS_esp8266_read(Network* n, unsigned char* buffer, int len, int timeout_ms);
static int FreeRTOS_esp8266_write(Network* n, unsigned char* buffer, int len, int timeout_ms);
static void FreeRTOS_esp8266_disconnect(Network* n);


void NetworkInit(Network* n) {
    n->mSocket    = NULL;
    n->mqttread   = FreeRTOS_esp8266_read;
    n->mqttwrite  = FreeRTOS_esp8266_write;
    n->disconnect = FreeRTOS_esp8266_disconnect;
}

int NetworkConnect(Network* n, char* addr, int port) {

    ts_esp8266_socket * lSocket;

    if(esp8266_cmd_cipstart_tcp(&lSocket, (uint8_t*)addr, strlen(addr), port)) {

        n->mSocket = lSocket;

        return true;
    }

    return false;
}


static int FreeRTOS_esp8266_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    TickType_t xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
    TimeOut_t xTimeOut;
    int recvLen = 0;

    vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */
    do
    {
        size_t lReceiveCount;

        esp8266_set_socket_timeout(n->mSocket, timeout_ms);
        if(esp8266_receive(n->mSocket, (uint8_t*)(buffer + recvLen), len - recvLen, &lReceiveCount)) {
            recvLen += lReceiveCount;
        } else {
            /* return error */
            recvLen = -1;
            break;
        }

    } while (recvLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

    return recvLen;
}


static int FreeRTOS_esp8266_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    TickType_t xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
    TimeOut_t xTimeOut;
    int sentLen = 0;

    vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */
    do
    {
        if(esp8266_cmd_cipsend_tcp(n->mSocket, (uint8_t*)buffer, len)) {
            sentLen = len;
        } else {
            sentLen = -1;
            break;
        }
    } while (sentLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

    return sentLen;
}


static void FreeRTOS_esp8266_disconnect(Network* n) {

    esp8266_cmd_cipclose(n->mSocket);

    n->mSocket = NULL;
}






/* eof */
