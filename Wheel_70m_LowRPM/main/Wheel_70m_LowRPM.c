/*
 * ============================================================
 * 70 METRE WHEEL - LOW RPM / LINEAR SPEED TEST
 * ============================================================
 *
 * WIRING - HALL EFFECT SENSOR
 *
 * Hall Sensor +  -> ESP32 3.3V
 * Hall Sensor G  -> ESP32 GND
 * Hall Sensor A0 -> ESP32 GPIO34
 * Hall Sensor D0 -> ESP32 GPIO27
 *
 *
 * WHEEL:
 *
 * Diameter        = 70 metres
 * Radius          = 35 metres
 * Magnets         = 2
 * Magnets/rev     = 2
 *
 *
 * OUTPUT:
 *
 * RPM
 * Linear velocity at the wheel rim in m/s
 *
 *
 * MEASUREMENT:
 *
 * We measure the TIME between consecutive magnet detections.
 *
 * This is specifically designed for VERY LOW RPM.
 *
 *
 * IMPORTANT:
 *
 * No floating-point calculations are performed inside the
 * GPIO interrupt.
 *
 * This prevents the crash that occurred in the previous
 * Wheel_RPM_Test program.
 *
 * ============================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"


/* ============================================================
 * HARDWARE
 * ============================================================
 */

#define HALL_D0_GPIO       GPIO_NUM_27
#define HALL_A0_GPIO       GPIO_NUM_34


/* ============================================================
 * WHEEL CONFIGURATION
 * ============================================================
 */

#define WHEEL_DIAMETER_M   70.0

#define MAGNETS_PER_REV    2


/* ============================================================
 * PHYSICAL CONSTANTS
 * ============================================================
 */

#define PI                  3.14159265358979323846

#define WHEEL_CIRCUMFERENCE_M \
    (PI * WHEEL_DIAMETER_M)


/* ============================================================
 * NOISE FILTER
 * ============================================================
 *
 * Any two pulses closer than this are considered noise.
 *
 * 10 ms corresponds to 6000 RPM with 1 magnet/rev,
 * so this is nowhere near our intended operating range.
 * ============================================================
 */

#define MIN_PULSE_INTERVAL_US   10000LL


/* ============================================================
 * SHARED DATA
 * ============================================================
 */

static volatile int64_t last_magnet_time_us = 0;

static volatile int64_t measured_interval_us = 0;

static volatile uint32_t magnet_count = 0;

static volatile bool new_measurement = false;


/* ============================================================
 * LOG TAG
 * ============================================================
 */

static const char *TAG = "70M_WHEEL";


/* ============================================================
 * HALL SENSOR INTERRUPT
 * ============================================================
 *
 * VERY IMPORTANT:
 *
 * Keep this function extremely small.
 *
 * DO NOT:
 *   - printf
 *   - ESP_LOG
 *   - floating point calculations
 *   - division
 *
 * ============================================================
 */

static void IRAM_ATTR hall_interrupt_handler(void *arg)
{
    int64_t now_us = esp_timer_get_time();


    /*
     * First magnet detection.
     *
     * We cannot calculate speed yet because there is
     * no previous magnet to compare against.
     */
    if (last_magnet_time_us == 0)
    {
        last_magnet_time_us = now_us;
        magnet_count++;

        return;
    }


    /*
     * Calculate time since the previous magnet.
     */
    int64_t interval_us =
        now_us - last_magnet_time_us;


    /*
     * Reject extremely fast pulses.
     *
     * These are most likely electrical noise.
     */
    if (interval_us < MIN_PULSE_INTERVAL_US)
    {
        return;
    }


    /*
     * Save the new pulse information.
     */
    last_magnet_time_us = now_us;

    measured_interval_us = interval_us;

    magnet_count++;

    new_measurement = true;
}


/* ============================================================
 * HALL SENSOR SETUP
 * ============================================================
 */

static void setup_hall_sensor(void)
{
    gpio_config_t hall_config = {
        .pin_bit_mask = (1ULL << HALL_D0_GPIO),

        .mode = GPIO_MODE_INPUT,

        .pull_up_en = GPIO_PULLUP_DISABLE,

        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        /*
         * Detect rising edge.
         */
        .intr_type = GPIO_INTR_POSEDGE
    };


    ESP_ERROR_CHECK(
        gpio_config(&hall_config)
    );


    /*
     * Install GPIO interrupt service.
     */
    ESP_ERROR_CHECK(
        gpio_install_isr_service(
            ESP_INTR_FLAG_IRAM
        )
    );


    /*
     * Attach interrupt handler.
     */
    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            HALL_D0_GPIO,
            hall_interrupt_handler,
            NULL
        )
    );
}


/* ============================================================
 * MAIN
 * ============================================================
 */

