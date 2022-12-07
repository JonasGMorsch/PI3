
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

#define GPIO_RETUN_TIP GPIO_NUM_34


void app_main(void)
{
    gpio_set_direction(GPIO_RETUN_TIP, GPIO_MODE_INPUT);  

    int cnt = 0;
    while(1) {
        printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_RATE_MS);
        printf("GPIO[%d] intr, val: %d\n", GPIO_RETUN_TIP, gpio_get_level(GPIO_RETUN_TIP));
    }
}
