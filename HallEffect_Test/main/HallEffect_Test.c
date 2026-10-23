/*
 * ============================================================
 * HALL EFFECT SENSOR — ESP32 WIRING / INSTRUCTIONS
 * ============================================================
 *
 * SENSOR:
 *     HW-484 V0.3 Hall Effect Sensor Module
 *
 * WIRING:
 *
 *     Hall +   ---> ESP32 3.3V
 *     Hall G   ---> ESP32 GND
 *     Hall A0  ---> ESP32 GPIO34
 *     Hall D0  ---> ESP32 GPIO27
 *
 * MODULE PINS:
 *
 *     A0 = Analog Hall sensor output
 *     G  = Ground
 *     +  = VCC
 *     D0 = Digital threshold output
 *
 * TEST:
 *
 *     A0 Raw = actual ESP32 ADC reading
 *     A0     = A0 Raw - 1900 (approximate zeroed value)
 *     D0     = Digital magnetic-field threshold
 *
 * ============================================================
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#define HALL_ADC_CHANNEL    ADC_CHANNEL_6
#define HALL_ADC_UNIT       ADC_UNIT_1
#define HALL_DIGITAL_GPIO   GPIO_NUM_27

// Approximate zero-field baseline
#define HALL_BASELINE       1900

static const char *TAG = "HALL_TEST";

void app_main(void)
{
    ESP_LOGI(TAG, "==============================");
    ESP_LOGI(TAG, "      HALL EFFECT TEST");
    ESP_LOGI(TAG, "==============================");

    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = HALL_ADC_UNIT,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &adc_config,
            &adc_handle
        )
    );

    adc_oneshot_chan_cfg_t adc_channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            HALL_ADC_CHANNEL,
            &adc_channel_config
        )
    );

    gpio_config_t digital_config = {
        .pin_bit_mask = (1ULL << HALL_DIGITAL_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(
        gpio_config(&digital_config)
    );

    ESP_LOGI(TAG, "Hall ADC initialized.");
    ESP_LOGI(TAG, "Hall digital input initialized.");

    while (1)
    {
        int adc_raw = 0;

        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                HALL_ADC_CHANNEL,
                &adc_raw
            )
        );

        int a0 = adc_raw - HALL_BASELINE;

        int digital_state =
            gpio_get_level(HALL_DIGITAL_GPIO);

        ESP_LOGI(
            TAG,
            "A0 Raw: %d | A0: %+d | D0: %s",
            adc_raw,
            a0,
            digital_state ? "HIGH" : "LOW"
        );

        vTaskDelay(
            pdMS_TO_TICKS(100)
        );
    }
}