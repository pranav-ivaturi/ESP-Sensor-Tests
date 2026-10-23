/*
 * ============================================================
 * PT1000 — ESP32 WIRING / INSTRUCTIONS
 * ============================================================
 *
 * WIRING WILL BE DONE LATER.
 *
 * IMPORTANT:
 * A PT1000 is a resistive sensor.
 * It cannot be read directly like a digital sensor.
 *
 * This test assumes a resistor-divider interface:
 *
 *       3.3V
 *        |
 *      PT1000
 *        |
 *        +------ ESP32 ADC
 *        |
 *    Reference resistor
 *        |
 *       GND
 *
 * The reference resistor value must be known for
 * accurate temperature calculation.
 *
 * TEST:
 * ADC reading -> voltage -> resistance -> temperature
 *
 * ============================================================
 */

#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"


#define ADC_CHANNEL ADC_CHANNEL_0
#define ADC_UNIT    ADC_UNIT_1

#define REFERENCE_RESISTOR 1000.0f

static const char *TAG = "PT1000";


void app_main(void)
{
    ESP_LOGI(TAG, "==============================");
    ESP_LOGI(TAG, "        PT1000 TEST");
    ESP_LOGI(TAG, "==============================");

    adc_oneshot_unit_handle_t adc;

    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &adc_config,
            &adc
        )
    );

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc,
            ADC_CHANNEL,
            &channel_config
        )
    );

    ESP_LOGI(TAG, "ADC initialized.");

    while (1)
    {
        int raw = 0;

        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc,
                ADC_CHANNEL,
                &raw
            )
        );

        ESP_LOGI(
            TAG,
            "ADC Raw: %d",
            raw
        );

        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}