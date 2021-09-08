#ifndef __PLATFORM_DEFS_H_
#define __PLATFORM_DEFS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_vfs_fat.h"

#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "sdkconfig.h"

#endif // __PLATFORM_DEFS_H_
