/*
 * ============================================================
 *              4 MAGNET WHEEL RPM TEST - ESP32
 * ============================================================
 *
 * SENSOR:
 *     HW-484 V0.3 Linear Hall Effect Sensor
 *
 *
 * WIRING:
 *
 *     HW-484 +  -> ESP32 3.3V
 *     HW-484 G  -> ESP32 GND
 *     HW-484 A0 -> ESP32 GPIO34
 *     HW-484 D0 -> ESP32 GPIO27
 *
 *
 * WHEEL:
 *
 *     Diameter = 130 mm
 *     Magnets  = 4
 *
 *     Magnets should be approximately
 *     90 degrees apart.
 *
 *
 * RPM CALCULATION:
 *
 *     4 magnets = 1 revolution
 *
 *     Therefore, the time between two magnet
 *     detections represents 1/4 revolution.
 *
 *
 *     RPM = 60,000,000
 *           -------------------------
 *           time_between_magnets × 4
 *
 *
 *     RPM = 15,000,000 / time_between_magnets_us
 *
 *
 * FEATURES:
 *
 *     - D0 rising-edge interrupt
 *     - One count per HIGH event
 *     - Waits for D0 to return LOW
 *     - Noise filtering
 *     - Moving average
 *     - Automatic stop detection
 *     - Integer RPM
 *     - Speed to 4 decimal places
 *     - Speed in km/h
 *     - No floating point in ISR
 *
 * ============================================================
 */


#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_log.h"
#include "esp_timer.h"


#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"



/* ============================================================
 * PIN CONFIGURATION
 * ============================================================ */

#define HALL_A0_GPIO        GPIO_NUM_34
#define HALL_D0_GPIO        GPIO_NUM_27

#define HALL_ADC_UNIT       ADC_UNIT_1
#define HALL_ADC_CHANNEL    ADC_CHANNEL_6



/* ============================================================
 * WHEEL CONFIGURATION
 * ============================================================ */

#define MAGNETS_PER_REV     4


/*
 * Wheel diameter:
 *
 *     130 mm
 *
 *
 * Circumference:
 *
 *     PI × 130
 *     = 408.407 mm
 *
 *
 * Stored as micrometers.
 */

#define WHEEL_CIRCUMFERENCE_UM    408407



/* ============================================================
 * FILTER CONFIGURATION
 * ============================================================ */


/*
 * Minimum time between magnet detections.
 *
 *
 * For 4 magnets:
 *
 *
 * RPM = 15,000,000 / interval_us
 *
 *
 * 8000 us gives:
 *
 *     15,000,000 / 8,000
 *     = 1875 RPM
 *
 *
 * Our maximum accepted RPM is 1800.
 */

#define MIN_PULSE_US        8000


/*
 * Maximum accepted RPM.
 */

#define MAX_RPM             1800


/*
 * If no valid magnet is detected for
 * 500 ms, assume the wheel has stopped.
 */

#define STOP_TIMEOUT_US     500000


/*
 * Number of RPM readings used
 * for the moving average.
 */

#define RPM_AVERAGE_SIZE    5



/* ============================================================
 * LOG TAG
 * ============================================================ */

static const char *TAG = "4_MAGNET_RPM";



/* ============================================================
 * ISR VARIABLES
 * ============================================================ */


/*
 * Time at which the last accepted magnet
 * was detected.
 */

static volatile int64_t last_pulse_time_us = 0;


/*
 * Time between the last two accepted magnets.
 */

static volatile int64_t pulse_interval_us = 0;


/*
 * Total number of accepted magnet detections.
 */

static volatile uint32_t magnet_count = 0;


/*
 * D0 lock.
 *
 * 0 = ready for a magnet
 * 1 = magnet detected, waiting for D0 LOW
 */

static volatile bool waiting_for_low = false;



/* ============================================================
 * HALL SENSOR INTERRUPT
 * ============================================================ */


/*
 * The ISR only:
 *
 *     - Gets timestamp
 *     - Checks debounce
 *     - Records pulse
 *
 *
 * It does NOT:
 *
 *     - Calculate RPM
 *     - Calculate speed
 *     - Print anything
 */

