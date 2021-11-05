

#include "stdio.h"
#include "string.h"
#include "stdint.h"
#include "e2000a.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "cJSON.h"
#include "esp_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_functions.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "esp_system.h"
#include "tcpip_adapter.h"
#include "esp_event.h"
#include "esp_sntp.h"

#include "time.h"
#include "sys/time.h"
#include "esp_sleep.h"
#include "esp_heap_caps.h"

#include "soc/sens_periph.h"

#define BUF_SIZE 1024
#define TAG "ESP"
#define BLINK_GPIO GPIO_NUM_2
#define DEBUG 1

uint32_t caps;
int ret;

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
		printf("Response is .....................\n");
                printf("%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

void http_connect(void *pvParameters)
{
	for(;;)
	{
			
		ret = heap_caps_get_free_size(caps);
		if(DEBUG) printf("*********************************** Size %d in Bytes Free out of 520000 Bytes \n", ret);

		
		esp_http_client_config_t config = {
		   .url = "http://kea.cogniermail.com",
		   .event_handler = _http_event_handle,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);
		esp_err_t err = esp_http_client_perform(client);

		if (err == ESP_OK) {
		   ESP_LOGI(TAG, "Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
		}
		printf("_________________________________________\n");
		ret = heap_caps_get_free_size(caps);
		if(DEBUG) printf("*********************************** Size %d in Bytes Free out of 520000 Bytes \n", ret);
		
		
		esp_http_client_cleanup(client);

		ret = heap_caps_get_free_size(caps);
		if(DEBUG) printf("*********************************** Size %d in Bytes Free out of 520000 Bytes \n", ret);
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}

void app_main()
{
	wifi_initialise();
	wifi_wait_connected();

	xTaskCreate(http_connect, "http_connect", 5000, NULL, 5, NULL);
}
