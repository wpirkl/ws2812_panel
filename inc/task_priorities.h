#ifndef TASK_PRIORITIES_H_
#define TASK_PRIORITIES_H_

#include "FreeRTOS.h"


#if configMAX_PRIORITIES < 5
#error At least 5 task priorities are used!
#endif


#define CPU_LOAD_TASK_PRIORITY          (4)

#define ESP8266_HTTP_SERVER_PRIORITY    (0)
#define ESP8266_MQTT_TASK_PRIORITY      (0)
#define ESP8266_RX_TASK_PRIORITY        (1)
#define ESP8266_SOCKET_TASK_PRIORITY    (2)
#define ESP8266_WIFI_TASK_PRIORITY      (2)

#define INIT_TASK_PRIORITY              (4)

#define LED_TASK_PRIORITY               (3)



#endif /* TASK_PRIORITIES_H_ */

/* eof */
