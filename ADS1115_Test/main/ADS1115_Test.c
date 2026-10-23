/*
 * ============================================================
 * ADS1115 — ESP32 WIRING / INSTRUCTIONS
 * ============================================================
 *
 * ADS1115       ESP32
 * -------------------
 * VDD     ->     3.3V
 * GND     ->     GND
 * SDA     ->     GPIO 21
 * SCL     ->     GPIO 22
 * ADDR    ->     GND
 *
 * ANALOG INPUTS:
 * AIN0    ->     Leave disconnected for initial test
 * AIN1    ->     Leave disconnected
 * AIN2    ->     Leave disconnected
 * AIN3    ->     Leave disconnected
 *
 * I2C ADDRESS:
 * ADDR -> GND
 * Address = 0x48
 *
 * IMPORTANT:
 * - Use 3.3V for VDD.
 * - Do not apply a voltage to AIN0 higher than the
 *   allowed ADS1115 input/supply limits.
 *
 * TEST:
 * The program reads AIN0 relative to GND.
 * It prints the raw ADC value and calculated voltage
 * every 1 second.
 *
 * ============================================================
 */

#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/i2c_master.h"

#define I2C_SDA_PIN     21
#define I2C_SCL_PIN     22

#define ADS1115_ADDR    0x48

static const char *TAG = "ADS1115";

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t ads1115;

// ---------------------------------------------------------
// Write a 16-bit value to an ADS1115 register
// ---------------------------------------------------------
static esp_err_t ads1115_write_register(uint8_t reg, uint16_t value)
{
    uint8_t data[3];

    data[0] = reg;
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;

    return i2c_master_transmit(ads1115, data, 3, -1);
}

// ---------------------------------------------------------
// Read a 16-bit ADS1115 register
// ---------------------------------------------------------
static esp_err_t ads1115_read_register(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];

    esp_err_t ret = i2c_master_transmit_receive(
        ads1115,
        &reg,
        1,
        data,
        2,
        -1
    );

    if (ret != ESP_OK)
    {
        return ret;
    }

    *value = ((uint16_t)data[0] << 8) | data[1];

    return ESP_OK;
}

// ---------------------------------------------------------
// Initialize I2C
// ---------------------------------------------------------
static esp_err_t i2c_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(
        i2c_new_master_bus(&bus_config, &i2c_bus)
    );

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ADS1115_ADDR,
        .scl_speed_hz = 100000,
    };

    return i2c_master_bus_add_device(
        i2c_bus,
        &device_config,
        &ads1115
    );
}

// ---------------------------------------------------------
// Configure ADS1115
// ---------------------------------------------------------
static esp_err_t ads1115_configure(void)
{
    /*
     * CONFIG REGISTER
     *
     * OS     = 1       Start single conversion
     * MUX    = 100     AIN0 vs GND
     * PGA    = 001     +/- 4.096V
     * MODE   = 1       Single-shot mode
     * DR     = 100     128 samples/sec
     */

    uint16_t config = 0;

    config |= (1 << 15);      // Start conversion

    config |= (4 << 12);      // AIN0 - GND

    config |= (1 << 9);       // +/- 4.096V

    config |= (1 << 8);       // Single-shot mode

    config |= (4 << 5);       // 128 SPS

    config |= 0x0003;         // Disable comparator

    return ads1115_write_register(0x01, config);
}

// ---------------------------------------------------------
// Read voltage from AIN0
// ---------------------------------------------------------
static esp_err_t ads1115_read_voltage(float *voltage)
{
    esp_err_t ret;

    ret = ads1115_configure();

    if (ret != ESP_OK)
    {
        return ret;
    }

    // Wait for conversion to complete
    vTaskDelay(pdMS_TO_TICKS(10));

    uint16_t raw;

    ret = ads1115_read_register(0x00, &raw);

    if (ret != ESP_OK)
    {
        return ret;
    }

    int16_t signed_raw = (int16_t)raw;

    /*
     * PGA = +/-4.096V
     *
     * ADS1115 resolution:
     * 4.096V / 32768 = 0.000125V
     */

    *voltage = signed_raw * 0.000125f;

    ESP_LOGI(
        TAG,
        "RAW: %d | ADC Voltage: %.4f V",
        signed_raw,
        *voltage
    );

    return ESP_OK;
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "       ADS1115 TEST");
    ESP_LOGI(TAG, "=================================");

    ESP_LOGI(TAG, "Initializing I2C...");

    ESP_ERROR_CHECK(i2c_init());

    ESP_LOGI(TAG, "I2C initialized.");
    ESP_LOGI(TAG, "ADS1115 address: 0x%02X", ADS1115_ADDR);

    // Check if ADS1115 is actually connected
    esp_err_t ret = i2c_master_probe(
        i2c_bus,
        ADS1115_ADDR,
        100
    );

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "ADS1115 FOUND!");
    }
    else
    {
        ESP_LOGE(TAG, "ADS1115 NOT FOUND!");
        ESP_LOGE(TAG, "Check SDA, SCL, VCC and GND.");
    }

    while (1)
    {
        float voltage = 0;

        ret = ads1115_read_voltage(&voltage);

        if (ret == ESP_OK)
        {
            ESP_LOGI(
                TAG,
                "Measurement: %.4f V",
                voltage
            );
        }
        else
        {
            ESP_LOGE(
                TAG,
                "ADS1115 read failed: %s",
                esp_err_to_name(ret)
            );
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}