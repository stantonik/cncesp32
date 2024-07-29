/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : motors
 * @created     : Monday Jul 29, 2024 14:26:55 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "motors.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_check.h"
/*     Global Variables       */
/******************************/

/******************************/
/*     Static Variables       */
/******************************/
static const char *TAG = "motors";

static const uint16_t STEP_ON_DELAY_US = 100;

typedef struct
{
  struct motor *motor;

  enum motor_dir dir_state;
  bool step_state;

  uint32_t rpm;
  uint64_t period_us;
  uint64_t dt_us;
  uint64_t step_remaining;
  long step_count;

  enum motor_state state;
  bool enable;
  bool initialized;
} motor_internal_t;

static gptimer_handle_t timer_handle = NULL;
static uint8_t motor_count = 0;
static motor_internal_t *motors[MAX_MOTOR_COUNT] = {  };

/******************************/
/*    Function Prototypes     */
/******************************/
static bool motor_timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
static bool is_motor_init(struct motor *motor);
static int8_t motor_get_id(struct motor *motor);

/******************************/
/*   Function Definitions     */
/******************************/
esp_err_t motor_init(struct motor *motor)
{
  ESP_RETURN_ON_FALSE(motor != NULL, -1, TAG, "Motor is NULL.");
  ESP_RETURN_ON_FALSE(motor_count < MAX_MOTOR_COUNT, -1, TAG, "Max motor count reached.");
  ESP_RETURN_ON_FALSE(motor->step_per_rev >= 20 && motor->step_per_rev <= 2000, -1, TAG, "Step per revolution out of range (20-2000)");
  ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(motor->dir_pin), -1, TAG, "Motor %s dir GPIO not valid.", motor->name);
  ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(motor->step_pin), -1, TAG, "Motor %s step GPIO not valid.", motor->name);

  /* Create GPTimer if the first motor is created */
  if (motor_count == 0)
  {
    ESP_LOGI(TAG, "Initializing motor timer....");

    static gptimer_config_t timer_cfg =
    {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = 1 * 1000 * 1000
    };

    static gptimer_alarm_config_t alarm_cfg =
    {
      .alarm_count = 10,
      .reload_count = 0,
      .flags = { .auto_reload_on_alarm = true }
    };

    gptimer_event_callbacks_t timer_cb = { .on_alarm = motor_timer_callback };

    ESP_RETURN_ON_ERROR(gptimer_new_timer(&timer_cfg, &timer_handle) , TAG, "Failed to create the motor timer.");
    ESP_RETURN_ON_ERROR(gptimer_register_event_callbacks(timer_handle, &timer_cb, NULL), TAG, "Failed to pass motor timer callback.");
    ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(timer_handle, &alarm_cfg), TAG, "Failed to set the motor timer alarm.");
    ESP_RETURN_ON_ERROR(gptimer_enable(timer_handle), TAG, "Failed to enable motor timer.");
    ESP_RETURN_ON_ERROR(gptimer_start(timer_handle), TAG, "Failed to start motor timer.");
    ESP_LOGI(TAG, "Motor timer created.");
  }

  /* Assign a disponible ID to the motor */
  motor_internal_t *motor_internal = NULL;
  for (int i = 0; i < MAX_MOTOR_COUNT; i++)
  {
    if (motors[i] == NULL) 
    {
      motors[i] = malloc(sizeof(motor_internal_t));
      motors[i]->motor = motor;
      motor_internal = motors[i];
      break;
    }
  }

  /* GPIO initialization */
  gpio_reset_pin(motor->dir_pin);
  gpio_reset_pin(motor->step_pin);
  gpio_set_direction(motor->dir_pin, GPIO_MODE_OUTPUT);
  gpio_set_direction(motor->step_pin, GPIO_MODE_OUTPUT);

  /* Motor struct initialization */
  motor_internal->step_state = 0;
  motor_internal->dir_state = 0;
  motor_internal->dt_us = 0;
  motor_internal->step_count = 0;

  motor_internal->state = MOTOR_STILL_STATE;
  motor_internal->enable = false;

  /* Motor initialization succeded */
  motor_internal->initialized = true;
  motor_count++;
  ESP_LOGI(TAG, "Motor %s initialized.", motor->name);
  return ESP_OK; 
}

esp_err_t motor_delete(struct motor *motor)
{
  return motor_delete_by_id(motor_get_id(motor));
}

