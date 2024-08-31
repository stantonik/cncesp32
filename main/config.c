/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : config
 * @created     : Friday Aug 30, 2024 19:21:01 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "config.h"

#include "esp_check.h"
#include "esp_log.h"
#include "sd.h"

#define JSMN_HEADER
#include "jsmn.h"

#include <stdio.h>
#include <string.h>

/******************************/
/*      Macro Definitions     */
/******************************/
#define FILE_PATH SD_MOUNT_POINT"/system/config.json"

/******************************/
/*     Static Variables       */
/******************************/
#define TAG "config"

jsmntok_t *t;
uint16_t r;
char *configstr;

struct setting_map
{
    enum config_setting_key key;
    const char *str;
    enum config_setting_type type;
} 
mapping[config_setting_count] =
{
    { config_wifi_ssid, "wifi_ssid", CONFIG_STRING },
    { config_wifi_password, "wifi_password", CONFIG_STRING },
    { config_wifi_auth_mode, "wifi_auth_mode", CONFIG_STRING },

    { config_hostname, "hostname", CONFIG_STRING },
    { config_port, "port", CONFIG_INT },

    { config_x_motor_dir_pin, "x_motor_dir_pin", CONFIG_INT },
    { config_x_motor_step_pin, "x_motor_step_pin", CONFIG_INT },
    { config_x_motor_en_pin, "x_motor_en_pin", CONFIG_INT },
    { config_x_motor_revsteps, "x_motor_revsteps", CONFIG_UINT },
    { config_x_motor_reverse, "x_motor_reverse", CONFIG_UINT },
    { config_x_endstop_pin, "x_endstop_pin", CONFIG_INT },
    { config_x_microsteps, "x_microsteps", CONFIG_UINT },
    { config_x_lead, "x_lead", CONFIG_FLOAT },
    { config_x_speed_profile, "x_speed_profile", CONFIG_STRING },
    { config_x_accel, "x_accel", CONFIG_UINT },
    { config_x_decel, "x_decel", CONFIG_UINT },
    { config_x_max_speed, "x_max_speed", CONFIG_FLOAT },
    { config_x_homing_speed, "x_homing_speed", CONFIG_FLOAT },
    { config_x_min_pos, "x_min_pos", CONFIG_FLOAT },
    { config_x_max_pos, "x_max_pos", CONFIG_FLOAT },

    { config_y_motor_dir_pin, "y_motor_dir_pin", CONFIG_UINT },
    { config_y_motor_step_pin, "y_motor_step_pin", CONFIG_UINT },
    { config_y_motor_en_pin, "y_motor_en_pin", CONFIG_UINT },
    { config_y_motor_revsteps, "y_motor_revsteps", CONFIG_UINT },
    { config_y_motor_reverse, "y_motor_reverse", CONFIG_UINT },
    { config_y_endstop_pin, "y_endstop_pin", CONFIG_UINT },
    { config_y_microsteps, "y_microsteps", CONFIG_UINT },
    { config_y_lead, "y_lead", CONFIG_FLOAT },
    { config_y_speed_profile, "y_speed_profile", CONFIG_STRING },
    { config_y_accel, "y_accel", CONFIG_FLOAT },
    { config_y_decel, "y_decel", CONFIG_FLOAT },
    { config_y_max_speed, "y_max_speed", CONFIG_FLOAT },
    { config_y_homing_speed, "y_homing_speed", CONFIG_FLOAT },
    { config_y_min_pos, "y_min_pos", CONFIG_FLOAT },
    { config_y_max_pos, "y_max_pos", CONFIG_FLOAT },

    { config_z_motor_dir_pin, "z_motor_dir_pin", CONFIG_UINT },
    { config_z_motor_step_pin, "z_motor_step_pin", CONFIG_UINT },
    { config_z_motor_en_pin, "z_motor_en_pin", CONFIG_UINT },
    { config_z_motor_revsteps, "z_motor_revsteps", CONFIG_UINT },
    { config_z_motor_reverse, "z_motor_reverse", CONFIG_UINT },
    { config_z_endstop_pin, "z_endstop_pin", CONFIG_UINT },
    { config_z_microsteps, "z_microsteps", CONFIG_UINT },
    { config_z_lead, "z_lead", CONFIG_FLOAT },
    { config_z_speed_profile, "z_speed_profile", CONFIG_STRING },
    { config_z_accel, "z_accel", CONFIG_FLOAT },
    { config_z_decel, "z_decel", CONFIG_FLOAT },
    { config_z_max_speed, "z_max_speed", CONFIG_FLOAT },
    { config_z_homing_speed, "z_homing_speed", CONFIG_FLOAT },
    { config_z_min_pos, "z_min_pos", CONFIG_FLOAT },
    { config_z_max_pos, "z_max_pos", CONFIG_FLOAT },

    { config_z_homing_retract_dist, "z_homing_retract_dist", CONFIG_FLOAT },

    { config_e_motor_dir_pin, "e_motor_dir_pin", CONFIG_UINT },
    { config_e_motor_step_pin, "e_motor_step_pin", CONFIG_UINT },
    { config_e_motor_en_pin, "e_motor_en_pin", CONFIG_UINT },
    { config_e_motor_revsteps, "e_motor_revsteps", CONFIG_UINT },
    { config_e_motor_reverse, "e_motor_reverse", CONFIG_UINT },
    { config_e_microsteps, "e_microsteps", CONFIG_UINT },
    { config_e_lead, "e_lead", CONFIG_FLOAT },
    { config_e_speed_profile, "e_speed_profile", CONFIG_STRING },
    { config_e_max_speed, "e_max_speed", CONFIG_FLOAT },

    { config_e_max_temp, "e_max_temp", CONFIG_FLOAT },
    { config_e_min_extrude_temp, "e_min_extrude_temp", CONFIG_FLOAT },
    { config_e_resistor_pin, "e_resistor_pin", CONFIG_UINT },
    { config_e_thermistor_pin, "e_thermistor_pin", CONFIG_UINT },

    { config_miso_pin, "miso_pin", CONFIG_UINT },
    { config_mosi_pin, "mosi_pin", CONFIG_UINT },
    { config_clk_pin, "clk_pin", CONFIG_UINT },

    { config_display_cs_pin, "display_cs_pin", CONFIG_UINT },
    { config_display_ao_pin, "display_ao_pin", CONFIG_UINT },
    { config_display_reset_pin, "display_reset_pin", CONFIG_UINT },
    { config_rotenc_clk_pin, "rotenc_clk_pin", CONFIG_UINT },
    { config_rotenc_dt_pin, "rotenc_dt_pin", CONFIG_UINT },
    { config_rotenc_switch_pin, "rotenc_switch_pin", CONFIG_UINT },
};

