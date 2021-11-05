#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"




#define BLINK_GPIO GPIO_NUM_23 

void app_main()
{
    
    gpio_pad_select_gpio(BLINK_GPIO);
    
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
  
    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
   
}