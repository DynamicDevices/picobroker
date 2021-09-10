//
// Board specific definitions
//

#ifndef __BOARD_DEFS_H_
#define __BOARD_DEFS_H_

#include "platform_defs.h"

#ifdef CONFIG_BOARD_M5STACK

// Display support definitions

#define SUPPORT_DISPLAY 

// todo: Not entirely sure how this will impact SD Card
#define DISPLAY_SPI_CLOCK_FREQ      20000000

#define DISPLAY_SPI_HOST            SPI3_HOST

#define DISPLAY_SPI_MISO_PIN        19
#define DISPLAY_SPI_MOSI_PIN        23
#define DISPLAY_SPI_CLK_PIN         18
#define DISPLAY_SPI_CS_PIN          14
#define DISPLAY_SPI_DC_PIN          27
#define DISPLAY_SPI_RESET_PIN       33
#define DISPLAY_SPI_BL_PIN          32

// SD card support definitions

// SD card is SPI 
#define SD_CARD_SPI

// SD Card SPI host
#define SD_SPI_HOST                 SPI3_HOST

// SD Card SPI pinout 

#define SD_SPI_MISO_PIN             19
#define SD_SPI_MOSI_PIN             23
#define SD_SPI_CLK_PIN              18
#define SD_SPI_CS_PIN               4

// External power control
#define PWR_CONTROL_PIN             21

#elif CONFIG_BOARD_ESP32_GENERIC

#elif CONFIG_BOARD_OLIMEX_GATEWAY

#define SD_CARD_MMC

#else

#error Board type is not defined !!!

#endif

// Functional definitions

//#define TEST_SDCARD

#define SUPPORT_WIFI_AP

#endif
