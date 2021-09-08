
#ifndef __SDCARD_H__
#define __SDCARD_H__

// Includes

#include "board_defs.h"

// Defines

#define SD_CARD_BASE_PATH "/sdcard"

#ifdef SD_CARD_SPI

#define SDSPI_HOST_M5STACK() { \
.flags = SDMMC_HOST_FLAG_SPI, \
.slot = SD_SPI_HOST, \
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
.gpio_miso = SD_SPI_MISO_PIN, \
.gpio_mosi = SD_SPI_MOSI_PIN, \
.gpio_sck = SD_SPI_CLK_PIN, \
.gpio_cs = SD_SPI_CS_PIN, \
.gpio_cd = SDSPI_SLOT_NO_CD, \
.gpio_wp = SDSPI_SLOT_NO_WP, \
.dma_channel = SD_SPI_HOST \
}

#define SDSPI_DEVICE_CONFIG_M5STACK() { \
.host_id = SD_SPI_HOST, \
.gpio_cs = SD_SPI_CS_PIN, \
.gpio_cd = SDSPI_SLOT_NO_CD, \
.gpio_wp = SDSPI_SLOT_NO_WP, \
}

#endif

// Prototypes

void sdcard_test(void *pvParameter);

#endif