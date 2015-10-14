#ifndef MQTT_ESP8266_H_
#define MQTT_ESP8266_H_

#include <stdint.h>
#include <stddef.h>

#include "esp8266.h"

typedef struct Network Network;

struct Network {
    int (*mqttread) (Network*, unsigned char*, int, int);
    int (*mqttwrite) (Network*, unsigned char*, int, int);
};


#endif /* MQTT_ESP8266_H_ */

/* eof */
