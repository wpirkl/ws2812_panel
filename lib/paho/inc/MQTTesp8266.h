#ifndef MQTT_ESP8266_H_
#define MQTT_ESP8266_H_

#include <stdint.h>
#include <stddef.h>

#include "esp8266.h"

typedef struct Network Network;

struct Network {
    ts_esp8266_socket * mSocket;
    int (*mqttread) (Network*, unsigned char*, int, int);
    int (*mqttwrite) (Network*, unsigned char*, int, int);
    void (*disconnect) (Network*);
};


void NetworkInit(Network* n);
int NetworkConnect(Network* n, char* addr, int port);


#endif /* MQTT_ESP8266_H_ */

/* eof */