esp_err_t motor_delete_by_id(int8_t id)
{
  ESP_RETURN_ON_FALSE(id >= 0 && id < MAX_MOTOR_COUNT, -1, TAG, "Invalid ID range.");
  struct motor *motor = motors[id]->motor;
  ESP_RETURN_ON_FALSE(motor != NULL, -1, TAG, "Motor is NULL.");

  /* Delete timer if the last motor was removed */
  uint8_t motor_count_cp = motor_count - 1;
  if (motor_count_cp <= 0)
  {
    gptimer_stop(timer_handle);
    gptimer_disable(timer_handle);
    ESP_RETURN_ON_ERROR(gptimer_del_timer(timer_handle), TAG, "Failed to delete motor timer.");
  }

  gpio_reset_pin(motor->dir_pin);
  gpio_reset_pin(motor->step_pin);

  free(motors[id]);
  motors[id] = NULL;
  motor_count--;

  ESP_LOGI(TAG, "Motor %s deleted.", motor->name);
  return ESP_OK;
}

esp_err_t motor_turn_step(struct motor *motor, uint32_t steps, double rpm, enum motor_dir dir)
{
  ESP_RETURN_ON_FALSE(motor != NULL, -1, TAG, "Motor is NULL.");
  ESP_RETURN_ON_FALSE(is_motor_init(motor), -1, TAG, "Motor %s is not initialized.", motor->name);

  motor_internal_t *motor_internal = motors[motor_get_id(motor)];
  ESP_RETURN_ON_FALSE(motor_internal->state == MOTOR_STILL_STATE, -1, TAG, "Motor %s is already moving !", motor->name);
  ESP_RETURN_ON_FALSE(motor_internal->enable, -1, TAG, "Motor %s is disable.", motor->name);

  motor_internal->dir_state = dir;
  gpio_set_level(motor->dir_pin, (int)dir);

  motor_internal->step_remaining = steps;
  motor_internal->period_us = (60.f / rpm) / (float)motor->step_per_rev * 1 * 1000 * 1000;
  motor_internal->state = MOTOR_CONSTANT_STATE;
  gpio_set_level(motor->step_pin, 1);

  ESP_LOGI(TAG, "Motor %s is turning... Period : %i | Steps : %i", motor->name, (int)motor_internal->period_us, (int)steps);

  return ESP_OK;
}

esp_err_t motor_enable(struct motor *motor)
{
  ESP_RETURN_ON_FALSE(motor != NULL, -1, TAG, "Motor is NULL.");
  ESP_RETURN_ON_FALSE(is_motor_init(motor), -1, TAG, "Motor %s is not initialized.", motor->name);

  motors[motor_get_id(motor)]->enable = true;
  return ESP_OK;
}


bool motor_timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
  for (int i = 0; i < MAX_MOTOR_COUNT; i++)
  {
    motor_internal_t *motor_internal = motors[i];
    if (motor_internal == NULL) continue;
    struct motor *motor = motor_internal->motor;

    if (motor_internal->step_remaining <= 0)
    {
      gpio_set_level(motor->step_pin, 0);
      motor_internal->state = MOTOR_STILL_STATE;
      continue;
    }

    motor_internal->dt_us+=10; 

    if (motor_internal->dt_us >= motor_internal->period_us)
    {
      motor_internal->dt_us = 0;
      motor_internal->step_remaining--;
      motor_internal->step_count += motor_internal->dir_state ? -1 : 1;
    }
    if (motor_internal->dt_us <= STEP_ON_DELAY_US)
    {
      gpio_set_level(motor->step_pin, 1);
    }
    else
    {
      gpio_set_level(motor->step_pin, 0);
    }
  }

  return true;
}

inline bool is_motor_init(struct motor *motor)
{
  return motor != NULL && motor_get_id(motor) >= 0;
}

int8_t motor_get_id(struct motor *motor)
{
  ESP_RETURN_ON_FALSE(motor != NULL, -1, TAG, "Motor is NULL.");

  for (int i = 0; i < MAX_MOTOR_COUNT; i++)
  {
    if (motors[i] == NULL) continue;
    if (motor == motors[i]->motor) return i;
  }

  ESP_LOGW(TAG, "Motor %s ID not found.", motor->name);

  return -1;
}

