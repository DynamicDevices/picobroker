
// Includes

#include "display.h"

// Defines

// Statics

scr_driver_t     g_lcd;
scr_info_t       g_lcd_info;

static i2c_bus_handle_t s_spi_bus_handle = NULL;

static const char *TAG = "screen";

// Functions

void screen_clear(scr_driver_t *lcd, int color)
{
    scr_info_t lcd_info;
    lcd->get_info(&lcd_info);
    uint16_t *buffer = malloc(lcd_info.width * sizeof(uint16_t));
    if (NULL == buffer) {
        for (size_t y = 0; y < lcd_info.height; y++) {
            for (size_t x = 0; x < lcd_info.width; x++) {
                lcd->draw_pixel(x, y, color);
            }
        }
    } else {
        for (size_t i = 0; i < lcd_info.width; i++) {
            buffer[i] = color;
        }

        for (int y = 0; y < lcd_info.height; y++) {
            lcd->draw_bitmap(0, y, lcd_info.width, 1, buffer);
        }

        free(buffer);
    }
}

static esp_err_t display_spi_bus_init(void)
{
    spi_config_t bus_conf = {
        .miso_io_num = DISPLAY_SPI_MISO_PIN,
        .mosi_io_num = DISPLAY_SPI_MOSI_PIN,
        .sclk_io_num = DISPLAY_SPI_CLK_PIN,
        .max_transfer_sz = 2*320*240 + 10,
    };
    s_spi_bus_handle = spi_bus_create(DISPLAY_SPI_HOST, &bus_conf);

    return ESP_OK;
}

void init_display(void)
{
    esp_err_t ret;

    printf("Create bus...\n");

    display_spi_bus_init();

    scr_interface_spi_config_t spi_lcd_cfg = {
        .spi_bus = s_spi_bus_handle,
        .pin_num_cs = DISPLAY_SPI_CS_PIN,
        .pin_num_dc = DISPLAY_SPI_DC_PIN,
        .clk_freq = DISPLAY_SPI_CLOCK_FREQ,
        .swap_data = true,
    };

    printf("Create device...\n");

    scr_interface_driver_t *iface_drv;
    scr_interface_create(SCREEN_IFACE_SPI, &spi_lcd_cfg, &iface_drv);

    printf("Find driver...\n");

    ret = scr_find_driver(SCREEN_CONTROLLER_ILI9342, &g_lcd);
    if (ESP_OK != ret) {
        return;
        ESP_LOGE(TAG, "screen find failed");
    }

    scr_controller_config_t lcd_cfg = {
        .interface_drv = iface_drv,
        .pin_num_rst = DISPLAY_SPI_RESET_PIN,
        .pin_num_bckl = DISPLAY_SPI_BL_PIN,
        .rst_active_level = 0,
        .bckl_active_level = 1,
        .offset_hor = 0,
        .offset_ver = 0,
        .width = 320,
        .height = 240,
        .rotate = SCR_DIR_LRTB,
    };

    printf("Init screen...\n");

    ret = g_lcd.init(&lcd_cfg);
}
