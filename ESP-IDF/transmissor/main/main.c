// Jonas G. Morsch
// Projeto Integrador 3
// Sistema de detecção de metais por capacitância
// sem fio para esgrima

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "hal/adc_hal.h"
#include "esp_log.h"

// Need to be selected for every transmitter 
#define PLAYER 1
// #define PLAYER 2

////////////// PIN DECLARATION //////////////
#define GPIO_OUTPUT_LED GPIO_NUM_22
#define GPIO_CAP_TIP GPIO_NUM_32
#define GPIO_RETUN_TIP GPIO_NUM_34

//////////////////////////// ESP-NOW SETTINGS ////////////////////////////
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF ESP_IF_WIFI_STA

typedef struct
{
    bool unicast;                       // Send unicast ESPNOW data.
    bool broadcast;                     // Send broadcast ESPNOW data.
    uint16_t count;                     // Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                     // Delay between sending two ESPNOW data, unit: ms.
    int len;                            // Length of ESPNOW data to be sent, unit: byte.
    uint8_t *buffer;                    // Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN]; // MAC address of destination device.
} espnow_send_param_t;

typedef struct struct_message
{
    volatile uint32_t peak_hold_value;
    float adc_mediam;
    volatile bool hit;
    uint32_t player_side;
    volatile uint32_t adc_micros;
} espnow_data_t;

// Create a struct_message
volatile espnow_data_t espnow_data;

//////////////////////////// TRANSMISSOR SETTINGS ////////////////////////////
// receiver ESP32 mac addresses (change these as per your device)
uint8_t receiver_MAC[] = {0x78, 0xe3, 0x6d, 0x1a, 0x89, 0x48};
///////////////////////////////////////////////////////////////////////////////

volatile uint32_t return_tip_coupled_count;
const char *TAG = "TRANSMITTER";

void IRAM_ATTR ADCTimer() // void *arg)
{

    if (gpio_get_level(GPIO_RETUN_TIP)) // needs to be before adc_task, it takes ~100uS to reach HIGH state.
        return_tip_coupled_count++;
    else
        return_tip_coupled_count = 0;

    //////////////// ACD VALUE ////////////////
    espnow_data.adc_micros = esp_timer_get_time();
    gpio_set_direction(GPIO_CAP_TIP, GPIO_MODE_INPUT);
    uint32_t adc_value = adc1_get_raw(ADC1_CHANNEL_4);
    gpio_set_direction(GPIO_CAP_TIP, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_CAP_TIP, 1);
    espnow_data.adc_micros = esp_timer_get_time() - espnow_data.adc_micros;

    if (espnow_data.adc_micros > 45) // if adc mesurement was delayed by system kernel, discart it, mutex won't help in this case
    {
        adc_value = 0;
        espnow_data.adc_micros = 0;
        return_tip_coupled_count--;
        return;
    }

    const register double lowpass_coefficient = 0.05; // About 10 seconds to stabilize
    switch (return_tip_coupled_count)
    {
    case 0:
        espnow_data.peak_hold_value = adc_value; // Debug Info

        if (adc_value > espnow_data.adc_mediam)
            espnow_data.adc_mediam += lowpass_coefficient;
        else
            espnow_data.adc_mediam -= lowpass_coefficient;
        break;

    case 1 ... 2:
        espnow_data.peak_hold_value = 0; // Discart the 3 initial measuriments, they are prone to static interference
        break;

    case 3 ... 9:
        if (adc_value > espnow_data.peak_hold_value)
            espnow_data.peak_hold_value = adc_value;
        break;

    case 10:
        if (espnow_data.peak_hold_value < espnow_data.adc_mediam)
            espnow_data.hit = true;
        break;

    default:
        break;
    }
}

void app_main(void)
{
    // SETUP
    nvs_flash_init();

    ////////////// WIFI SETUP //////////////
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_LR)); // THIS BLOCKS WIFI DEVICES TO FIND IT

    ////////////// ESPNOW SETUP //////////////
    ESP_ERROR_CHECK(esp_now_init());
    // add receiver address to peer list
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 1;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, receiver_MAC, sizeof(receiver_MAC));
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    espnow_data.adc_mediam = 1000;
    espnow_data.player_side = PLAYER;

    ////////////// PIN SETUP //////////////
    gpio_set_direction(GPIO_OUTPUT_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_OUTPUT_LED, 1);

    adc_hal_set_atten(ADC_NUM_1, ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    adc_set_data_inv(ADC_UNIT_1, 1);

    gpio_set_direction(GPIO_CAP_TIP, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_CAP_TIP, 1);
    gpio_set_direction(GPIO_RETUN_TIP, GPIO_MODE_INPUT);

    ////////////// TIMER SETUP //////////////
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &ADCTimer,
        .name = "ADCTimer", // name is optional, but may help identify the timer when debugging
        .dispatch_method = ESP_TIMER_TASK,
        .arg = NULL,
        .skip_unhandled_events = true};
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, 200); // Start the timers

    while (1) // MAIN
    {
        static uint32_t hit_micros;
        if (espnow_data.hit)
        {
            esp_now_send(receiver_MAC, (uint8_t *)&espnow_data, sizeof(espnow_data));

            gpio_set_level(GPIO_OUTPUT_LED, 0);

            espnow_data.hit = false;
            hit_micros = esp_timer_get_time();
        }

        if (espnow_data.hit == false && esp_timer_get_time() - hit_micros > 100000)
        {
            gpio_set_level(GPIO_OUTPUT_LED, 1);
        }

        // Debug Data
        // ESP_LOGI(TAG, "peak hold value: %.4d \t adc_mediam: %.2f \t adc_micros: %.3d",
        //          espnow_data.peak_hold_value, espnow_data.adc_mediam, espnow_data.adc_micros);
        //  ESP_LOGI(TAG, "return_tip_coupled_count: %d", return_tip_coupled_count);
        // ESP_LOGI(TAG, "adc_micros: %d", espnow_data.adc_micros);

        vTaskDelay(1); // 1 tick = 100Hz
    }
}