/******************************/
/*    Function Prototypes     */
/******************************/

/******************************/
/*   Function Definitions     */
/******************************/
esp_err_t config_load()
{
  ESP_LOGI(TAG, "reading...");
  FILE *file = fopen(FILE_PATH, "r");
  ESP_RETURN_ON_FALSE(file != NULL, -1, TAG, "read failed");

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  configstr = (char *)malloc((file_size + 1) * sizeof(char));
  ESP_RETURN_ON_FALSE(configstr != NULL, -1, TAG, "no memory left");
  fread(configstr, sizeof(char), file_size, file);
  configstr[file_size] = '\0';
  fclose(file);

  jsmn_parser p;
  jsmn_init(&p);
  t = (jsmntok_t *)malloc(sizeof(jsmntok_t) * file_size);
  r = jsmn_parse(&p, configstr, file_size * sizeof(char), t, file_size * 2 + 1);
  ESP_RETURN_ON_FALSE(r > 0 && t[0].type == JSMN_OBJECT, -1, TAG, "file syntax error");
  //TODO : handle this error (freeing)

  ESP_LOGI(TAG, "complete");
  return ESP_OK;
}

esp_err_t config_get_setting(enum config_setting_key key, void *dst, enum config_setting_type type)
{
  ESP_RETURN_ON_FALSE(dst != NULL, -1, TAG, "dst points to null");
  struct setting_map setting_map = { 0 };
  for (int i = 0; i < config_setting_count; i++)
  {
    if (mapping[i].key == key) setting_map = mapping[i];
  }

  int i;
  for (i = 1; i < r; i += 2)
  {
    if (strncmp(configstr + t[i].start, setting_map.str, t[i].end - t[i].start) == 0)
    {
      ESP_RETURN_ON_FALSE(t[i + 1].type != JSMN_STRING || setting_map.type == CONFIG_STRING, -1, TAG, "'%s' bad unit", setting_map.str);

      int size = t[i + 1].end - t[i + 1].start;

      if (t[i + 1].type == JSMN_STRING)
      {
        memcpy(dst, configstr + t[i + 1].start, size);
        ((char *)dst)[size] = '\0';
      }
      else if (t[i + 1].type == JSMN_PRIMITIVE)
      {
        char temp[16];
        memcpy(temp, configstr + t[i + 1].start, size);
        float x = atof(temp);
        if (type == CONFIG_FLOAT) *((float *)dst) = (float)x;
        else if (type == CONFIG_UINT) *((uint32_t *)dst) = (uint32_t)x;
        else if (type == CONFIG_INT) *((int32_t *)dst) = (int32_t)x;
      }
      else
      {
      }
      break;
    }   
  }

  ESP_RETURN_ON_FALSE(i < r, -1, TAG, "some settings were not found");

  return ESP_OK;
}

esp_err_t config_free()
{
  free(configstr);
  free(t);
  return ESP_OK;
}
