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
#include <cstring>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "stepperesp.h"
#include "webserver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/******************************/
/*     Global Variables       */
/******************************/
#define TAG "cncesp32"

float xpos, ypos, zpos;

motor_handle_t xmotor, ymotor, zmotor, emotor;
motor_handle_t *motors[4] = { &xmotor, &ymotor, &zmotor, &emotor };
char motor_names[4] = { 'X', 'Y', 'Z', 'E' };

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
  struct motor_config x_motor_config =
  {
    .dir_pin = GPIO_NUM_25,
    .step_pin = GPIO_NUM_26,
    .steps_per_rev = 48,
    .microsteps = 16,
    .name = 'X'
  };

  struct motor_config y_motor_config =
  {
    .dir_pin = GPIO_NUM_3,
    .step_pin = GPIO_NUM_4,
    .steps_per_rev = 48,
    .microsteps = 16,
    .name = 'Y'
  };

  struct motor_config z_motor_config =
  {
    .dir_pin = GPIO_NUM_3,
    .step_pin = GPIO_NUM_4,
    .steps_per_rev = 48,
    .microsteps = 16,
    .name = 'Z'
  };

  struct motor_config e_motor_config =
  {
    .dir_pin = GPIO_NUM_3,
    .step_pin = GPIO_NUM_4,
    .steps_per_rev = 48,
    .microsteps = 16,
    .name = 'E'
  };

  struct motor_profile_config motor_profile_config =
  {
    .type = MOTOR_PROFILE_LINEAR,
    .accel = 7000,
    .decel = 7000
  };

  ESP_ERROR_CHECK(motor_create(&x_motor_config, &xmotor));
  ESP_ERROR_CHECK(motor_create(&y_motor_config, &ymotor));
  ESP_ERROR_CHECK(motor_create(&z_motor_config, &zmotor));
  ESP_ERROR_CHECK(motor_create(&e_motor_config, &emotor));

  motor_set_lead(xmotor, 8);
  motor_set_lead(ymotor, 8);
  motor_set_lead(zmotor, 8);
  motor_set_profile(xmotor, &motor_profile_config);
  motor_set_profile(ymotor, &motor_profile_config);
  motor_set_profile(zmotor, &motor_profile_config);

  /* Temp */
  motor_enable(xmotor);
  motor_enable(ymotor);
  motor_enable(zmotor);
  motor_enable(emotor);

  /* Server initialization */
  webserver_init();
  webserver_set_post_callback(webserver_post_callback);

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

  /* Speed projection */
  float xf = f * fabs(x) / d;
  float yf = f * fabs(y) / d;
  float zf = f * fabs(z) / d;

  /* Move motors */
  motor_turn_mm(xmotor, x, xf);
  motor_turn_mm(ymotor, y, yf);
  motor_turn_mm(zmotor, z, zf);

  return 0;
}

void webserver_post_callback(char *key, char *val)
{
  ESP_LOGI(TAG, "received -> key : %s, value : %s", key, val);

  if (strcmp(key, "X_move_from") == 0)
  {
    move_relative(atof(val), 0, 0, 30); 
  }
  else if (strcmp(key, "Y_move_from") == 0)
  {
    move_relative(0, atof(val), 0, 30); 
  }
  else if (strcmp(key, "Z_move_from") == 0)
  {
    move_relative(0, 0, atof(val), 30); 
  }
  else if (strcmp(key, "gcode-cmd") == 0)
  {
  }
  else
  {
    ESP_LOGW(TAG, "'%s' key token unknown", key);
  }
}

void app_main(void)
{
  init();

  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

