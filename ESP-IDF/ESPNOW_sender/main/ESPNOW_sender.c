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
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#define ESPNOW_QUEUE_SIZE           6
#define IS_BROADCAST_ADDR(addr) (memcmp(addr, esp_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

/* Parameters of sending ESPNOW data. */
typedef struct {
    bool unicast;                         //Send unicast ESPNOW data.
    bool broadcast;                       //Send broadcast ESPNOW data.
    uint16_t count;                       //Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
    int len;                              //Length of ESPNOW data to be sent, unit: byte.
    uint8_t *buffer;                      //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
}espnow_send_param_t;


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
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10

};

/*led pin definition and pin mask*/
#define GPIO_OUTPUT_LED GPIO_NUM_22
#define GPIO_OUTPUT_PIN_SEL (1ULL << GPIO_OUTPUT_LED)

const char *TAG = "Sender";

volatile uint8_t received_mac;

/*Queue handle*/
// xQueueHandle espnow_queue;

/*Timer handle*/
// TimerHandle_t espnow_timer;

/**
 * @brief  initialize gpio pin
 * @param  None
 * @retval None
 */
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

    // espnow_event_t evt;

    // uint32_t delay = (uint32_t)pvParameters;

    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_LED, 0));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_LED, 1));

    // evt.id = ESPNOW_LED_TASK;

    /*xQueueReset(espnow_queue);
    if (xQueueSend(espnow_queue, &evt, 200) != pdTRUE)
    {
      ESP_LOGW(TAG, "send queue fail");
    }*/

    // vTaskDelay(delay / portTICK_PERIOD_MS);
    vTaskDelete(NULL);
}

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
        esp_now_send(send_param->dest_mac, (uint8_t *)sending_data, send_param->len); // send data
        vTaskDelay(500 / portTICK_PERIOD_MS);
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

    //#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));
    //#endif
}

void espnow_init(void)
{

    esp_now_peer_num_t num_peers;
    espnow_send_param_t *send_param;

    /*create queue for application*/
    /*espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
    if (espnow_queue == NULL)
    {
        ESP_LOGE(TAG, "Create mutex fail");
    }*/

    /*create timer for application(recursive timer)*/
    // espnow_timer = xTimerCreate("ESPNOW timer", pdMS_TO_TICKS(500), pdTRUE, (void *)0, vTimerCallback);

    ESP_ERROR_CHECK(esp_now_init());
    // ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    //  ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

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

    /*get number of peers and peer data from stored list*/
    ESP_ERROR_CHECK(esp_now_get_peer_num(&num_peers));
    ESP_LOGI(TAG, "no of peers in peers list:%d", num_peers.total_num);
    peer = (esp_now_peer_info_t *)malloc(sizeof(esp_now_peer_info_t));
    for (int num_peer = 0; num_peer < num_peers.total_num; num_peer++)
    {
        esp_now_get_peer(receiver_MAC, peer);
        ESP_LOGI(TAG, "channel:%d", peer->channel);
        ESP_LOGI(TAG, "peer address");
        for (int address_size = 0; address_size < ESP_NOW_ETH_ALEN; address_size++)
        {
            ESP_LOGI(TAG, "%02X\r", peer->peer_addr[address_size]);
        }
    }
    free(peer);

    /* Initialize sending parameters. */
    send_param = malloc(sizeof(espnow_send_param_t));
    memset(send_param, 0, sizeof(espnow_send_param_t));
    if (send_param == NULL)
    {
        ESP_LOGE(TAG, "Malloc send parameter fail");
    }
    send_param->unicast = true;
    send_param->count = sizeof(sending_data); // CONFIG_ESPNOW_SEND_COUNT;
    send_param->delay = 1000;
    send_param->len = sizeof(sending_data);
    send_param->buffer = &sending_data[0];
    ESP_LOGI(TAG, "before task buffer");
    for (int size = 0; size < send_param->len; size++)
    {
        ESP_LOGI(TAG, "%02X", send_param->buffer[size]);
    }
    memcpy(send_param->dest_mac, receiver_MAC, ESP_NOW_ETH_ALEN);
    ESP_LOGI(TAG, "mac address:%02X:%02X:%02X:%02X:%02X:%02X", send_param->dest_mac[0], send_param->dest_mac[1], send_param->dest_mac[2], send_param->dest_mac[3], send_param->dest_mac[4], send_param->dest_mac[5]);

    /*create task*/
    xTaskCreate(esp_now_task, "ESP now Task", 5000, (void *)send_param, 2, NULL);
}

void app_main(void)
{
    // SETUP
    nvs_flash_init();
    wifi_init();
    espnow_init();

    // MAIN
    for (;;)
    {
        // esp_now_send(send_param->dest_mac, (uint8_t *)sending_data, send_param->len); // send data
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
