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


/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#define ESPNOW_QUEUE_SIZE           6

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    const uint8_t *data;
    int data_len;
}espnow_event_recv_cb_t;

/*led pin definition and pin mask*/
#define GPIO_OUTPUT_LED GPIO_NUM_22

const char *TAG = "Receiver";

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{

  espnow_event_recv_cb_t recv_cb; // Struct (replace for mine after)

  if (mac_addr == NULL || data == NULL || len <= 0)
  {
    ESP_LOGE(TAG, "Receive cb arg error");
    return;
  }

  // memcpy(recv_cb.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
  recv_cb.data = data;
  recv_cb.data_len = len;

  ESP_LOGI(TAG, "data received");
  ESP_LOGI(TAG, "received data length:%d", recv_cb.data_len);
  /*for (int datalen = 0; datalen < recv_cb.data_len; datalen++)
  {
    ESP_LOGI(TAG, "%02X", recv_cb.data[datalen]);
  }*/
  ESP_LOGI(TAG, "\r\n");

  gpio_set_level(GPIO_OUTPUT_LED, 0);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  gpio_set_level(GPIO_OUTPUT_LED, 1);
}

static void wifi_init(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
  ESP_ERROR_CHECK(esp_wifi_start());
  //ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR)); //THIS BLOCKS WIFI DEVICES TO FIND IT
  //ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_LR)); //THIS BLOCKS WIFI DEVICES TO FIND IT
}

void espnow_init(void)
{
  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
  ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)"PMK1233443433245")); // set primary key
}

void app_main(void)
{
  // SETUP
  nvs_flash_init();
  wifi_init();
  espnow_init();

  // gpio_reset_pin(GPIO_OUTPUT_LED);
  gpio_set_direction(GPIO_OUTPUT_LED, GPIO_MODE_OUTPUT); // Set the GPIO as a push/pull output

  // MAIN
  for (;;)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
