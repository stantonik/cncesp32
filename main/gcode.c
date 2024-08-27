/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : gcode
 * @created     : Tuesday Aug 27, 2024 02:13:43 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "gcode.h"

#include <string.h>
#include <errno.h>

#include "esp_check.h"

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*     Static Variables       */
/******************************/
#define TAG "gcode reader"

static void (*cmd_callback)(char, int);

static float params[26];
static char last_cmd_type;
static uint16_t last_cmd_number;

/******************************/
/*    Function Prototypes     */
/******************************/

/******************************/
/*   Function Definitions     */
/******************************/
esp_err_t gcode_reset()
{
  for (int i = 0; i < sizeof(params) / sizeof(float); i++) params[i] = 0;
  last_cmd_number = 0;
  last_cmd_type = 0;

  return ESP_OK;
}

esp_err_t gcode_read_file(FILE *file)
{
  ESP_RETURN_ON_FALSE(file != NULL, -1, TAG, "null file reading"); 

  char line[256];
  while (fgets(line, sizeof(line), file) != NULL) 
  {
    gcode_read_cmd(line);
  }

  return ESP_OK;
}

esp_err_t gcode_read_cmd(const char *cmd)
{
  int i = 0;
  char line[256]; 

  const char *comment_start = strchr(cmd, ';');
  if (comment_start != NULL)
  {
    size_t len = comment_start - cmd;
    if (len >= sizeof(line)) len = sizeof(line) - 1;
    strncpy(line, cmd, len);
    line[len] = '\0';
  }
  else
  {
    strncpy(line, cmd, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
  }

  char *token = strtok(line, " ");
  while (token != NULL)
  {
    char *endptr;
    errno = 0;

    if (i == 0)
    {
      char type = token[0];
      ESP_RETURN_ON_FALSE(type == 'G' || type == 'M', -1, TAG, "command type '%c' not valid", type);
      last_cmd_type = type;

      long number = strtol(token + 1, &endptr, 10);
      ESP_RETURN_ON_FALSE(errno == 0 && *endptr == '\0', -1, TAG, "command number not valid");
      last_cmd_number = (int)number;
    }
    else
    {
      int paramtype = (int)token[0] - 65;
      ESP_RETURN_ON_FALSE(paramtype >= 0 && paramtype < 26, -1, TAG, "param type not valid");

      float paramval = strtof(token + 1, &endptr);
      ESP_RETURN_ON_FALSE(errno == 0 && *endptr == '\0', -1, TAG, "param value not valid");
      params[paramtype] = paramval;
    }
    i++;
    token = strtok(NULL, " ");
  }

  if (cmd_callback != NULL && i > 0) cmd_callback(last_cmd_type, last_cmd_number);

  return ESP_OK;
}

float gcode_get_param_value(char param)
{
  int ind = (int)param - 65;
  if (ind < 0 || ind >= 26) return 0;
  return params[ind];
}

esp_err_t gcode_get_last_cmd(char *type, uint16_t *number)
{
  if (type != NULL) *type = last_cmd_type;
  if (number != NULL) *number = last_cmd_number;

  return ESP_OK;
}

esp_err_t gcode_set_cmd_callback(void (*callback)(char, int))
{
  ESP_RETURN_ON_FALSE(callback != NULL, -1, TAG, "null cmd callback"); 

  cmd_callback = callback;

  return ESP_OK;
}
