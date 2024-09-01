/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : extruder
 * @created     : Sunday Sep 01, 2024 12:25:03 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "extruder.h"

#include "config.h"

#include <math.h>
#include "driver/adc_types_legacy.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/ledc_types.h"

/******************************/
/*      Macro Definitions     */
/******************************/
#define TAG "extruder"

#define ADC_WIDTH ADC_WIDTH_BIT_12
#define ADC_ATTEN ADC_ATTEN_DB_6

/******************************/
/*     Global Variables       */
/******************************/
float epos;
motor_handle_t emotor;

/******************************/
/*     Static Variables       */
/******************************/
gpio_num_t resistor_pin;
gpio_num_t thermistance_pin;
adc1_channel_t channel;

uint32_t thermistance_r0 = 10000;
uint32_t r_fixed = 10000;
uint32_t beta = 3950;

esp_adc_cal_characteristics_t *adc_chars;
float integral;
float kp, ki, kd;
int64_t last_time;
float previous_error;

/******************************/
/*    Function Prototypes     */
/******************************/
float compute_pid(float target_temp, float measured_temp);

/******************************/
/*   Function Definitions     */
/******************************/
esp_err_t extruder_init()
{
  config_get_setting(config_e_resistor_pin, &resistor_pin, CONFIG_INT);
  config_get_setting(config_e_thermistor_pin, &thermistance_pin, CONFIG_INT);

  /* Init motor */
  struct motor_config motor_config = { 0 };
  motor_config.name = 'E';
  config_get_setting(config_e_motor_dir_pin, &motor_config.dir_pin, CONFIG_INT);
  config_get_setting(config_e_motor_step_pin, &motor_config.step_pin, CONFIG_INT);
  config_get_setting(config_e_motor_en_pin, &motor_config.en_pin, CONFIG_INT);
  config_get_setting(config_e_motor_revsteps, &motor_config.steps_per_rev, CONFIG_UINT);
  config_get_setting(config_e_microsteps, &motor_config.microsteps, CONFIG_UINT);
  ESP_ERROR_CHECK(motor_create(&motor_config, &emotor));

  float lead;
  config_get_setting(config_e_lead, &lead, CONFIG_FLOAT);
  motor_set_lead(emotor, lead);

  ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(resistor_pin), -1, TAG, "resistor pin not valid");
  switch ((int)thermistance_pin) 
  {
    case GPIO_NUM_34:
      channel = ADC1_CHANNEL_6;
      break;
    case GPIO_NUM_35:
      channel = ADC1_CHANNEL_7;
      break;
    case GPIO_NUM_36:
      channel = ADC1_CHANNEL_0;
      break;
    case GPIO_NUM_39:
      channel = ADC1_CHANNEL_3;
      break;
    default:
      ESP_LOGE(TAG, "thermistance pin not valid (34, 35, 36 or 39)");
      return -1;
  }

  /* Resistor PWM config */
  /* ledc_timer_config_t ledc_timer = */ 
  /* { */
  /*   .speed_mode       = LEDC_LOW_SPEED_MODE, */
  /*   .timer_num        = LEDC_TIMER_0, */
  /*   .duty_resolution  = LEDC_TIMER_8_BIT, */
  /*   .freq_hz          = 1000, */
  /*   .clk_cfg          = LEDC_AUTO_CLK */
  /* }; */
  /* ledc_timer_config(&ledc_timer); */

  /* ledc_channel_config_t ledc_channel = */ 
  /* { */
  /*   .speed_mode     = LEDC_LOW_SPEED_MODE, */
  /*   .channel        = LEDC_CHANNEL_0, */
  /*   .timer_sel      = LEDC_TIMER_0, */
  /*   .intr_type      = LEDC_INTR_DISABLE, */
  /*   .gpio_num       = resistor_pin, */
  /*   .duty           = 0, */
  /*   .hpoint         = 0 */
  /* }; */
  /* ledc_channel_config(&ledc_channel); */

  /* /1* ADC setup *1/ */
  /* adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t)); */

  /* esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT_12, 3300, adc_chars); */
  /* adc1_config_width(ADC_WIDTH); */
  /* adc1_config_channel_atten(channel, ADC_ATTEN); */

  return ESP_OK;
}

esp_err_t extruder_set_temp(uint16_t temp, bool blocking)
{
  last_time = esp_timer_get_time();
  while (1) 
  {
    /* Read current temp */
    float current_temperature = extruder_get_current_temp();

    /* Compute the PWM power */
    float power = compute_pid(temp, current_temperature);

    if (power > 255) power = 255;
    if (power < 0) power = 0;

    /* Set the resistor PWM */
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, power);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  return ESP_OK;
}

float extruder_get_current_temp()
{
  uint32_t adc_reading = adc1_get_raw(channel);
  uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

  uint32_t thermistor_res = r_fixed * (3300 / voltage - 1);

  // Calculate temperature in Kelvin using the Beta equation
  float temperature_kelvin = 1 / ((log((float)thermistor_res / (float)thermistance_r0) / beta) + (1 / (273.15f + 25)));

  // Convert Kelvin to Celsius
  return temperature_kelvin - 273.15;
}

esp_err_t extruder_extrude(float e, float f)
{

  return ESP_OK;
}

float compute_pid(float target_temp, float measured_temp) 
{
  int64_t current_time = esp_timer_get_time();
  float delta_time = (current_time - last_time) / 1000000.0; 
  last_time = current_time;

  float error = target_temp - measured_temp;
  integral += error * delta_time;
  float derivative = (error - previous_error) / delta_time;

  float output = kp * error + ki * integral + kd * derivative;

  previous_error = error;

  return output;
}
