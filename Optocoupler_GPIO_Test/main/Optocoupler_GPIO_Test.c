#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "opto_test";

#define OPTO_LED_PIN GPIO_NUM_25   // ESP32 output pin driving the optocoupler LED side
#define OPTO_OUT_PIN GPIO_NUM_34   // ESP32 input pin reading the optocoupler transistor side

void app_main(void)
{
    // -------------------------------------------------------------------------
    // Wiring (for a basic optocoupler test):
    //
    // ESP32 GPIO25 ----[220Ω resistor]----+---- Optocoupler LED Anode
    //                                      |
    //                                     GND
    //
    // Optocoupler LED Cathode -------------- GND
    //
    // Optocoupler transistor side:
    //   Collector ---- 3.3V through 10k pull-up resistor
    //   Emitter ------------------------------- GND
    //   Collector/output node ------------------ ESP32 GPIO34
    //
    // When the LED is ON, the transistor turns ON and pulls GPIO34 LOW.
    // When the LED is OFF, the pull-up brings GPIO34 HIGH.
    // -------------------------------------------------------------------------

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OPTO_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf = (gpio_config_t){
        .pin_bit_mask = (1ULL << OPTO_OUT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_LOGI(TAG, "Optocoupler test started.");

    while (1) {
        gpio_set_level(OPTO_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));

        int output_state = gpio_get_level(OPTO_OUT_PIN);
        ESP_LOGI(TAG, "LED ON -> optocoupler output = %s",
                 output_state ? "HIGH" : "LOW");

        gpio_set_level(OPTO_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));

        output_state = gpio_get_level(OPTO_OUT_PIN);
        ESP_LOGI(TAG, "LED OFF -> optocoupler output = %s",
                 output_state ? "HIGH" : "LOW");
    }
}
