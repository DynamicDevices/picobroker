
#include "sdcard.h"

#include <string.h>

static const char *TAG = "sdcard";

void sdcard_test(void *pvParameter)
{
    while(1)
    {
        // Use POSIX and C standard library functions to work with files.
        // First create a file.
        ESP_LOGI(TAG, "Opening file");
        FILE* f = fopen(SD_CARD_BASE_PATH "/hello.txt", "w");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            goto err;
        }
        fprintf(f, "Hello PicoBroker World!\n");
        fclose(f);
        ESP_LOGI(TAG, "File written");

        // Check if destination file exists before renaming
        struct stat st;
        if (stat(SD_CARD_BASE_PATH "/foo.txt", &st) == 0) {
            // Delete it if it exists
            f_unlink(SD_CARD_BASE_PATH "/foo.txt");
        }

        // Rename original file
//        ESP_LOGI(TAG, "Renaming file");
//        if (rename(SD_CARD_BASE_PATH "/hello.txt", SD_CARD_BASE_PATH "/foo.txt") != 0) {
//            ESP_LOGE(TAG, "Rename failed");
//            goto err;
//        }

        // Open renamed file for reading
        ESP_LOGI(TAG, "Reading file");
//        f = fopen(SD_CARD_BASE_PATH "/foo.txt", "r");
        f = fopen(SD_CARD_BASE_PATH "/hello.txt", "r");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for reading");
            goto err;
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

err: 
        vTaskDelay(10000 / portTICK_RATE_MS);
    }

}