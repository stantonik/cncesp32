/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : webserver
 * @created     : Monday Aug 26, 2024 19:32:56 CEST
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/*          INCLUDES          */
/******************************/
#include "esp_err.h"

/******************************/
/*      Macro Definitions     */
/******************************/
#define WIFI_SSID "SFR_8450"
#define WIFI_PASSWORD "uhq55dykr8fni27ucvb4"
#define WIFI_AUTH_MODE WIFI_AUTH_WPA_WPA2_PSK

#define SERVER_PORT 80

/******************************/
/*   Typedefs, Struct, Enums  */
/******************************/

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*   Function Declarations    */
/******************************/
extern esp_err_t webserver_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* WEBSERVER_H */

