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
#include "ESPNOW.h"



/*led pin definition and pin mask*/
#define GPIO_OUTPUT_LED GPIO_NUM_22
#define GPIO_OUTPUT_PIN_SEL (1ULL << GPIO_OUTPUT_LED)

const char *TAG = "Receiver";

/*Queue handle*/
xQueueHandle espnow_queue;

void gpio_init(void)
{
  gpio_config_t pin_conf;

  /*configure output led pin*/
  pin_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  pin_conf.mode = GPIO_MODE_OUTPUT;
  pin_conf.pull_up_en = false;
  pin_conf.pull_down_en = false;
  pin_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&pin_conf);
}


void led_task(void *pvParameters)
{

  //espnow_event_t evt;

  //uint32_t delay = (uint32_t)pvParameters;

  ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_LED, 0));
  vTaskDelay( 10 / portTICK_PERIOD_MS);
  ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_LED, 1));

  //evt.id = ESPNOW_LED_TASK;

  /*xQueueReset(espnow_queue);
  if (xQueueSend(espnow_queue, &evt, 200) != pdTRUE)
  {
    ESP_LOGW(TAG, "send queue fail");
  }*/

  //vTaskDelay(delay / portTICK_PERIOD_MS);
  vTaskDelete(NULL);
}

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  espnow_event_t evt;
  espnow_event_recv_cb_t recv_cb;

  if (mac_addr == NULL || data == NULL || len <= 0)
  {
    ESP_LOGE(TAG, "Receive cb arg error");
    return;
  }

  memcpy(recv_cb.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
  recv_cb.data = data;
  recv_cb.data_len = len;

  evt.id = ESPNOW_RECV_CB;
  evt.info.recv_cb = recv_cb;

  if (xQueueSend(espnow_queue, &evt, 200) != pdTRUE)
  {
    ESP_LOGW(TAG, "Send receive queue fail");
  }
}

/**
 * @brief  ESPNOW main task
 * @param  task parameters
 * @retval None
 * @note   this task waits to receive data and then sends ack message back to the transmitter
 */
void esp_now_task(void *pvParameters)
{
  espnow_event_t evt;

  for (;;)
  {

    while (xQueueReceive(espnow_queue, &evt, 200) == pdTRUE)
    {

      switch (evt.id)
      {
      case ESPNOW_SEND_CB:
      {/*

        ESP_LOGI(TAG, "send task complete");
        ESP_LOGI(TAG, "data sent to device:");
        for (int mac_size = 0; mac_size < ESP_NOW_ETH_ALEN; mac_size++)
        {
          ESP_LOGI(TAG, "%02X", evt.info.send_cb.mac_addr[mac_size]);
        }*/
      }
      break;
      case ESPNOW_RECV_CB:
      {

        ESP_LOGI(TAG, "data received");
        ESP_LOGI(TAG, "received data length:%d", evt.info.recv_cb.data_len);
        /*for (int datalen = 0; datalen < evt.info.recv_cb.data_len; datalen++)
        {
          ESP_LOGI(TAG, "%02X", evt.info.recv_cb.data[datalen]);
        }*/
        ESP_LOGI(TAG, "\r\n");

        /****************************************************/
        xTaskCreate(led_task, "LED_TASK", 2500, (void *)500, 5, NULL);
        /****************************************************/
      }

      break;
      case ESPNOW_LED_TASK:
      {
        /* char *ack = "ok";
        memcpy(send_param.dest_mac, sender_MAC, sizeof(sender_MAC));
        send_param.buffer = (uint8_t *)ack;
        send_param.len = strlen(ack);

        if (esp_now_send(send_param.dest_mac, send_param.buffer, send_param.len) != ESP_OK)
        {
          ESP_LOGE(TAG, "Send error");
          espnow_deinit(&send_param);
          vTaskDelete(NULL);
        }*/
      }
      break;
      default:
        break;
      }
    }
  }
  vTaskDelete(NULL);
}

static void wifi_init(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
  ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));
#endif
}

void espnow_init(void)
{

  // esp_now_peer_num_t num_peers;
  espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
  if (espnow_queue == NULL)
  {
    ESP_LOGE(TAG, "Create mutex fail");
  }

  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

  /*set primary key*/
  ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)"PMK1233443433245"));

  /*create task*/
  xTaskCreate(esp_now_task, "ESP now Task", 5000, NULL, 2, NULL);
}


void app_main(void)
{
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  gpio_init();
  wifi_init();
  espnow_init();

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
