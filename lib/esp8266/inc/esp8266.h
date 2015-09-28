#ifndef ESP8266_H_
#define ESP8266_H_

#include <stdint.h>
#include <stdbool.h>

/*! socket structure */
typedef struct s_esp8266_socket ts_esp8266_socket;

/*!
    Initialize the ESP8266 driver

    This will initialize the uart driver
*/
void esp8266_init(void);


/*!
    Handle rx
*/
void esp8266_rx_handler(void);



#endif /* ESP8266_H_ */

/* eof */
