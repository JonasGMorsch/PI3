/*@file   ESPNOW_receiver.c
  @brief  simple implementation of ESPNOW protocol (receiver end code)
  @author bheesma-10
*/

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

////////////// PIN DECLARATION //////////////
#define GPIO_OUTPUT_LED GPIO_NUM_22
////////////////////////////////////////////////////////////////////////////////////

//////////////////////////// ESP-NOW SETTINGS ////////////////////////////
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF ESP_IF_WIFI_AP

typedef struct
{
  uint8_t mac_addr[ESP_NOW_ETH_ALEN];
  const uint8_t *data;
  int data_len;
} espnow_event_recv_cb_t;

typedef struct struct_message
{
  uint32_t peak_hold_value;
  float adc_mediam;
  bool hit;
  uint32_t player_side;
  uint32_t adc_micros;
} espnow_data_t;

// Create a struct_message called myData
espnow_data_t espnow_data;

////////////////////////////////////////////////////////////////////////////////////

const char *TAG = "Placar";

static void wifi_init(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
  ESP_ERROR_CHECK(esp_wifi_start());
  // ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR)); //THIS BLOCKS WIFI DEVICES TO FIND IT
  // ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_LR)); //THIS BLOCKS WIFI DEVICES TO FIND IT
}

static void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  if (mac_addr == NULL || data == NULL || len <= 0)
  {
    ESP_LOGE(TAG, "Receive cb arg error");
    return;
  }

  memcpy(&espnow_data, data, len);

  // ESP_LOGI(TAG, "peak hold value: %.4d   adc_mediam: %.2f", espnow_data.peak_hold_value, espnow_data.adc_mediam);
  ESP_LOGI(TAG, "peak hold value: %.4d adc_mediam: %.2f adc_micros: %.3d return_tip_coupled_count: %.4d",
           espnow_data.peak_hold_value, espnow_data.adc_mediam, espnow_data.adc_micros, espnow_data.player_side);

  vTaskDelay(1);
}

void espnow_init(void)
{
  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_recv_cb(OnDataRecv));
  ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)"PMK1233443433245")); // set primary key
}

void app_main(void)
{
  // SETUP
  nvs_flash_init();
  wifi_init();
  espnow_init();

  gpio_set_direction(GPIO_OUTPUT_LED, GPIO_MODE_OUTPUT); // Set the GPIO as a push/pull output
  gpio_set_level(GPIO_OUTPUT_LED, 1);

  // MAIN
  for (;;)

  {

    static uint32_t hit_micros;
    if (espnow_data.hit)
    {
      gpio_set_level(GPIO_OUTPUT_LED, 0);
      hit_micros = esp_timer_get_time();
      espnow_data.hit = false;
    }

    if (espnow_data.hit == false && esp_timer_get_time() - hit_micros > 100000)
      gpio_set_level(GPIO_OUTPUT_LED, 1);

    vTaskDelay(1);
  }

}
