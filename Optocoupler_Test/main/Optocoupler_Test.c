#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define OPTO_PIN  GPIO_NUM_27

void app_main(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << OPTO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf);

    printf("\nReading GPIO%d every 1 s (internal pull-up on)\n\n", OPTO_PIN);

    while (1) {
        printf("GPIO%d = %d\n", OPTO_PIN, gpio_get_level(OPTO_PIN));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}