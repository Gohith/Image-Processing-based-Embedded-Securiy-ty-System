/* Sample Code For Implementing PWM On Led */


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"


void app_main()
{
	ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, 
        .freq_hz = 2500,                      
        .speed_mode = LEDC_HIGH_SPEED_MODE,           
        .timer_num =   LEDC_TIMER_0          
	};
	ledc_timer_config(&ledc_timer);

	ledc_channel_config_t ledc_channel = 
        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = GPIO_NUM_4,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  =  LEDC_TIMER_0
	};
	ledc_channel_config(&ledc_channel);
	
	ledc_fade_func_install(0);

	while(1)
	{
		ledc_set_fade_with_time(ledc_channel.speed_mode, ledc_channel.channel, 4250 , 500);
		ledc_fade_start(ledc_channel.speed_mode, ledc_channel.channel,LEDC_FADE_WAIT_DONE);
		printf("Duty CYcle is 2000\n");

		// vTaskDelay(3000 / portTICK_PERIOD_MS);
		ledc_set_fade_with_time(ledc_channel.speed_mode, ledc_channel.channel, 2250 , 500);
		ledc_fade_start(ledc_channel.speed_mode, ledc_channel.channel,LEDC_FADE_WAIT_DONE);
		printf("Duty CYcle is 0\n");

		// vTaskDelay(3000 / portTICK_PERIOD_MS);
		ledc_set_fade_with_time(ledc_channel.speed_mode, ledc_channel.channel, 1000 , 500);
		ledc_fade_start(ledc_channel.speed_mode, ledc_channel.channel,LEDC_FADE_WAIT_DONE);
		printf("Duty CYcle is 8000\n");

		// vTaskDelay(3000 / portTICK_PERIOD_MS);
	}

}