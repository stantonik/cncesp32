/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : cncesp32
 * @created     : Monday Aug 26, 2024 19:03:35 CEST
 */

#ifndef CNCESP32_H
#define CNCESP32_H

/******************************/
/*          INCLUDES          */
/******************************/
#include "esp_err.h"

/******************************/
/*      Macro Definitions     */
/******************************/
#define X_DIR_PIN 26
#define X_STEP_PIN 25
#define X_STEP_PER_REV (48)
#define X_LINEAR_DISPLACEMENT_COEF (X_STEP_PER_REV * 10)

#define Y_DIR_PIN 1
#define Y_STEP_PIN 1
#define Y_STEP_PER_REV 48
#define Y_LINEAR_DISPLACEMENT_COEF (Y_STEP_PER_REV * 10)

#define Z_DIR_PIN 1
#define Z_STEP_PIN 1
#define Z_STEP_PER_REV 200
#define Z_LINEAR_DISPLACEMENT_COEF (Z_STEP_PER_REV * 10)

#define E_DIR_PIN 1
#define E_STEP_PIN 1
#define E_STEP_PER_REV 200
#define E_LINEAR_DISPLACEMENT_COEF (E_STEP_PER_REV * 10)

/******************************/
/*   Typedefs, Struct, Enums  */
/******************************/

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*   Function Declarations    */
/******************************/
esp_err_t init();
esp_err_t move_absolute(float x, float y, float z, float f);
esp_err_t move_relative(float x, float y, float z, float f);

void webserver_post_callback(char *key, char *val);
void gcode_cmd_callback(char cmd_type, int cmd_number);

extern "C" void app_main(void);

#endif /* CNCESP32_H */

