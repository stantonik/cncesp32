/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : cncesp32
 * @created     : Monday Aug 26, 2024 19:00:52 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "cncesp32.h"

#include <cmath>

#include "stepperesp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "webserver.h"

/******************************/
/*     Global Variables       */
/******************************/
#define TAG "cncesp32"

float xpos, ypos, zpos;

motor_handle_t xmotor, ymotor, zmotor, emotor;

/******************************/
/*     Static Variables       */
/******************************/

/******************************/
/*    Function Prototypes     */
/******************************/

/******************************/
/*   Function Definitions     */
/******************************/
esp_err_t init()
{
  /* Motors initialization */
  struct motor_config motor_config =
  {
    .dir_pin = GPIO_NUM_25,
    .step_pin = GPIO_NUM_26,
    .steps_per_rev = 48,
    .microsteps = 16,
    .name = 'X'
  };

  struct motor_profile_config motor_profile_config =
  {
    .type = MOTOR_PROFILE_LINEAR,
    .accel = 1000,
    .decel = 1000
  };

  esp_err_t ret = motor_create(&motor_config, &xmotor);
  if (ret != ESP_OK)
  {
    return ret;
  }

  /* motor_set_lead(xmotor, 8); */
  motor_set_profile(xmotor, &motor_profile_config);

  /* Server initialization */
  webserver_init();

  return ESP_OK;
}

esp_err_t move_absolute(float x, float y, float z, float f)
{
  return move_relative(xpos - x, ypos - y, zpos - z, f);
}

esp_err_t move_relative(float x, float y, float z, float f)
{
  /* Calculate the euclidien distance to the target point */
  float d = sqrt(x * x + y * y + z * z);

  if (d == 0 || f == 0) return -1;

  /* Convert linear speed into round per minutes */
  float xrpm = f * fabs(x) / d / X_LINEAR_DISPLACEMENT_COEF;
  float yrpm = f * fabs(y) / d / Y_LINEAR_DISPLACEMENT_COEF;
  float zrpm = f * fabs(z) / d / Z_LINEAR_DISPLACEMENT_COEF;

  /* Convert mm into steps */
  int32_t xsteps = x * X_LINEAR_DISPLACEMENT_COEF;
  int32_t ysteps = y * Y_LINEAR_DISPLACEMENT_COEF;
  int32_t zsteps = z * Z_LINEAR_DISPLACEMENT_COEF;

  /* Move motors */
  motor_turn_full_step(xmotor, xsteps, xrpm);
  motor_turn_full_step(ymotor, ysteps, yrpm);
  motor_turn_full_step(zmotor, zsteps, zrpm);

  return 0;
}

void app_main(void)
{
  init();

  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

