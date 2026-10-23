/*
 * ============================================================
 * LVMS TEST — ESP32 WIRING / INSTRUCTIONS
 * ============================================================
 *
 * IMPORTANT:
 * The exact LVMS wiring depends on the specific LVMS
 * model/interface.
 *
 * DO NOT CONNECT THE LVMS TO THE ESP32 YET.
 *
 * Once the exact LVMS model is confirmed, update:
 *
 *     LVMS_GPIO
 *
 * and the wiring instructions below.
 *
 * CURRENT TEST:
 *     ESP32 reads one digital input.
 *
 * GPIO 27 is being reserved as the LVMS signal input.
 *
 * Expected concept:
 *
 *     LVMS SIGNAL  ---> ESP32 GPIO 27
 *     LVMS GND     ---> ESP32 GND
 *
 * DO NOT connect an unknown LVMS output directly until
 * we confirm that its signal voltage is ESP32-safe (3.3 V).
 *
 * ============================================================
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/gpio.h"

#define LVMS_GPIO GPIO_NUM_27

static const char *TAG = "LVMS_TEST";

void app_main(void)
{
    ESP_LOGI(TAG, "==============================");
    ESP_LOGI(TAG, "          LVMS TEST");
    ESP_LOGI(TAG, "==============================");

    gpio_config_t lvms_config = {
        .pin_bit_mask = (1ULL << LVMS_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&lvms_config));

    ESP_LOGI(TAG, "LVMS GPIO initialized: GPIO %d", LVMS_GPIO);

    while (1)
    {
        int state = gpio_get_level(LVMS_GPIO);

        ESP_LOGI(
            TAG,
            "LVMS state: %s",
            state ? "HIGH" : "LOW"
        );

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}x