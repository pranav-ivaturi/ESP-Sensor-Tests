#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

#define ADC_CHANNEL ADC_CHANNEL_6

#define SENSOR_MIN_V 0.5f
#define SENSOR_MAX_V 4.5f
#define MAX_PRESSURE_PA 1000000.0f

// 10k + 20k voltage divider
#define DIVIDER_RATIO 1.5f

void app_main(void)
{
    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    adc_oneshot_config_channel(
        adc_handle,
        ADC_CHANNEL,
        &adc_config
    );

    printf("\n");
    printf("========================================\n");
    printf("     HYDRAULIC PRESSURE SENSOR TEST\n");
    printf("========================================\n");

    while (1)
    {
        int raw;

        adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);

        float adc_voltage = ((float)raw / 4095.0f) * 3.3f;

        // Convert ADC voltage back to sensor output voltage
        float sensor_voltage = adc_voltage * DIVIDER_RATIO;

        // Convert voltage to pressure
        float pressure_pa =
            ((sensor_voltage - SENSOR_MIN_V) /
            (SENSOR_MAX_V - SENSOR_MIN_V))
            * MAX_PRESSURE_PA;

        if (pressure_pa < 0)
            pressure_pa = 0;

        if (pressure_pa > MAX_PRESSURE_PA)
            pressure_pa = MAX_PRESSURE_PA;

        printf(
            "RAW: %4d | Sensor: %.3f V | Pressure: %.0f Pa\n",
            raw,
            sensor_voltage,
            pressure_pa
        );

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}