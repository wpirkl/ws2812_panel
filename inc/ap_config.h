#ifndef AP_CONFIG_H_
#define AP_CONFIG_H_

#include <stdint.h>

#include "esp8266.h"


extern const char                     * const g_AP_SSID     __attribute__((weak));
extern const char                     * const g_AP_PWD      __attribute__((weak));
extern const uint8_t                    g_AP_CHAN     __attribute__((weak));
extern const te_esp8266_encryption_mode g_AP_ENC_MODE __attribute__((weak));

#endif /* AP_CONFIG_H_ */

/* eof */
