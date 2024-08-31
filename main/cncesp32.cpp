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
#include <cstdio>
#include <cstring>
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
  struct motor_config x_motor_config = { .name = 'X' };
  config_get_setting(config_x_motor_dir_pin, &x_motor_config.dir_pin, CONFIG_INT);
  config_get_setting(config_x_motor_step_pin, &x_motor_config.step_pin, CONFIG_INT);
  config_get_setting(config_x_motor_en_pin, &x_motor_config.en_pin, CONFIG_INT);
  config_get_setting(config_x_motor_revsteps, &x_motor_config.steps_per_rev, CONFIG_UINT);
  config_get_setting(config_x_microsteps, &x_motor_config.microsteps, CONFIG_UINT);

  struct motor_config y_motor_config = { .name = 'Y' };
  config_get_setting(config_y_motor_dir_pin, &y_motor_config.dir_pin, CONFIG_INT);
  config_get_setting(config_y_motor_step_pin, &y_motor_config.step_pin, CONFIG_INT);
  config_get_setting(config_y_motor_en_pin, &y_motor_config.en_pin, CONFIG_INT);
  config_get_setting(config_y_motor_revsteps, &y_motor_config.steps_per_rev, CONFIG_UINT);
  config_get_setting(config_y_microsteps, &y_motor_config.microsteps, CONFIG_UINT);

  struct motor_config z_motor_config = { .name = 'Z' };
  config_get_setting(config_z_motor_dir_pin, &z_motor_config.dir_pin, CONFIG_INT);
  config_get_setting(config_z_motor_step_pin, &z_motor_config.step_pin, CONFIG_INT);
  config_get_setting(config_z_motor_en_pin, &z_motor_config.en_pin, CONFIG_INT);
  config_get_setting(config_z_motor_revsteps, &z_motor_config.steps_per_rev, CONFIG_UINT);
  config_get_setting(config_z_microsteps, &z_motor_config.microsteps, CONFIG_UINT);

  struct motor_config e_motor_config = { .name = 'E' };
  config_get_setting(config_e_motor_dir_pin, &e_motor_config.dir_pin, CONFIG_INT);
  config_get_setting(config_e_motor_step_pin, &e_motor_config.step_pin, CONFIG_INT);
  config_get_setting(config_e_motor_en_pin, &e_motor_config.en_pin, CONFIG_INT);
  config_get_setting(config_e_motor_revsteps, &e_motor_config.steps_per_rev, CONFIG_UINT);
  config_get_setting(config_e_microsteps, &e_motor_config.microsteps, CONFIG_UINT);

  struct motor_profile_config motor_profile_config = {  };
  char temp[16];
  config_get_setting(config_x_speed_profile, temp, CONFIG_STRING);
  if (strcmp(temp, "LINEAR") == 0) 
  {
    motor_profile_config.type = MOTOR_PROFILE_LINEAR;
    config_get_setting(config_x_accel, &motor_profile_config.accel, CONFIG_UINT);
    config_get_setting(config_x_decel, &motor_profile_config.decel, CONFIG_UINT);
  }
  else if (strcmp(temp, "CONSTANT") == 0) motor_profile_config.type = MOTOR_PROFILE_CONSTANT;

  ESP_ERROR_CHECK(motor_create(&x_motor_config, &xmotor));
  ESP_ERROR_CHECK(motor_create(&y_motor_config, &ymotor));
  /* ESP_ERROR_CHECK(motor_create(&z_motor_config, &zmotor)); */
  /* ESP_ERROR_CHECK(motor_create(&e_motor_config, &emotor)); */

  float lead;
  config_get_setting(config_x_lead, &lead, CONFIG_FLOAT);
  motor_set_lead(xmotor, lead);
  config_get_setting(config_y_lead, &lead, CONFIG_FLOAT);
  motor_set_lead(ymotor, lead);
  config_get_setting(config_z_lead, &lead, CONFIG_FLOAT);
  /* motor_set_lead(zmotor, lead); */
  motor_set_profile(xmotor, &motor_profile_config);
  motor_set_profile(ymotor, &motor_profile_config);
  /* motor_set_profile(zmotor, &motor_profile_config); */

  /* Temp */
  motor_enable(xmotor);
  motor_enable(ymotor);
  /* motor_enable(zmotor); */
  /* motor_enable(emotor); */

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
  /* motor_turn_mm(zmotor, z, zf); */

  xpos += x;
  ypos += y;
  zpos += z;

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
    gcode_read_cmd(val);
  }
  else if (strcmp(key, "gcode-file") == 0)
  {
    ESP_LOGI(TAG, "gcode file is downloading...");
    FILE *print = fopen(SD_MOUNT_POINT"/current.gcode", "w");
    fprintf(print, "%s", val);
    fclose(print);
    ESP_LOGI(TAG, "complete");
  }
  else if (strcmp(key, "print-start") == 0)
  {
    ESP_LOGI(TAG, "print started...");
    FILE *print = fopen(SD_MOUNT_POINT"/current.gcode", "r");
    gcode_read_file(print);
    fclose(print);
    ESP_LOGI(TAG, "print complete");
  }
  else
  {
    ESP_LOGW(TAG, "'%s' key token unknown", key);
  }
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
}

void app_main(void)
{
  init();

  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

