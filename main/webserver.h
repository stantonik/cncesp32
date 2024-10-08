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
extern esp_err_t webserver_set_post_callback(void(*callback)(char *, char *));

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* WEBSERVER_H */

