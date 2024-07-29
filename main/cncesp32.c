#include "motors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "cncesp";

void app_main(void)
{
  struct motor motor1 = 
  {
    .dir_pin = 25,
    .step_pin = 26,
    .step_per_rev = 48,
    .name = "X"
  };

  struct motor motor2 = 
  {
    .dir_pin = 25,
    .step_pin = 19,
    .step_per_rev = 48,
    .name = "Y"
  };

  motor_init(&motor1);
  motor_enable(&motor1);
  motor_turn_step(&motor1, motor1.step_per_rev * 10, 30, MOTOR_DIR_CW);

  motor_init(&motor2);
  motor_enable(&motor2);
  motor_turn_step(&motor2, motor2.step_per_rev * 20, 100, MOTOR_DIR_CW);

  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
