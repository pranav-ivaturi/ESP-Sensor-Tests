/*
 * ============================================================
 * RELAY TEST — ESP32 WIRING / INSTRUCTIONS
 * ============================================================
 *
 * WIRING WILL BE DONE LATER.
 *
 * IMPORTANT:
 * Do NOT connect a relay coil directly to an ESP32 GPIO.
 *
 * This program assumes a relay module with a suitable
 * transistor/driver circuit and logic input.
 *
 * Default test GPIO:
 *
 * ESP32 GPIO 23  --->  Relay IN
 * ESP32 GND      --->  Relay GND
 *
 * Relay module VCC should be connected according to the
 * module's required supply voltage.
 *
 * TEST BEHAVIOR:
 *   - Relay OFF for 2 seconds
 *   - Relay ON  for 2 seconds
 *   - Repeat forever
 *
 * ============================================================
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/gpio.h"

#define RELAY_GPIO GPIO_NUM_23

static const char *TAG = "RELAY_TEST";

void app_main(void)
{
    ESP_LOGI(TAG, "==============================");
    ESP_LOGI(TAG, "         RELAY TEST");
    ESP_LOGI(TAG, "==============================");

    gpio_config_t relay_config = {
        .pin_bit_mask = (1ULL << RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&relay_config));

    ESP_LOGI(TAG, "Relay GPIO initialized: GPIO %d", RELAY_GPIO);

    while (1)
    {
        // Relay OFF
        gpio_set_level(RELAY_GPIO, 0);
        ESP_LOGI(TAG, "Relay OFF");

        vTaskDelay(pdMS_TO_TICKS(2000));

        // Relay ON
        gpio_set_level(RELAY_GPIO, 1);
        ESP_LOGI(TAG, "Relay ON");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}