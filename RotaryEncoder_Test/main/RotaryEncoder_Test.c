/*
 * ============================================================
 * ROTARY ENCODER — ESP32 WIRING / INSTRUCTIONS
 * ============================================================
 *
 * WIRING WILL BE DONE LATER.
 *
 * Typical rotary encoder:
 *
 * Encoder       ESP32
 * -------------------
 * VCC      ->   3.3V
 * GND      ->   GND
 * CLK/A    ->   GPIO 18
 * DT/B     ->   GPIO 19
 * SW       ->   GPIO 21
 *
 * TEST:
 * Turn the encoder:
 *   Clockwise        -> Position increases
 *   Counterclockwise -> Position decreases
 *
 * Press the encoder button:
 *   -> Button message is printed
 *
 * ============================================================
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/gpio.h"


#define ENCODER_A      18
#define ENCODER_B      19
#define ENCODER_BUTTON 21


static const char *TAG = "ENCODER";

static int position = 0;


void app_main(void)
{
    ESP_LOGI(TAG, "==============================");
    ESP_LOGI(TAG, "     ROTARY ENCODER TEST");
    ESP_LOGI(TAG, "==============================");

    // Configure encoder pins
    gpio_config_t encoder_config = {
        .pin_bit_mask =
            (1ULL << ENCODER_A) |
            (1ULL << ENCODER_B),

        .mode = GPIO_MODE_INPUT,

        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(
        gpio_config(&encoder_config)
    );


    // Configure encoder button
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << ENCODER_BUTTON),

        .mode = GPIO_MODE_INPUT,

        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(
        gpio_config(&button_config)
    );


    int last_a = gpio_get_level(ENCODER_A);
    int last_button = gpio_get_level(ENCODER_BUTTON);


    ESP_LOGI(TAG, "Encoder initialized.");
    ESP_LOGI(TAG, "Position: %d", position);


    while (1)
    {
        // Read encoder
        int a = gpio_get_level(ENCODER_A);
        int b = gpio_get_level(ENCODER_B);


        // Detect change on A
        if (a != last_a)
        {
            if (a == b)
            {
                position++;
            }
            else
            {
                position--;
            }

            ESP_LOGI(
                TAG,
                "Position: %d",
                position
            );

            last_a = a;
        }


        // Read button
        int button = gpio_get_level(ENCODER_BUTTON);

        if (button == 0 && last_button == 1)
        {
            ESP_LOGI(
                TAG,
                "BUTTON PRESSED!"
            );
        }

        last_button = button;


        vTaskDelay(
            pdMS_TO_TICKS(2)
        );
    }
}