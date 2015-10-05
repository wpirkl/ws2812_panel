#ifndef ESP8266_H_
#define ESP8266_H_

#include <stdint.h>
#include <stdbool.h>

/*! Defines return of a command */
typedef enum {
    /*! Successful completion */
    ESP8266_OK      = 0,
    /*! An error occured */
    ESP8266_ERROR,
    /*! Timeout */
    ESP8266_TIMEOUT,
    /*! In progress */
    ESP8266_IN_PROGRESS,
} te_esp8266_cmd_ret;

/*! Defines the wifi mode */
typedef enum {
    /*! Station mode */
    ESP8266_WIFI_MODE_STATION   = 1,
    /*! Access point mode */
    ESP8266_WIFI_MODE_AP        = 2,
    /*! Station and access point mode */
    ESP8266_WIFI_MODE_STA_AP    = 3,
} te_esp8266_wifi_mode;


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

    \param[out] outVersionInfo      Buffer which gets filled by this function
    \param[in]  inVersionInfoMaxLen Maximum number of bytes which gets filled by this function
    \param[out] outVersionInfoLen   Length of the data in the buffer

    \retval true    Successful completion
    \retval false   Failed
*/
bool esp8266_cmd_gmr(uint8_t * outVersionInfo, size_t inVersionInfoMaxLen, size_t * outVersionInfoLen);


/*! Set esp8266 echo off

    \retval true    Successful completion
    \retval false   Failed
*/
bool esp8266_cmd_ate0(void);


/*! Set esp8266 wifi mode

    \param[in]  inWifiMode      The mode to set

    \retval true    Successful completion
    \retval false   Failed
*/
bool esp8266_cmd_set_cwmode_cur(te_esp8266_wifi_mode inWifiMode);


/*! Join esp8266 wifi access point

    \param[in]  inSSID      The ssid to join (max 31 byte)
    \param[in]  inSSIDLen   The lenght of the ssid
    \param[in]  inPWD       The password (max 64 byte)
    \param[in]  inPWDLen    The length of the password

    \retval true    Successful completion
    \retval false   Failed
*/
bool esp8266_cmd_set_cwjap_cur(uint8_t * inSSID, size_t inSSIDLen, uint8_t * inPWD, size_t inPWDLen);




#endif /* ESP8266_H_ */

/* eof */
