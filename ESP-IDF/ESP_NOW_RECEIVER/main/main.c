/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "esp_wifi.h"
#include "esp_now.h"
#include <string.h>

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
//#include "espnow_example.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

// The structure must match the receiver structure
typedef struct struct_message
{
    uint32_t adc_filtered_value;
    float adc_mediam;
} struct_message;

// Create a struct_message called myData
struct_message myData;

////////////////////////////////////////////////////////////////////////////////////////////////////

/*       printf("Hello world!\n");

   // Print chip information
   esp_chip_info_t chip_info;
   esp_chip_info(&chip_info);
   printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

   printf("silicon revision %d, ", chip_info.revision);

   printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

   printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
*/
////////////////////////////////////////////////////////////////////////////////////////////////////

// callback when data  is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{

    memcpy(&myData, data, data_len);

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    // printf("Last Packet Recv from: ");
    // printf(macStr);

    printf("adc_top:2000 \t adc_value:");
    printf("%d", myData.adc_filtered_value);
    printf("\t adc_mediam:");
    printf("%f", myData.adc_mediam);
    printf("\t adc_bottom:0 /n");
}

static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());
    // ESP_ERROR_CHECK( esp_wifi_set_protocol(WIFI_MODE_AP, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
}

static xQueueHandle s_example_espnow_queue;

typedef enum
{
    EXAMPLE_ESPNOW_SEND_CB,
    EXAMPLE_ESPNOW_RECV_CB,
} example_espnow_event_id_t;

typedef struct
{
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} example_espnow_event_send_cb_t;

typedef struct
{
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} example_espnow_event_recv_cb_t;

typedef union
{
    example_espnow_event_send_cb_t send_cb;
    example_espnow_event_recv_cb_t recv_cb;
} example_espnow_event_info_t;

/* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
typedef struct
{
    example_espnow_event_id_t id;
    example_espnow_event_info_t info;
} example_espnow_event_t;

/* Parameters of sending ESPNOW data. */
typedef struct
{
    bool unicast;                       // Send unicast ESPNOW data.
    bool broadcast;                     // Send broadcast ESPNOW data.
    uint8_t state;                      // Indicate that if has received broadcast ESPNOW data or not.
    uint32_t magic;                     // Magic number which is used to determine which device to send unicast ESPNOW data.
    uint16_t count;                     // Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                     // Delay between sending two ESPNOW data, unit: ms.
    int len;                            // Length of ESPNOW data to be sent, unit: byte.
    uint8_t *buffer;                    // Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN]; // MAC address of destination device.
} example_espnow_send_param_t;

#define ESPNOW_QUEUE_SIZE 6


static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0x78, 0xe3, 0x6d, 0x19, 0x52, 0x08 };


static esp_err_t example_espnow_init(void)
{
    example_espnow_send_param_t *send_param;

    s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    if (s_example_espnow_queue == NULL)
    {
        printf("Create mutex fail");
        return ESP_FAIL;
    }

    // Initialize ESPNOW and register sending and receiving callback function.
    ESP_ERROR_CHECK(esp_now_init());
    // ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK(esp_now_register_recv_cb(OnDataRecv));

    // Set primary master key.
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)"pmk1234567890123"));

    // Add broadcast peer information to peer list.
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        printf("Malloc peer information fail");
        vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 1;
    peer->ifidx = ESP_IF_WIFI_AP;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);



    return ESP_OK;
}

//  Initialize ESPNOW and register sending and receiving callback function.
// ESP_ERROR_CHECK(esp_now_init());
// ESP_ERROR_CHECK(esp_now_register_recv_cb(OnDataRecv));

void app_main(void)
{
    //  SETUP

    //example_wifi_init();
    //example_espnow_init();

    if (esp_now_init() != ESP_OK) // Init ESP Now with fallback
        esp_restart();
    esp_now_register_recv_cb(OnDataRecv);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    while (1)
    {
        for (int i = 10; i >= 0; i--)
        {
            printf("Restarting in %d seconds...\n", i);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        printf("Restarting now.\n");

        uint32_t counter = 10;
        counter++;
        printf("some variable: %d \n", counter);

        fflush(stdout);
        esp_restart();
    }
}
