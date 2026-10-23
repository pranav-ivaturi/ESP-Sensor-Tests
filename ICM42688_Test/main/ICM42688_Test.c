#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/i2c_master.h"


#define I2C_SDA_PIN     21
#define I2C_SCL_PIN     22

#define ICM42688_ADDR_1 0x68
#define ICM42688_ADDR_2 0x69

#define REG_WHO_AM_I    0x75
#define REG_PWR_MGMT0   0x4E
#define REG_ACCEL_DATA  0x1F


static const char *TAG = "ICM42688";

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t imu;


static esp_err_t imu_write_register(uint8_t reg, uint8_t value)
{
    uint8_t data[2];

    data[0] = reg;
    data[1] = value;

    return i2c_master_transmit(imu, data, 2, -1);
}


static esp_err_t imu_read_register(uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(
        imu,
        &reg,
        1,
        value,
        1,
        -1
    );
}


static esp_err_t imu_read_registers(
    uint8_t reg,
    uint8_t *data,
    size_t length
)
{
    return i2c_master_transmit_receive(
        imu,
        &reg,
        1,
        data,
        length,
        -1
    );
}


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

    return ESP_OK;
}


static esp_err_t imu_init(uint8_t address)
{
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 400000,
    };

    esp_err_t ret = i2c_master_bus_add_device(
        i2c_bus,
        &device_config,
        &imu
    );

    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t who_am_i = 0;

    ret = imu_read_register(REG_WHO_AM_I, &who_am_i);

    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(
        TAG,
        "WHO_AM_I = 0x%02X",
        who_am_i
    );

    if (who_am_i != 0x47) {
        ESP_LOGE(
            TAG,
            "Unexpected WHO_AM_I value!"
        );

        return ESP_ERR_INVALID_RESPONSE;
    }

    /*
     * Enable:
     * - Gyroscope Low-Noise mode
     * - Accelerometer Low-Noise mode
     */
    ret = imu_write_register(
        REG_PWR_MGMT0,
        0x0F
    );

    if (ret != ESP_OK) {
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    return ESP_OK;
}


static esp_err_t imu_read_data(void)
{
    uint8_t data[12];

    esp_err_t ret = imu_read_registers(
        REG_ACCEL_DATA,
        data,
        sizeof(data)
    );

    if (ret != ESP_OK) {
        return ret;
    }

    int16_t accel_x =
        ((int16_t)data[0] << 8) |
        data[1];

    int16_t accel_y =
        ((int16_t)data[2] << 8) |
        data[3];

    int16_t accel_z =
        ((int16_t)data[4] << 8) |
        data[5];

    int16_t gyro_x =
        ((int16_t)data[6] << 8) |
        data[7];

    int16_t gyro_y =
        ((int16_t)data[8] << 8) |
        data[9];

    int16_t gyro_z =
        ((int16_t)data[10] << 8) |
        data[11];


    ESP_LOGI(
        TAG,
        "ACCEL  X: %6d  Y: %6d  Z: %6d",
        accel_x,
        accel_y,
        accel_z
    );

    ESP_LOGI(
        TAG,
        "GYRO   X: %6d  Y: %6d  Z: %6d",
        gyro_x,
        gyro_y,
        gyro_z
    );

    return ESP_OK;
}


void app_main(void)
{
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "       ICM-42688-P TEST");
    ESP_LOGI(TAG, "================================");

    ESP_LOGI(TAG, "Initializing I2C...");

    ESP_ERROR_CHECK(i2c_init());

    ESP_LOGI(TAG, "I2C initialized.");

    /*
     * Try address 0x68 first.
     */
    ESP_LOGI(
        TAG,
        "Checking ICM-42688 at address 0x68..."
    );

    esp_err_t ret = i2c_master_probe(
        i2c_bus,
        ICM42688_ADDR_1,
        100
    );

    uint8_t detected_address = 0;

    if (ret == ESP_OK) {
        detected_address = ICM42688_ADDR_1;

        ESP_LOGI(
            TAG,
            "Device detected at 0x68!"
        );
    }
    else {
        /*
         * Try address 0x69.
         */
        ESP_LOGI(
            TAG,
            "0x68 not found. Checking 0x69..."
        );

        ret = i2c_master_probe(
            i2c_bus,
            ICM42688_ADDR_2,
            100
        );

        if (ret == ESP_OK) {
            detected_address = ICM42688_ADDR_2;

            ESP_LOGI(
                TAG,
                "Device detected at 0x69!"
            );
        }
    }


    if (detected_address == 0) {
        ESP_LOGE(
            TAG,
            "ICM-42688-P NOT FOUND!"
        );

        ESP_LOGE(
            TAG,
            "No device detected at 0x68 or 0x69."
        );

        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }


    /*
     * Initialize the detected device.
     */
    ret = imu_init(detected_address);

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "ICM-42688 initialization failed: %s",
            esp_err_to_name(ret)
        );

        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }


    ESP_LOGI(TAG, "ICM-42688-P FOUND!");
    ESP_LOGI(TAG, "Sensor initialized successfully.");


    while (1)
    {
        ret = imu_read_data();

        if (ret != ESP_OK) {
            ESP_LOGE(
                TAG,
                "Sensor read failed: %s",
                esp_err_to_name(ret)
            );
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}