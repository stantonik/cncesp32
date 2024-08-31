/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : config
 * @created     : Friday Aug 30, 2024 19:20:54 CEST
 */

#ifndef CONFIG_H
#define CONFIG_H

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

/******************************/
/*   Typedefs, Struct, Enums  */
/******************************/

/******************************/
/*     Global Variables       */
/******************************/
enum config_setting_key : uint8_t
{
  config_wifi_ssid,
  config_wifi_password,
  config_wifi_auth_mode,

  config_hostname,
  config_port,

  config_x_motor_dir_pin,
  config_x_motor_step_pin,
  config_x_motor_en_pin,
  config_x_motor_revsteps,
  config_x_motor_reverse,
  config_x_endstop_pin,
  config_x_microsteps,
  config_x_lead,
  config_x_speed_profile,
  config_x_accel,
  config_x_decel,
  config_x_max_speed,
  config_x_homing_speed,
  config_x_min_pos,
  config_x_max_pos,

  config_y_motor_dir_pin,
  config_y_motor_step_pin,
  config_y_motor_en_pin,
  config_y_motor_revsteps,
  config_y_motor_reverse,
  config_y_endstop_pin,
  config_y_microsteps,
  config_y_lead,
  config_y_speed_profile,
  config_y_accel,
  config_y_decel,
  config_y_max_speed,
  config_y_homing_speed,
  config_y_min_pos,
  config_y_max_pos,

  config_z_motor_dir_pin,
  config_z_motor_step_pin,
  config_z_motor_en_pin,
  config_z_motor_revsteps,
  config_z_motor_reverse,
  config_z_endstop_pin,
  config_z_microsteps,
  config_z_lead,
  config_z_speed_profile,
  config_z_accel,
  config_z_decel,
  config_z_max_speed,
  config_z_homing_speed,
  config_z_min_pos,
  config_z_max_pos,

  config_z_homing_retract_dist,

  config_e_motor_dir_pin,
  config_e_motor_step_pin,
  config_e_motor_en_pin,
  config_e_motor_revsteps,
  config_e_motor_reverse,
  config_e_microsteps,
  config_e_lead,
  config_e_speed_profile,
  config_e_max_speed,

  config_e_max_temp,
  config_e_min_extrude_temp,
  config_e_resistor_pin,
  config_e_thermistor_pin,

  config_miso_pin,
  config_mosi_pin,
  config_clk_pin,

  config_display_cs_pin,
  config_display_ao_pin,
  config_display_reset_pin,
  config_rotenc_clk_pin,
  config_rotenc_dt_pin,
  config_rotenc_switch_pin,

  config_setting_count // end
};

enum config_setting_type : uint8_t
{
  CONFIG_UINT,
  CONFIG_INT,
  CONFIG_FLOAT,
  CONFIG_STRING
};

/******************************/
/*   Function Declarations    */
/******************************/
extern esp_err_t config_load();
extern esp_err_t config_get_setting(enum config_setting_key key, void *dst, enum config_setting_type type);
extern esp_err_t config_free();

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* CONFIG_H */

