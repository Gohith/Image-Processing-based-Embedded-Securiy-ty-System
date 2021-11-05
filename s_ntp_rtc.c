				/* SAMPLE CODE TO PERFORM LOCAL TIME RUN AFTER DISCONNECTING TO WIFI */


#include <string.h>

#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "wifi_functions.h"
#include "esp_sntp.h"
#include "esp_wifi.h"

time_t now;
struct tm timeinfo;

char strftime_buf[64];
	   

static const char *TAG = "RTC";

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    	sntp_init();
}



static void obtain_time(void)
{
	nvs_flash_init();
	tcpip_adapter_init();
	esp_event_loop_create_default();

	wifi_initialise();

 	wifi_wait_connected();
	initialize_sntp();
	time_t now = 0;
	struct tm timeinfo = { 0 };
	while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) 
	{
		ESP_LOGI(TAG, "Waiting for system time to be set...");
		vTaskDelay(2000 / portTICK_PERIOD_MS);
    	}
 	time(&now);
 	localtime_r(&now, &timeinfo);

    	esp_wifi_disconnect();
}



static void time_task(void *pvParameters)
{
	printf("NOW is %ld\n", now);
	for(;;)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		now++;
		printf("NOW is %ld\n", now);
		time(&now);
	    	localtime_r(&now, &timeinfo);
		strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	    	ESP_LOGI(TAG, "The current date/time in INDIA is: %s", strftime_buf);
	}
}



void app_main()
{
	    
	    time(&now);
	    localtime_r(&now, &timeinfo);

	    if (timeinfo.tm_year < (2016 - 1900)) 
	    {
		ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
		obtain_time();
		time(&now);
	    }
	    setenv("TZ", "IST-5:30", 1);
	    tzset();
	    localtime_r(&now, &timeinfo);
	    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	    ESP_LOGI(TAG, "The current date/time in INDIA is: %s", strftime_buf);

	    xTaskCreate(time_task, "time_task", 2048, NULL, 5, NULL);
	    
	  /*  const int deep_sleep_sec = 15;
	    ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
	    esp_deep_sleep(1000000LL * deep_sleep_sec);*/
}
