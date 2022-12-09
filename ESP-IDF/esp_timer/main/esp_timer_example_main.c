/* esp_timer (high resolution timer) example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "driver/adc.h"
#include "hal/adc_hal.h"
#include "esp_adc_cal.h"

#include "driver/gpio.h"

////////////// PIN DECLARATION //////////////
#define GPIO_OUTPUT_LED GPIO_NUM_22
#define GPIO_CAP_TIP GPIO_NUM_32
#define GPIO_RETUN_TIP GPIO_NUM_34

static esp_adc_cal_characteristics_t adc1_chars;

static const char *TAG = "TIMER";

volatile uint32_t return_tip_coupled_count;

volatile uint32_t guard_touched_micros;
volatile double adc_mediam = 1000;
volatile uint32_t peak_hold_value;

volatile bool hit = false;
volatile bool print = false;

void ADCTimer(void *arg)
{
    // int64_t time_since_boot = esp_timer_get_time();
    // ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);
    // gpio_set_direction(GPIO_RETUN_TIP, GPIO_MODE_INPUT);
    if (gpio_get_level(GPIO_RETUN_TIP)) // needs to be before adc_task, it takes ~100uS to reach HIGH state.
        return_tip_coupled_count++;
    else
        return_tip_coupled_count = 0;

    //////////////// acd_value ////////////////
    gpio_set_direction(GPIO_CAP_TIP, GPIO_MODE_INPUT);
    uint32_t adc_value = adc1_get_raw(ADC1_CHANNEL_4);
    // gpio_reset_pin(GPIO_CAP_TIP);

    gpio_set_direction(GPIO_CAP_TIP, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_CAP_TIP, 1);

    const register double lowpass_coefficient = 0.05;
    switch (return_tip_coupled_count)
    {
    case 0:
        peak_hold_value = adc_value;

        if (adc_value > adc_mediam)
            adc_mediam += lowpass_coefficient;
        else
            adc_mediam -= lowpass_coefficient;
        break;

    case 1 ... 2:
        peak_hold_value = 0;
        break;

    case 3 ... 9:
        if (adc_value > peak_hold_value)
            peak_hold_value = adc_value;
        break;

    default:
        if (peak_hold_value > adc_mediam)
            guard_touched_micros = esp_timer_get_time();

        if (esp_timer_get_time() - guard_touched_micros > 40000)
            hit = true;

        break;
    }

    print = true;
}

void app_main(void)
{
    // SETUP

    ////////////// PIN SETUP //////////////
    gpio_set_direction(GPIO_OUTPUT_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_OUTPUT_LED, 1);

    //esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chars);
    //adc1_config_width(ADC_WIDTH_BIT_12);

    //adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11); // This commando breaks GPIO output mode

    adc_hal_set_atten(ADC_NUM_1, ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    adc_set_data_inv(ADC_UNIT_1, 1);

    gpio_set_direction(GPIO_CAP_TIP, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_CAP_TIP, 1);

    // gpio_set_drive_capability(GPIO_CAP_TIP,0);

    gpio_set_direction(GPIO_RETUN_TIP, GPIO_MODE_INPUT);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &ADCTimer,
        .name = "ADCTimer"}; // name is optional, but may help identify the timer when debugging
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, 200); // Start the timers

    ESP_LOGI(TAG, "ENTEERING MAIN");
    // MAIN
    for (;;)
    {

        static uint32_t hit_micros;
        if (hit)
        {
            gpio_set_level(GPIO_OUTPUT_LED, 0);
            hit_micros = esp_timer_get_time();
            hit = false;
        }

        if (hit == 0 && esp_timer_get_time() - hit_micros > 100000)
        {
            gpio_set_level(GPIO_OUTPUT_LED, 1);
        }

        static uint32_t test_timer_micros;
        if (esp_timer_get_time() - test_timer_micros > 1000000)
        {
            test_timer_micros = esp_timer_get_time();
            ESP_LOGI(TAG, "peak hold value: %d \t adc_mediam: %f", peak_hold_value, adc_mediam);
            ESP_LOGI(TAG, "return_tip_coupled_count: %d", return_tip_coupled_count);
            // ESP_LOGI(TAG, "GPIO_RETUN_TIP: %d", gpio_get_level(GPIO_RETUN_TIP));
        }

        vTaskDelay(1);
    }
}
