/*
 * =====================================================================
 * VL53L0X Distance Test (ESP32, ESP-IDF, I2C)
 * =====================================================================
 *
 * WIRING:
    *   VL53L0X VIN   ->  ESP32 3V3
 *   VL53L0X GND   ->  ESP32 GND
 *   VL53L0X SDA   ->  ESP32 GPIO21
 *   VL53L0X SCL   ->  ESP32 GPIO22
 *   VL53L0X XSHUT ->  3V3 (or leave unconnected if the board pulls it high)
 *   VL53L0X GPIO1 ->  not connected (interrupt output, unused)
 *
 * NOTES:
 *   - I2C address: 0x29, speed: 400 kHz
 *   - Internal ESP32 pull-ups are enabled; most breakout boards
 *     already have external pull-ups on SDA/SCL.
 *   - A reading of ~8190 mm means out of range.
 * =====================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define SDA_PIN 21      // VL53L0X SDA -> GPIO21
#define SCL_PIN 22      // VL53L0X SCL -> GPIO22
#define ADDR    0x29    // VL53L0X default I2C address

static const char *TAG = "VL53L0X";
static i2c_master_dev_handle_t dev;
static uint8_t stop_var;

static void wr(uint8_t reg, uint8_t val)
{
    uint8_t d[2] = {reg, val};
    ESP_ERROR_CHECK(i2c_master_transmit(dev, d, 2, 100));
}

static uint8_t rd(uint8_t reg)
{
    uint8_t v;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev, &reg, 1, &v, 1, 100));
    return v;
}

static uint16_t rd16(uint8_t reg)
{
    uint8_t v[2];
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev, &reg, 1, v, 2, 100));
    return (v[0] << 8) | v[1];
}

/* Magic access sequence around the 0x91 "stop variable" register */
static void stop_var_access(bool write)
{
    wr(0x80, 1); wr(0xFF, 1); wr(0x00, 0);
    if (write) wr(0x91, stop_var); else stop_var = rd(0x91);
    wr(0x00, 1); wr(0xFF, 0); wr(0x80, 0);
}

static void sensor_init(void)
{
    wr(0x89, rd(0x89) | 0x01);      // 2.8V I/O mode
    wr(0x88, 0x00);
    stop_var_access(false);         // read stop variable
    wr(0x60, rd(0x60) | 0x12);      // disable signal rate limit checks
    wr(0x44, 0x00); wr(0x45, 0x20); // signal rate limit 0.25 MCPS
    wr(0x01, 0xFF);                 // sequence config
    wr(0x0A, 0x04);                 // interrupt on new sample ready
    wr(0x84, rd(0x84) & ~0x10);     // interrupt active low
    wr(0x0B, 0x01);                 // clear interrupt
}

static int read_mm(void)
{
    stop_var_access(true);
    wr(0x00, 0x01);                                  // start single shot
    for (int i = 0; (rd(0x00) & 0x01) && i < 100; i++) vTaskDelay(1);
    for (int i = 0; !(rd(0x13) & 0x07); i++) {
        if (i > 100) return -1;                      // timeout
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    int mm = rd16(0x1E);
    wr(0x0B, 0x01);                                  // clear interrupt
    return mm;
}

void app_main(void)
{
    i2c_master_bus_handle_t bus;
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &dev));

    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t id = rd(0xC0);
    ESP_LOGI(TAG, "Model ID: 0x%02X", id);
    if (id != 0xEE) {
        ESP_LOGE(TAG, "VL53L0X not found, check wiring (SDA=GPIO%d, SCL=GPIO%d)", SDA_PIN, SCL_PIN);
        return;
    }

    sensor_init();

    while (1) {
        int mm = read_mm();
        if (mm < 0) ESP_LOGE(TAG, "Timeout");
        else        ESP_LOGI(TAG, "Distance: %d mm (%d.%d cm)", mm, mm / 10, mm % 10);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}