static void IRAM_ATTR hall_isr(void *arg)
{
    /*
     * If the previous magnet is still holding
     * D0 HIGH, don't count another event.
     */

    if (waiting_for_low)
    {
        return;
    }


    /*
     * Get current timestamp.
     */

    int64_t now_us =
        esp_timer_get_time();


    /*
     * First magnet after startup.
     */

    if (last_pulse_time_us == 0)
    {
        last_pulse_time_us =
            now_us;


        magnet_count = 1;


        waiting_for_low = true;


        return;
    }


    /*
     * Time since previous accepted magnet.
     */

    int64_t delta_us =
        now_us -
        last_pulse_time_us;


    /*
     * Reject pulses that occur too quickly.
     *
     * These are likely noise/chatter.
     */

    if (delta_us < MIN_PULSE_US)
    {
        return;
    }


    /*
     * Valid magnet.
     */

    pulse_interval_us =
        delta_us;


    last_pulse_time_us =
        now_us;


    magnet_count++;


    waiting_for_low = true;
}



/* ============================================================
 * MAIN
 * ============================================================ */

void app_main(void)
{
    ESP_LOGI(
        TAG,
        "========================================"
    );


    ESP_LOGI(
        TAG,
        "       4 MAGNET WHEEL RPM TEST"
    );


    ESP_LOGI(
        TAG,
        "========================================"
    );


    ESP_LOGI(
        TAG,
        "Hall A0       : GPIO34"
    );


    ESP_LOGI(
        TAG,
        "Hall D0       : GPIO27"
    );


    ESP_LOGI(
        TAG,
        "Wheel         : 130 mm"
    );


    ESP_LOGI(
        TAG,
        "Magnets       : 4"
    );


    ESP_LOGI(
        TAG,
        "Min pulse     : %d us",
        MIN_PULSE_US
    );


    ESP_LOGI(
        TAG,
        "Max RPM       : %d",
        MAX_RPM
    );


    ESP_LOGI(
        TAG,
        "========================================"
    );



    /* ========================================================
     * ADC SETUP
     * ======================================================== */

    adc_oneshot_unit_handle_t adc_handle;


    adc_oneshot_unit_init_cfg_t adc_config =
    {
        .unit_id = HALL_ADC_UNIT
    };


    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &adc_config,
            &adc_handle
        )
    );


    adc_oneshot_chan_cfg_t adc_channel_config =
    {
        .bitwidth =
            ADC_BITWIDTH_DEFAULT,

        .atten =
            ADC_ATTEN_DB_12
    };


    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            HALL_ADC_CHANNEL,
            &adc_channel_config
        )
    );



    /* ========================================================
     * D0 GPIO SETUP
     * ======================================================== */

    gpio_config_t d0_config =
    {
        .pin_bit_mask =
            (1ULL << HALL_D0_GPIO),

        .mode =
            GPIO_MODE_INPUT,

        .pull_up_en =
            GPIO_PULLUP_DISABLE,

        .pull_down_en =
            GPIO_PULLDOWN_DISABLE,

        /*
         * Detect LOW -> HIGH
         */

        .intr_type =
            GPIO_INTR_POSEDGE
    };


    ESP_ERROR_CHECK(
        gpio_config(
            &d0_config
        )
    );



    /* ========================================================
     * INTERRUPT SETUP
     * ======================================================== */

    ESP_ERROR_CHECK(
        gpio_install_isr_service(0)
    );


    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            HALL_D0_GPIO,
            hall_isr,
            NULL
        )
    );


    ESP_LOGI(
        TAG,
        "Hall sensor ready."
    );


    ESP_LOGI(
        TAG,
        "Waiting for wheel..."
    );



    /* ========================================================
     * RPM HISTORY
     * ======================================================== */

    int rpm_history[RPM_AVERAGE_SIZE] =
        {0};


    int rpm_history_index = 0;


    int rpm_history_count = 0;


    /*
     * Last pulse interval processed
     * by the main task.
     */

    int64_t processed_interval_us = 0;


    /*
     * Current displayed RPM.
     */

    int displayed_rpm = 0;



    /* ========================================================
     * MAIN LOOP
     * ======================================================== */

    while (1)
    {

        /* ----------------------------------------------------
         * READ A0
         * ---------------------------------------------------- */

        int adc_raw = 0;


        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                HALL_ADC_CHANNEL,
                &adc_raw
            )
        );


        /*
         * Previous approximate zero point.
         */

        int a0 =
            adc_raw - 1900;



        /* ----------------------------------------------------
         * READ D0
         * ---------------------------------------------------- */

        int d0 =
            gpio_get_level(
                HALL_D0_GPIO
            );


        /*
         * Once D0 has returned LOW,
         * unlock the sensor.
         */

        if (d0 == 0)
        {
            waiting_for_low = false;
        }



        /* ----------------------------------------------------
         * COPY TIMING INFORMATION
         * ---------------------------------------------------- */

        int64_t interval_us =
            pulse_interval_us;


        int64_t last_pulse =
            last_pulse_time_us;


        int64_t now_us =
            esp_timer_get_time();



        /* ----------------------------------------------------
         * PROCESS NEW MAGNET EVENT
         * ---------------------------------------------------- */

        if (
            interval_us != 0 &&
            interval_us !=
            processed_interval_us
        )
        {

            processed_interval_us =
                interval_us;


            /*
             * FOUR MAGNETS PER REVOLUTION
             *
             *
             * RPM =
             *
             *     60,000,000
             *     -----------------
             *     interval × 4
             *
             *
             * Therefore:
             *
             *     RPM =
             *
             *     15,000,000
             *     -----------------
             *          interval
             */


            int instant_rpm =
                (int)(
                    15000000LL /
                    interval_us
                );



            /*
             * Accept only realistic RPM.
             */

            if (
                instant_rpm > 0 &&
                instant_rpm <= MAX_RPM
            )
            {

                /*
                 * Add to moving average.
                 */

                rpm_history[
                    rpm_history_index
                ] =
                    instant_rpm;


                rpm_history_index++;


                if (
                    rpm_history_index >=
                    RPM_AVERAGE_SIZE
                )
                {
                    rpm_history_index = 0;
                }


                if (
                    rpm_history_count <
                    RPM_AVERAGE_SIZE
                )
                {
                    rpm_history_count++;
                }
            }
        }



        /* ----------------------------------------------------
         * STOP DETECTION
         * ---------------------------------------------------- */

        if (
            last_pulse == 0 ||
            (
                now_us -
                last_pulse
            ) >
            STOP_TIMEOUT_US
        )
        {

            /*
             * Wheel has stopped.
             */

            displayed_rpm = 0;


            /*
             * Clear old RPM history.
             */

            rpm_history_count = 0;


            rpm_history_index = 0;


            processed_interval_us =
                pulse_interval_us;
        }



        /* ----------------------------------------------------
         * CALCULATE AVERAGE RPM
         * ---------------------------------------------------- */

        else if (
            rpm_history_count > 0
        )
        {

            int64_t sum = 0;


            for (
                int i = 0;
                i < rpm_history_count;
                i++
            )
            {
                sum +=
                    rpm_history[i];
            }


            displayed_rpm =
                (int)(
                    sum /
                    rpm_history_count
                );
        }



        /* ----------------------------------------------------
         * SAFETY LIMIT
         * ---------------------------------------------------- */

        if (
            displayed_rpm >
            MAX_RPM
        )
        {
            displayed_rpm = 0;
        }



        /* ----------------------------------------------------
         * CALCULATE SPEED
         * ---------------------------------------------------- */

        /*
         * Wheel diameter:
         *
         *     130 mm
         *
         *
         * Wheel circumference:
         *
         *     PI × 0.130
         *
         *     = 0.408407 m
         *
         *
         * Speed:
         *
         *     RPM × circumference × 60
         *     -------------------------
         *              1000
         *
         *
         * Result:
         *
         *     km/h
         *
         *
         * double is used here so that
         * four decimal places are preserved.
         */

        double speed_kmh =
            (
                (double)displayed_rpm *
                0.408407 *
                60.0
            )
            /
            1000.0;



        /* ----------------------------------------------------
         * OUTPUT
         * ---------------------------------------------------- */

        ESP_LOGI(
            TAG,

            "A0: %+d | D0: %d | Magnets: %" PRIu32
            " | RPM: %d | Speed: %.4f km/h",

            a0,

            d0,

            magnet_count,

            displayed_rpm,

            speed_kmh
        );



        /* ----------------------------------------------------
         * LOOP DELAY
         * ---------------------------------------------------- */

        vTaskDelay(
            pdMS_TO_TICKS(100)
        );
    }
}