void app_main(void)
{
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "       70 METRE WHEEL LOW RPM TEST");
    ESP_LOGI(TAG, "==========================================");

    ESP_LOGI(TAG,
             "Wheel diameter : %.2f m",
             WHEEL_DIAMETER_M);

    ESP_LOGI(TAG,
             "Wheel radius   : %.2f m",
             WHEEL_DIAMETER_M / 2.0);

    ESP_LOGI(TAG,
             "Circumference  : %.4f m",
             WHEEL_CIRCUMFERENCE_M);

    ESP_LOGI(TAG,
             "Magnets        : %d",
             MAGNETS_PER_REV);

    ESP_LOGI(TAG,
             "Hall D0        : GPIO%d",
             HALL_D0_GPIO);

    ESP_LOGI(TAG,
             "Hall A0        : GPIO%d",
             HALL_A0_GPIO);

    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Measurement method:");
    ESP_LOGI(TAG, "Time between magnet pulses");
    ESP_LOGI(TAG, "==========================================");

    ESP_LOGI(TAG, "Waiting for wheel movement...");


    /*
     * Configure Hall sensor.
     */
    setup_hall_sensor();


    /*
     * Variables used by the main task.
     */
    int64_t local_interval_us = 0;

    uint32_t local_magnet_count = 0;

    double rpm = 0.0;

    double speed_mps = 0.0;


    /*
     * Used to determine when the wheel has stopped.
     */
    int64_t last_valid_measurement_time_us = 0;

    int64_t stop_timeout_us = 3000000LL;


    while (1)
    {
        int64_t now_us = esp_timer_get_time();


        /* ====================================================
         * CHECK FOR NEW MAGNET MEASUREMENT
         * ====================================================
         */

        if (new_measurement)
        {
            /*
             * Copy the ISR data.
             */
            local_interval_us = measured_interval_us;

            local_magnet_count = magnet_count;

            /*
             * Mark the measurement as processed.
             */
            new_measurement = false;


            /* =================================================
             * CALCULATE RPM
             *
             * interval = time between individual magnets
             *
             * With 2 magnets per revolution:
             *
             * RPM =
             *
             *        60,000,000
             * -------------------------
             * interval_us × magnets/rev
             * =================================================
             */

            if (local_interval_us > 0)
            {
                rpm =
                    60000000.0 /
                    (
                        (double)local_interval_us *
                        (double)MAGNETS_PER_REV
                    );


                /* =============================================
                 * CALCULATE LINEAR SPEED
                 *
                 * revolutions/second = RPM / 60
                 *
                 * speed = rev/sec × circumference
                 * =============================================
                 */

                speed_mps =
                    (rpm / 60.0) *
                    WHEEL_CIRCUMFERENCE_M;


                /*
                 * We just received a valid magnet pulse.
                 */
                last_valid_measurement_time_us = now_us;


                /*
                 * If the wheel is rotating slowly, we don't
                 * want to declare it stopped too quickly.
                 *
                 * Wait approximately 2 magnet intervals.
                 *
                 * This automatically adapts to the RPM.
                 */
                stop_timeout_us =
                    local_interval_us * 2;


                /*
                 * Never use a timeout smaller than 3 seconds.
                 */
                if (stop_timeout_us < 3000000LL)
                {
                    stop_timeout_us = 3000000LL;
                }


                /*
                 * Maximum timeout of 10 minutes.
                 *
                 * This allows extremely slow rotation without
                 * overflowing anything.
                 */
                if (stop_timeout_us > 600000000LL)
                {
                    stop_timeout_us = 600000000LL;
                }
            }
        }


        /* ====================================================
         * STOP DETECTION
         * ====================================================
         *
         * The timeout is based on the previously measured
         * magnet interval.
         *
         * Therefore:
         *
         * Fast wheel  -> fast return to zero
         *
         * Slow wheel  -> enough time allowed for next magnet
         *
         * This is much better for sub-50 RPM operation.
         * ====================================================
         */

        if (last_valid_measurement_time_us != 0)
        {
            if (
                (now_us - last_valid_measurement_time_us)
                > stop_timeout_us
            )
            {
                rpm = 0.0;

                speed_mps = 0.0;
            }
        }


        /* ====================================================
         * PRINT RESULTS
         * ====================================================
         */

        ESP_LOGI(
            TAG,
            "Magnets: %" PRIu32
            " | dt: %" PRId64 " us"
            " | RPM: %.4f"
            " | Speed: %.6f m/s",
            local_magnet_count,
            local_interval_us,
            rpm,
            speed_mps
        );


        /*
         * Update the display every 100 ms.
         *
         * IMPORTANT:
         *
         * The measurement itself does NOT depend on this
         * 100 ms interval.
         *
         * The actual measurement is performed whenever a
         * magnet passes the Hall sensor.
         */
        vTaskDelay(
            pdMS_TO_TICKS(100)
        );
    }
}