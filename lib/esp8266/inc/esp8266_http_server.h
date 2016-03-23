#ifndef ESP8266_HTTP_SERVER_H_
#define ESP8266_HTTP_SERVER_H_

#include <stdbool.h>
#include <stdint.h>

/*!
    Starts the http server

    \retval true    server successfully started
    \retval false   server did not start
*/
bool esp8266_http_server_start(uint32_t inServerTaskPriority);




#endif /* ESP8266_HTTP_SERVER_H_ */

/* eof */
