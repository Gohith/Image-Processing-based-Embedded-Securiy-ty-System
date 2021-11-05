						/* Code to perform httpsping gfot testing */


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
#include "esp_sntp.h"

#include "time.h"
#include "sys/time.h"
#include "esp_sleep.h"
#include "esp_heap_caps.h"

#define TAG "Https_esp"
#define DEBUG 1

int ret;
uint32_t caps;


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
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
       	        		if(DEBUG) printf("%.*s", evt->data_len, (char*)evt->data);
			}
            		break;
        	case HTTP_EVENT_ON_FINISH:
            		ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            		break;
        	case HTTP_EVENT_DISCONNECTED:
            		ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED, Connecting again !!");
			//esp_http_client_perform(client_H);           		
			break;
    	}
    	return ESP_OK;
}


void http_url_post_Hshake(void *pvParameters)
{
	for(;;)
	{
		if(DEBUG) printf("\n Came http_url_post_Hshake() ..................\n");

		ret = heap_caps_get_free_size(caps);
		if(DEBUG) printf(" 1 Size %d in Bytes Free out of 520000 Bytes \t", ret);
		
		esp_http_client_config_t config = {
	   		.url = "http://kea.cogniermail.com:8000/api/handshake/1/",
			.event_handler = _http_event_handler,
		};

		esp_http_client_handle_t client_H = esp_http_client_init(&config);

		//esp_http_client_set_method(client_H, HTTP_METHOD_POST);

		esp_http_client_set_header(client_H, "Content-Type", "application/json");

		//esp_http_client_set_post_field(client_H, UART_data_jSON_parse, strlen(UART_data_jSON_parse));

		esp_err_t err = esp_http_client_perform(client_H);
		
		ret = heap_caps_get_free_size(caps);

		if(DEBUG) printf(" 2 Size %d in Bytes Free out of 520000 Bytes \t", ret);

		
		if (err == ESP_OK) 
		{
	        	ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
		}
		else
		{
        		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		}

	}
}


void app_main()
{
	xTaskCreate(http_url_post_Hshake, "http_url_post_Hshake", 2048, NULL, 4, NULL);	
}
