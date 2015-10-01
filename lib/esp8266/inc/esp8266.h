#ifndef ESP8266_H_
#define ESP8266_H_

#include <stdint.h>
#include <stdbool.h>

/*! socket structure */
typedef struct s_esp8266_socket ts_esp8266_socket;

/*! Initialize the ESP8266 driver

    This will initialize the uart driver
*/
void esp8266_init(void);


/*! Handle rx

    This function should be called periodically. Using FreeRTOS, it will block on a sempahore
    and therefore should be run in it's own task.
*/
void esp8266_rx_handler(void);


/*! Setup esp8266
*/
void esp8266_setup(void);


/*! Test esp8266 startup

    \retval true    Successful completion
    \retval false   Failed
*/
bool esp8266_cmd_at(void);


/*! Reset esp8266

    \retval true    Successful completion
    \retval false   Failed
*/
bool esp8266_cmd_rst(void);


/*! Get esp8266 version information

    \retval true    Successful completion
    \retval false   Failed
*/
bool esp8266_cmd_gmr(void);


#endif /* ESP8266_H_ */

/* eof */
