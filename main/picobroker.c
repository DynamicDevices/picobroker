
// --------
// INCLUDES
// --------

#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

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

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/gpio.h"

#ifdef SUPPORT_SYSLOG
#include "udp_logging.h"
#endif

// -------
// DEFINES
// -------

#define SD_CARD_BASE_PATH "/sdcard"

#define SD_PIN_NUM_MISO 19
#define SD_PIN_NUM_MOSI 23
#define SD_PIN_NUM_CLK 18
#define SD_PIN_NUM_CS 4

#define SDSPI_HOST_M5STACK() { \
.flags = SDMMC_HOST_FLAG_SPI, \
.slot = VSPI_HOST, \
.max_freq_khz = SDMMC_FREQ_DEFAULT, \
.io_voltage = 3.3f, \
.init = &sdspi_host_init, \
.set_bus_width = NULL, \
.get_bus_width = NULL, \
.set_card_clk = &sdspi_host_set_card_clk, \
.do_transaction = &sdspi_host_do_transaction, \
.deinit = &sdspi_host_deinit, \
.io_int_enable = NULL, \
.io_int_wait = NULL, \
.command_timeout_ms = 0, \
}

#define SDSPI_SLOT_CONFIG_M5STACK() { \
.gpio_miso = SD_PIN_NUM_MISO, \
.gpio_mosi = SD_PIN_NUM_MOSI, \
.gpio_sck = SD_PIN_NUM_CLK, \
.gpio_cs = SD_PIN_NUM_CS, \
.gpio_cd = SDSPI_SLOT_NO_CD, \
.gpio_wp = SDSPI_SLOT_NO_WP, \
.dma_channel = 1 \
}

// - WIFI

#define SUPPORT_AP

#define ESP_AP_SSID "esp-ap"
#define ESP_AP_PASS "decafbad00"
#define ESP_AP_AUTH_MODE WIFI_AUTH_WPA2_PSK
#define ESP_AP_MAX_CONN 4
#define ESP_AP_CHANNEL 11

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
//#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
//#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
//#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY
#define EXAMPLE_ESP_WIFI_SSID      "router"
#define EXAMPLE_ESP_WIFI_PASS      "decafbad00"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// ----
// GPIO
// ----

#define SUPPORT_RELAY
//#define TEST_RELAY

#define GPIO_OUTPUT_RELAY    21
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_RELAY))

// --------------------
// EXTERNAL DECLARATIONS
// ---------------------

// The RSMB component main method
extern int main(int argc, char* argv[]);

// -------
// GLOBALS
// -------

static const char *TAG = "PicoBroker";
static int s_retry_num = 0;
static char _ipSTA[16+1] = "";

// -------------------------
// SD CARD SUPPORT FUNCTIONS
// -------------------------

int sdcard_test(void)
{
    ESP_LOGI(TAG, "Initializing SD card");

    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_M5STACK();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_M5STACK();

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    gpio_install_isr_service(ESP_INTR_FLAG_INTRDISABLED);

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_CARD_BASE_PATH, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return -1;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen(SD_CARD_BASE_PATH "/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return -1;
    }
    fprintf(f, "Hello %s!\n", card->cid.name);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat(SD_CARD_BASE_PATH "/foo.txt", &st) == 0) {
        // Delete it if it exists
        f_unlink(SD_CARD_BASE_PATH "/foo.txt");
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file");
    if (rename(SD_CARD_BASE_PATH "/hello.txt", SD_CARD_BASE_PATH "/foo.txt") != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return -1;
    }

    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file");
    f = fopen(SD_CARD_BASE_PATH "/foo.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return -1;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdmmc_unmount();
    ESP_LOGI(TAG, "Card unmounted");

    return ESP_OK;
}

// ----------------------
// WIFI SUPPORT FUNCTIONS
// ----------------------

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

        // Store IP
        snprintf(_ipSTA, sizeof(_ipSTA),  IPSTR, IP2STR(&event->ip_info.ip));

        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(void)
{
    ESP_LOGI(TAG, "wifi_init started.");

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

#ifdef SUPPORT_AP
    wifi_config_t ap_config = {
        .ap = {
            .ssid = ESP_AP_SSID,
            .password = ESP_AP_PASS,
            .authmode = ESP_AP_AUTH_MODE,
            .ssid_len = 0,
            .max_connection = ESP_AP_MAX_CONN,
            .channel = ESP_AP_CHANNEL
        }
    };


    esp_netif_t* wifiAP = esp_netif_create_default_wifi_ap();
    
    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192,168,1,1);
    IP4_ADDR(&ipInfo.gw, 0,0,0,0); // do not advertise as a gateway router
    IP4_ADDR(&ipInfo.netmask, 255,255,255,0);
    esp_netif_dhcps_stop(wifiAP);
    esp_netif_set_ip_info(wifiAP, &ipInfo);
    esp_netif_dhcps_start(wifiAP);
#endif

    wifi_config_t sta_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };


    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
#ifdef SUPPORT_AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config) );
#else
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
#endif
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

// ---------
// MAIN TASK
// ---------

void main_task(void *pvParameter)
{
    printf("ESP32 Starting Up...\n");

    // Stop the watchdog for this task 
    // (todo: some kind of broker callback)
    esp_task_wdt_delete(NULL);

#if TEST_SDCARD
    esp_err_t retVal = sdcard_test();

    if(retVal != ESP_OK)
        printf("- test_fatfs() failed: %d\n", retVal);
    printf("- test_fatfs() SUCCESS\n");
#endif

    // Setup SD Card over SPI

    sdmmc_host_t host = SDSPI_HOST_M5STACK();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_M5STACK();

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
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_CARD_BASE_PATH, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Initialize NVS

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();

#ifdef SUPPORT_UDP_LOGGING
    // Setup UDP logging
    #define CONFIG_LOG_UDP_IP "192.168.0.219"
    #define CONFIG_LOG_UDP_PORT 514
    udp_logging_init( CONFIG_LOG_UDP_IP, CONFIG_LOG_UDP_PORT, udp_logging_vprintf );
#endif

#ifdef SUPPORT_RELAY
    // Setup control relay
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    // turn relay on
    gpio_set_level(GPIO_OUTPUT_RELAY, 1);
#endif

#ifdef TEST_RELAY
    while(1) {
        gpio_set_level(GPIO_OUTPUT_RELAY, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(GPIO_OUTPUT_RELAY, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
#endif
    fflush(stdout);

    // Now run the RSMB broker code. Give it the name of a configuration file to load from SD Card
    char* argv[] = { "broker.exe", SD_CARD_BASE_PATH "/broker.cfg" };
    main(2, argv);

    // Shouldn't get here!

    printf("Broker Exited!\n");

#ifdef DEBUG
    while(1)
      ;
#endif
}
 
void app_main()
{
    xTaskCreate(&main_task, "main_task", 8192, NULL, 5, NULL);
}
