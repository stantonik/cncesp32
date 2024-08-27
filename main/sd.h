/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : sd
 * @created     : Tuesday Aug 27, 2024 16:43:38 CEST
 */

#ifndef SD_H
#define SD_H

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
#define SD_MOUNT_POINT "/sd"

/******************************/
/*   Typedefs, Struct, Enums  */
/******************************/

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*   Function Declarations    */
/******************************/
extern esp_err_t sd_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* SD_H */

