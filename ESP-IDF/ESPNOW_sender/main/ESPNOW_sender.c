/*@file   ESPNOW_sender.c
  @brief  implementation of ESPNOW protocol (sender end code)
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

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF ESP_IF_WIFI_STA
#define ESPNOW_QUEUE_SIZE 6

/* Parameters of sending ESPNOW data. */
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

/*receiver and sender ESP32 mac addresses(change these as per your device)*/
uint8_t receiver_MAC[] = {0x78, 0xe3, 0x6d, 0x19, 0x40, 0x61};
// uint8_t sender_MAC[]   = {0x78,0xe3,0x6d,0x19,0x52,0x09};

/*dummy data to be sent*/
uint8_t sending_data[] = {

    0xEF, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x10

};

/*led pin definition and pin mask*/
#define GPIO_OUTPUT_LED GPIO_NUM_22

const char *TAG = "Sender";

volatile uint8_t received_mac;

void esp_now_task(void *pvParameters) // ESPNOW main task
{

    // send_cb evt;
    espnow_send_param_t *send_param = (espnow_send_param_t *)pvParameters;

    ESP_LOGI(TAG, "mac address in task:%02X:%02X:%02X:%02X:%02X:%02X", send_param->dest_mac[0], send_param->dest_mac[1], send_param->dest_mac[2], send_param->dest_mac[3], send_param->dest_mac[4], send_param->dest_mac[5]);
    ESP_LOGI(TAG, "buffer length:%d", send_param->len);
    for (int size = 0; size < send_param->len; size++)
    {
        ESP_LOGI(TAG, "%02X", send_param->buffer[size]);
    }

    for (;;)
    {
        // esp_now_send(send_param->dest_mac, (uint8_t *)sending_data, send_param->len); // send data
        vTaskDelay(100 / portTICK_PERIOD_MS);
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
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));
}

void espnow_init(void)
{

    esp_now_peer_num_t num_peers;
    espnow_send_param_t *send_param;

    ESP_ERROR_CHECK(esp_now_init());

    /*set primary key*/
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)"PMK1233443433245"));

    /*add receiver address to peer list*/
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 1;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, receiver_MAC, sizeof(receiver_MAC));
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    ESP_LOGI(TAG, "mac address:%02X:%02X:%02X:%02X:%02X:%02X", receiver_MAC[0], receiver_MAC[1], receiver_MAC[2], receiver_MAC[3], receiver_MAC[4], receiver_MAC[5]);
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

        ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_LED, 0));
        vTaskDelay(10 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_LED, 1));

        esp_now_send(receiver_MAC, (uint8_t *)sending_data, sizeof(sending_data)); // send data
        
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
