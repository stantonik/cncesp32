/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : sd
 * @created     : Tuesday Aug 27, 2024 16:34:42 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "sd.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_check.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

/******************************/
/*     Global Variables       */
/******************************/
#define TAG "sd"

/******************************/
/*     Static Variables       */
/******************************/

/******************************/
/*    Function Prototypes     */
/******************************/

/******************************/
/*   Function Definitions     */
/******************************/
esp_err_t sd_init()
{
  esp_vfs_fat_sdmmc_mount_config_t mount_config = 
  {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };
  sdmmc_card_t *card;
  const char mount_point[] = SD_MOUNT_POINT;

  ESP_LOGI(TAG, "initializing using sdmmc peripheral...");

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.max_freq_khz = 40000;

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  ESP_LOGI(TAG, "mounting filesystem...");
  ESP_RETURN_ON_ERROR(esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card), TAG, "failed");

  ESP_LOGI(TAG, "filesystem mounted");

  sdmmc_card_print_info(stdout, card);

  struct stat st = {0};

  if (stat("/sd/system", &st) == -1) 
  {
    mkdir("/sd/system", 0700);
  }

  return ESP_OK;
}


