/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : cncesp32
 * @created     : Monday Jul 29, 2024 20:17:30 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "motors.h"

/******************************/
/*      Macro Definitions     */
/******************************/
#define TAG "cncesp"

#define X_DIR_PIN 1
#define X_STEP_PIN 1
#define X_STEP_PER_REV 48
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
/*     Global Variables       */
/******************************/
struct motor motor_x =
{
  .dir_pin = X_DIR_PIN,
  .step_pin = X_STEP_PIN,
  .step_per_rev = X_STEP_PER_REV,
  .name = "X"
};

struct motor motor_y =
{
  .dir_pin = Y_DIR_PIN,
  .step_pin = Y_STEP_PIN,
  .step_per_rev = Y_STEP_PER_REV,
  .name = "Y"
};

struct motor motor_z =
{
  .dir_pin = Z_DIR_PIN,
  .step_pin = Z_STEP_PIN,
  .step_per_rev = Z_STEP_PER_REV,
  .name = "Z"
};

struct motor motor_e =
{
  .dir_pin = E_DIR_PIN,
  .step_pin = E_STEP_PIN,
  .step_per_rev = E_STEP_PER_REV,
  .name = "E"
};

double xpos, ypos, zpos;

/******************************/
/*     Static Variables       */
/******************************/

/******************************/
/*    Function Prototypes     */
/******************************/
void init();
void move_absolute(double x, double y, double z, double f);
void move_relative(double x, double y, double z, double f);

/******************************/
/*   Function Definitions     */
/******************************/

void init()
{
  /* Initialize motors */
  motor_init(&motor_x);
  motor_init(&motor_y);
  motor_init(&motor_z);
  motor_init(&motor_e);

  /* Initialize SPI Bus */

  /* Initialize SD Card */

  /* Initialize Wifi */

  /* Initialiaze HTTPS Server */

  /* Initialize Display */
}

void move_absolute(double x, double y, double z, double f)
{
  move_relative(xpos - x, ypos - y, zpos - z, f);
}

void move_relative(double x, double y, double z, double f)
{
  /* Calculate the euclidien distance to the target point */
  double d = sqrt(x * x + y * y + z * z);

  if (d == 0 || f == 0) return;

  /* Convert linear speed into round per minutes */
  double xrpm = f * fabs(x) / d / X_LINEAR_DISPLACEMENT_COEF;
  double yrpm = f * fabs(y) / d / Y_LINEAR_DISPLACEMENT_COEF;
  double zrpm = f * fabs(z) / d / Z_LINEAR_DISPLACEMENT_COEF;

  /* Convert mm into steps */
  int32_t xsteps = x * X_LINEAR_DISPLACEMENT_COEF;
  int32_t ysteps = y * Y_LINEAR_DISPLACEMENT_COEF;
  int32_t zsteps = z * Z_LINEAR_DISPLACEMENT_COEF;

  /* Move motors */
  motor_turn_step(&motor_x, xsteps, xrpm);
  motor_turn_step(&motor_y, ysteps, yrpm);
  motor_turn_step(&motor_z, zsteps, zrpm);
}

void app_main(void)
{
  init();

  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
