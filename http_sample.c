#include "stdio.h"
#include "string.h"
#include "stdint.h"

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

#include "esp_log.h"
#include "esp_system.h"
#include "tcpip_adapter.h"
#include "esp_event.h"


#define BUF_SIZE 1024
#define TAG "ESP_UART_2"

const char Json_string[] = {"Hello World !!"};

int err;

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
	switch(evt->event_id) 
	{
		case HTTP_EVENT_ERROR:
            		ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            		break;
        	case HTTP_EVENT_ON_CONNECTED:
            		ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
           		break;
        	case HTTP_EVENT_HEADER_SENT:
            		ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            		break;
        	case HTTP_EVENT_ON_HEADER:
            		ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            		break;
        	case HTTP_EVENT_ON_DATA:
       			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            		if (!esp_http_client_is_chunked_response(evt->client))
			{
                // Write out data
                		printf("%.*s", evt->data_len, (char*)evt->data);
            		}

            		break;
        	case HTTP_EVENT_ON_FINISH:
            		ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            		break;
        	case HTTP_EVENT_DISCONNECTED:
            		ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            		break;
    	}
    	return ESP_OK;
}

void http_url_post(void)
{
	esp_http_client_config_t config = {
   		.url = "http://fcm.googleapis.com/fcm/send",
   		.event_handler = _http_event_handle,
		};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	if (err == ESP_OK) 
	{
   		ESP_LOGI(TAG, "Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
	}

	esp_http_client_set_post_field(client, Json_string, strlen(Json_string));
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) 
	{
   		ESP_LOGI(TAG, "Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
	}
	else 
	{
	        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
	}


}




void app_main()
{
	wifi_initialise();
	wifi_wait_connected();
	http_url_post();
}
