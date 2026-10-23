/*
 * ============================================================
 * GY-906 / MLX90614 IR TEMPERATURE SENSOR
 * ESP32 TEST
 * ============================================================
 *
 * WIRING:
 *
 * GY-906 Pin       ESP32
 * -----------------------
 * VCC       --->   3.3V
 * GND       --->   GND
 * SDA       --->   GPIO 21
 * SCL       --->   GPIO 22
 *
 * I2C ADDRESS:
 * GY-906 / MLX90614 = 0x5A
 *
 * ============================================================
 */

#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"


/* ============================================================
 * PIN CONFIGURATION
 * ============================================================ */

#define SDA_PIN         21
#define SCL_PIN         22

#define GY906_ADDR      0x5A
#define I2C_FREQ_HZ     100000


/* MLX90614 RAM registers */

#define REG_AMBIENT     0x06
#define REG_OBJECT1     0x07


static const char *TAG = "GY906";

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t sensor_handle;


/* ============================================================
 * READ 16-BIT REGISTER
 * ============================================================ */

static esp_err_t gy906_read_register(
    uint8_t reg,
    uint16_t *value
)
{
    uint8_t data[3];

    /*
     * MLX90614 response:
     *
     * data[0] = Low byte
     * data[1] = High byte
     * data[2] = PEC
     */

    esp_err_t err = i2c_master_transmit_receive(
        sensor_handle,
        &reg,
        1,
        data,
        3,
        100
    );

    if (err != ESP_OK)
    {
        return err;
    }

    *value =
        ((uint16_t)data[1] << 8) |
        data[0];

    return ESP_OK;
}


/* ============================================================
 * CONVERT RAW VALUE TO CELSIUS
 * ============================================================ */

static float raw_to_celsius(uint16_t raw)
{
    /*
     * MLX90614 temperature formula:
     *
     * Temperature = RAW × 0.02 - 273.15
     */

    return ((float)raw * 0.02f) - 273.15f;
}


/* ============================================================
 * INITIALIZE I2C
 * ============================================================ */

static esp_err_t i2c_init(void)
{
    /*
     * ESP32 I2C:
     *
     * SDA = GPIO21
     * SCL = GPIO22
     */

    i2c_master_bus_config_t bus_config =
    {
        .i2c_port = I2C_NUM_0,

        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,

        .clk_source = I2C_CLK_SRC_DEFAULT,

        .glitch_ignore_cnt = 7,

        .flags.enable_internal_pullup = true
    };

    esp_err_t err = i2c_new_master_bus(
        &bus_config,
        &bus_handle
    );

    if (err != ESP_OK)
    {
        return err;
    }


    /*
     * Add GY-906 as an I2C device.
     *
     * Address = 0x5A
     * Speed   = 100 kHz
     */

    i2c_device_config_t device_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,

        .device_address = GY906_ADDR,

        .scl_speed_hz = I2C_FREQ_HZ
    };

    return i2c_master_bus_add_device(
        bus_handle,
        &device_config,
        &sensor_handle
    );
}


/* ============================================================
 * CHECK IF SENSOR IS CONNECTED
 * ============================================================ */

static esp_err_t gy906_check(void)
{
    esp_err_t err = i2c_master_probe(
        bus_handle,
        GY906_ADDR,
        100
    );

    if (err == ESP_OK)
    {
        ESP_LOGI(
            TAG,
            "GY-906 / MLX90614 detected!"
        );
    }
    else
    {
        ESP_LOGE(
            TAG,
            "GY-906 not detected!"
        );
    }

    return err;
}


/* ============================================================
 * MAIN
 * ============================================================ */

void app_main(void)
{
    ESP_LOGI(
        TAG,
        "GY-906 IR Temperature Sensor Test"
    );


    /* --------------------------------------------------------
     * INITIALIZE I2C
     * -------------------------------------------------------- */

    ESP_LOGI(
        TAG,
        "Initializing I2C..."
    );

    ESP_ERROR_CHECK(i2c_init());

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );


    /* --------------------------------------------------------
     * CHECK SENSOR
     * -------------------------------------------------------- */

    if (gy906_check() != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "GY-906 not found!"
        );

        ESP_LOGE(
            TAG,
            "Check wiring:"
        );

        ESP_LOGE(
            TAG,
            "VCC -> 3.3V"
        );

        ESP_LOGE(
            TAG,
            "GND -> GND"
        );

        ESP_LOGE(
            TAG,
            "SDA -> GPIO21"
        );

        ESP_LOGE(
            TAG,
            "SCL -> GPIO22"
        );

        ESP_LOGE(
            TAG,
            "I2C Address -> 0x5A"
        );


        while (1)
        {
            vTaskDelay(
                pdMS_TO_TICKS(1000)
            );
        }
    }


    ESP_LOGI(
        TAG,
        "Starting temperature measurements..."
    );


    /* --------------------------------------------------------
     * CONTINUOUS MEASUREMENT LOOP
     * -------------------------------------------------------- */

    while (1)
    {
        uint16_t ambient_raw = 0;
        uint16_t object_raw = 0;


        /* Read ambient temperature */

        esp_err_t ambient_err =
            gy906_read_register(
                REG_AMBIENT,
                &ambient_raw
            );


        /* Read object temperature */

        esp_err_t object_err =
            gy906_read_register(
                REG_OBJECT1,
                &object_raw
            );


        if (
            ambient_err == ESP_OK &&
            object_err == ESP_OK
        )
        {
            float ambient_temp =
                raw_to_celsius(
                    ambient_raw
                );

            float object_temp =
                raw_to_celsius(
                    object_raw
                );


            ESP_LOGI(
                TAG,
                "Ambient: %.2f °C | Object: %.2f °C",
                ambient_temp,
                object_temp
            );
        }
        else
        {
            ESP_LOGE(
                TAG,
                "Temperature read failed"
            );

            ESP_LOGE(
                TAG,
                "Ambient error: %s",
                esp_err_to_name(ambient_err)
            );

            ESP_LOGE(
                TAG,
                "Object error: %s",
                esp_err_to_name(object_err)
            );
        }


        /*
         * Take a reading every 500 ms.
         */

        vTaskDelay(
            pdMS_TO_TICKS(500)
            );
    }
}