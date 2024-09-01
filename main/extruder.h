/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : extruder
 * @created     : Sunday Sep 01, 2024 12:25:08 CEST
 */

#ifndef EXTRUDER_H
#define EXTRUDER_H

#include "stepperesp.h"
#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/*          INCLUDES          */
/******************************/
#include "esp_err.h"
#include <stdbool.h>

/******************************/
/*      Macro Definitions     */
/******************************/

/******************************/
/*   Typedefs, Struct, Enums  */
/******************************/

/******************************/
/*     Global Variables       */
/******************************/
extern float epos;
extern motor_handle_t emotor;

/******************************/
/*   Function Declarations    */
/******************************/
extern esp_err_t extruder_init();
extern esp_err_t extruder_set_temp(uint16_t temp, bool blocking);
extern float extruder_get_current_temp();
extern esp_err_t extruder_extrude(float e, float f);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* EXTRUDER_H */

