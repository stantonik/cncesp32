/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : cncesp32
 * @created     : Monday Aug 26, 2024 19:00:52 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "stepperesp.h"
#include "webserver.h"
#include "gcode.h"
#include "sd.h"
#include "config.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
esp_err_t init();
esp_err_t move_absolute(float x, float y, float z, float f);
esp_err_t move_relative(float x, float y, float z, float f);

void webserver_post_callback(char *key, char *val);
void gcode_cmd_callback(char cmd_type, int cmd_number);

void gcode_task(void *arg);

/******************************/
/*   Function Definitions     */
/******************************/
esp_err_t init()
{
  ESP_ERROR_CHECK(sd_init());
  ESP_ERROR_CHECK(config_load());
  gcode_reset();
  gcode_set_cmd_callback(gcode_cmd_callback);

  /* Motors initialization */
  struct motor_config motor_config = { 0 };
#define CREATE_MOTOR(axis, motor_config) \
  motor_config.name = #axis[0]; \
  config_get_setting(config_##axis##_motor_dir_pin, &motor_config.dir_pin, CONFIG_INT); \
  config_get_setting(config_##axis##_motor_step_pin, &motor_config.step_pin, CONFIG_INT); \
  config_get_setting(config_##axis##_motor_en_pin, &motor_config.en_pin, CONFIG_INT); \
  config_get_setting(config_##axis##_motor_revsteps, &motor_config.steps_per_rev, CONFIG_UINT); \
  config_get_setting(config_##axis##_microsteps, &motor_config.microsteps, CONFIG_UINT); \
  ESP_ERROR_CHECK(motor_create(&motor_config, &axis##motor))

  CREATE_MOTOR(x, motor_config);
  CREATE_MOTOR(y, motor_config);
  CREATE_MOTOR(z, motor_config);
  CREATE_MOTOR(e, motor_config);

  float lead;
  config_get_setting(config_x_lead, &lead, CONFIG_FLOAT);
  motor_set_lead(xmotor, lead);
  config_get_setting(config_y_lead, &lead, CONFIG_FLOAT);
  motor_set_lead(ymotor, lead);
  config_get_setting(config_z_lead, &lead, CONFIG_FLOAT);
  motor_set_lead(zmotor, lead);
  config_get_setting(config_e_lead, &lead, CONFIG_FLOAT);
  motor_set_lead(emotor, lead);

  struct motor_profile_config motor_profile_config = {  };
  char temp[16];
#define SET_PROFILE(axis, profile) \
  config_get_setting(config_##axis##_speed_profile, temp, CONFIG_STRING); \
  if (strcmp(temp, "LINEAR") == 0) \
  { \
    motor_profile_config.type = MOTOR_PROFILE_LINEAR; \
    config_get_setting(config_##axis##_accel, &motor_profile_config.accel, CONFIG_UINT); \
    config_get_setting(config_##axis##_decel, &motor_profile_config.decel, CONFIG_UINT); \
  } \
  else if (strcmp(temp, "CONSTANT") == 0) motor_profile_config.type = MOTOR_PROFILE_CONSTANT; \
  motor_set_profile(axis##motor, &motor_profile_config)

  SET_PROFILE(x, motor_profile_config);
  SET_PROFILE(y, motor_profile_config);
  SET_PROFILE(z, motor_profile_config);

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
  return move_relative(x - xpos, y - ypos, z - zpos, f);
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

  xpos += x;
  ypos += y;
  zpos += z;

  return 0;
}

void webserver_post_callback(char *key, char *val)
{
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
    gcode_read_cmd(val);
  }
  else if (strcmp(key, "gcode-file") == 0)
  {
    // TODO : not working..
    ESP_LOGI(TAG, "gcode file is downloading...");
    FILE *print = fopen(SD_MOUNT_POINT"/current.gcode", "w");
    fprintf(print, "%s", val);
    fclose(print);
    ESP_LOGI(TAG, "complete");
  }
  else if (strcmp(key, "print-start") == 0)
  {
    xTaskCreatePinnedToCore(gcode_task, "gcodeTask", 8192, NULL, 1, NULL, 1); 
  }
  else
  {
    ESP_LOGW(TAG, "'%s' key token unknown", key);
  }
}

void gcode_task(void *arg)
{
  ESP_LOGI(TAG, "print started...");
  FILE *print = fopen(SD_MOUNT_POINT"/current.gcode", "r");
  gcode_read_file(print);
  fclose(print);
  ESP_LOGI(TAG, "print complete");
}

void gcode_cmd_callback(char cmd_type, int cmd_number)
{
  if (cmd_type == 'G')
  {
    if (cmd_number == 0)
    {
      move_absolute(gcode_get_param_value('X'), gcode_get_param_value('Y'), gcode_get_param_value('Z'), gcode_get_param_value('F'));
    }
    else if (cmd_number == 1)
    {
      move_absolute(gcode_get_param_value('X'), gcode_get_param_value('Y'), gcode_get_param_value('Z'), gcode_get_param_value('F'));
    }
  }
  else if (cmd_type == 'M')
  {

  }

  // wait until done
  while (motor_get_state(xmotor) != MOTOR_STATE_STILL || motor_get_state(ymotor) != MOTOR_STATE_STILL)
  {
    vTaskDelay(pdMS_TO_TICKS(10));
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

