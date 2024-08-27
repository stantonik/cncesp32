/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : gcode
 * @created     : Tuesday Aug 27, 2024 02:13:27 CEST
 */

#ifndef GCODE_H
#define GCODE_H

/******************************/
/*          INCLUDES          */
/******************************/
#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/*          INCLUDES          */
/******************************/
#include <stdlib.h>
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
extern esp_err_t gcode_reset();
extern esp_err_t gcode_read_file(FILE *file);
extern esp_err_t gcode_read_cmd(const char *cmd);

extern float gcode_get_param_value(char param);
extern esp_err_t gcode_get_last_cmd(char *type, uint16_t *number);

extern esp_err_t gcode_set_cmd_callback(void (*callback)(char type, int number));

#ifdef __cplusplus
}
#endif /* __cplusplus */

/******************************/
/*   Function Declarations    */
/******************************/


#endif /* GCODE_H */

