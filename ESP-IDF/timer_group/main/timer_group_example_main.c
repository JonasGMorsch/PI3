#include <stdio.h>
#include "soc/soc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/timer.h"
#include "esp_log.h"

// void IRAM_ATTR ADCTimer(void)
bool IRAM_ATTR ADCTimer(void *args)
{
    printf("I");
    return 0;
}

typedef struct
{
    QueueHandle_t user_queue;
    int timer_group;
    int timer_idx;
    int alarm_value;
    bool auto_reload;
} example_timer_user_data_t;

typedef struct {
    uint64_t timer_count_value;
} example_timer_event_t;


void app_main(void)
{

    example_timer_user_data_t *user_data = calloc(1, sizeof(example_timer_user_data_t));
    assert(user_data);
    user_data->user_queue = xQueueCreate(10, sizeof(example_timer_event_t));
    assert(user_data->user_queue);
    user_data->timer_group = 0;
    user_data->timer_idx = 0;
    user_data->alarm_value = 20000000;
    user_data->auto_reload = true;

    timer_config_t config = {
        .divider = 2,
        .counter_dir = true,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = true,
    };

    timer_init(0, 0, &config);
    timer_set_counter_value(0, 0, 0);
    timer_isr_callback_add(0, 0, ADCTimer, user_data, 0);
    timer_set_alarm_value(0, 0, 20000000);
    // timerSetAutoReload(0, true);
    timer_set_alarm(0, 0, true);

    //timerStart(0, 0);

    while (1)
    {
        vTaskDelay(1);
    }
}