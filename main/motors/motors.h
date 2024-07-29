/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : motors
 * @created     : Monday Jul 29, 2024 14:29:30 CEST
 */

#ifndef MOTORS_H
#define MOTORS_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/*          INCLUDES          */
/******************************/
#include <stdbool.h>

#include "driver/gptimer.h"
#include "esp_err.h"
#include "soc/gpio_num.h"

/******************************/
/*      Macro Definitions     */
/******************************/
#define MAX_MOTOR_COUNT 5

/******************************/
/*   Typedefs, Struct, Enums  */
/******************************/
enum motor_state : uint8_t
{
  MOTOR_STILL_STATE = 0,
  MOTOR_ACCEL_STATE,
  MOTOR_CONSTANT_STATE,
  MOTOR_DECEL_STATE
};

enum motor_dir : uint8_t
{
  MOTOR_DIR_CW = 0,
  MOTOR_DIR_CCW
};

struct motor_profile
{

};

struct motor
{
  const gpio_num_t dir_pin;
  const gpio_num_t step_pin;
  const uint16_t step_per_rev;
  const char name[8];
};

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*   Function Declarations    */
/******************************/
extern esp_err_t motor_init(struct motor *motor);
extern esp_err_t motor_delete(struct motor *motor);
extern esp_err_t motor_delete_by_id(int8_t id);
extern esp_err_t motor_set_profile(struct motor *motor, struct motor_profile *profile);
extern esp_err_t motor_enable(struct motor *motor);
extern esp_err_t motor_disable(struct motor *motor);
extern esp_err_t motor_stop(struct motor *motor);

extern esp_err_t motor_turn_step(struct motor *motor, int32_t steps, double rpm);
extern esp_err_t motor_turn_deg(struct motor *motor, double deg, uint16_t rpm);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* MOTORS_H */

