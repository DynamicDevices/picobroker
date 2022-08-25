THIS IS A DODGY COMMIT

//comment
// --------
// INCLUDES
// --------

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define BOARD_M5STACK 

#include "board_defs.h"

#include "udp_logging.h"

#include "wifi.h"
#include "sdcard.h"
#include "display.h"
#include "power_control.h"

#define BASE_PATH SD_CARD_BASE_PATH

#if defined (CONFIG_FILES_IN_SPIFFS)
#include "esp_spiffs.h"
#undef BASE_PATH 
// todo: base path in rsmb is hardcoded to sdcard for now
//       so we use that here. I'd like to be able to use
//       /spiffs as a mount so we can have spiffs and sd
//       card supported 
//#define BASE_PATH "/spiffs"
#define BASE_PATH SD_CARD_BASE_PATH
#endif

// --------------------
// EXTERNAL DECLARATIONS
// ---------------------

// The RSMB component main method
extern int main(int argc, char* argv[]);

// Statics

static const char *TAG = "picobroker";

// Implementation
//hello
#ifdef SUPPORT_DISPLAY 

void lcd_task(void *pvParameter)
{
    while(1) 
    {
        vTaskDelay(1000 / portTICK_RATE_MS);

#ifdef TEST_DISPLAY
        screen_clear(&g_lcd, COLOR_BLACK);
//        vTaskDelay(1000 / portTICK_RATE_MS);
        screen_clear(&g_lcd, COLOR_RED);
#endif
    }

}

#endif

void main_task(void *pvParameter)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "ESP32 Starting Up...\n");

    // Stop the watchdog for this task 
    // (todo: some kind of broker callback)
    esp_task_wdt_delete(NULL);

#if defined(SUPPORT_DISPLAY)
    init_display();

    g_lcd.get_info(&g_lcd_info);
    ESP_LOGI(TAG, "Screen name:%s | width:%d | height:%d", g_lcd_info.name, g_lcd_info.width, g_lcd_info.height);

    screen_clear(&g_lcd, COLOR_BLACK);

    xTaskCreate(&lcd_task, "lcd_task", 8192, NULL, 5, NULL);
#endif

#if defined(CONFIG_SUPPORT_SD_CARD)

#if defined(SD_CARD_SPI)

    ESP_LOGI(TAG, "Mounting SD SPI on " BASE_PATH);

    // Setup SD Card over SPI
    sdmmc_host_t host_config = SDSPI_HOST_M5STACK();
#if defined(SUPPORT_DISPLAY)
    sdspi_device_config_t device_config = SDSPI_DEVICE_CONFIG_M5STACK();
#else
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_M5STACK();
#endif

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    gpio_install_isr_service(ESP_INTR_FLAG_INTRDISABLED);

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t* card;
#if defined(SUPPORT_DISPLAY)
    ret = esp_vfs_fat_sdspi_mount(SD_CARD_BASE_PATH, &host_config, &device_config, &mount_config, &card);
#else
    ret = esp_vfs_fat_sdmmc_mount(SD_CARD_BASE_PATH, &host_config, &slot_config, &mount_config, &card);
#endif
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
    }

#elif defined(SD_CARD_MMC)

    ESP_LOGI(TAG, "Mounting MMC on " BASE_PATH);

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz = SDMMC_FREQ_PROBING;

        // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

        // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and formatted
    // in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t* card;
    ret = esp_vfs_fat_sdmmc_mount(SD_CARD_BASE_PATH, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
        }
    }

#endif // SPI / HS2

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

#if defined(TEST_SDCARD)
    xTaskCreate(&sdcard_test, "sdcard_test", 8192, NULL, 5, NULL);

    while(1)
        ;
#endif

#elif defined (CONFIG_FILES_IN_SPIFFS)

    esp_vfs_spiffs_conf_t conf = {
      .base_path = BASE_PATH,
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };

    ESP_LOGI(TAG, "Mounting SPIFFS on " BASE_PATH);

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
#else

#error You have to either support an SD card or store files in SPIFFS !

#endif

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialise WiFi
    ESP_LOGI(TAG, "Start WiFi\n");
    wifi_init();

#ifdef SUPPORT_UDP_LOGGING
    // Setup UDP logging
    #define CONFIG_LOG_UDP_IP "192.168.0.219"
    #define CONFIG_LOG_UDP_PORT 514
    udp_logging_init( CONFIG_LOG_UDP_IP, CONFIG_LOG_UDP_PORT, udp_logging_vprintf );
#endif

#ifdef SUPPORT_POWER_CONTROL
    init_power_control();
#endif

    // Make sure any serial output is gone before we kick off the broker
    fflush(stdout);

    // Now run the RSMB broker code. Give it the name of a configuration file to load from SD Card
    char* argv[] = { "broker.exe", BASE_PATH "/broker.cfg" };
    main(2, argv);

    // Shouldn't get here!
    ESP_LOGI(TAG, "Broker Exited!\n");
    fflush(stdout);

#ifdef DEBUG
    while(1)
      ;
#endif
}
 
void app_main()
{
    xTaskCreate(&main_task, "main_task", 8192, NULL, 5, NULL);
// create new task here
}
