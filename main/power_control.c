
#include "power_control.h"

#ifdef SUPPORT_POWER_CONTROL

void init_power_control(void)
{
    // Setup control relay
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = PWR_CONTROL_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    // turn relay on
    gpio_set_level(PWR_CONTROL_PIN, 1);

#ifdef DISABLED
    printf("*** PIN %d, %llu", PWR_CONTROL_PIN, PWR_CONTROL_PIN_SEL);
    while(1)
    {
        gpio_set_level(PWR_CONTROL_PIN, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(PWR_CONTROL_PIN, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
#endif
}

#endif // SUPPORT_POWER_CONTROL
