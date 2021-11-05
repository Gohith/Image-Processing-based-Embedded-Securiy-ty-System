							/* ESP32 Sample Code For MQTT And InBuilt LED Operation */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "cJSON.h"


#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "wifi_functions.h"

static double FIRMWARE_VERSION	= //FIRMWARE_VERSION_HERE;

#define UPDATE_JSON_URL		"JSON_FILE_URL"
#define DEBUG 0


char rcv_buffer[200];

//extern const char server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
//extern const char server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;
static const char *TAG = "MQTT_SAMPLE_Eg";


int i= 0;
char* ptr;
char* ptr1 = "ON";
char* ptr2 = "OFF";
void task_one(char* j);



static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch(event->event_id) {
        case MQTT_EVENT_CONNECTED:
           	 	 if(DEBUG)ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
          		 msg_id = esp_mqtt_client_subscribe(client, "/Its_Topic@452", 0);
          	 	 if(DEBUG) ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
			 break;

        case MQTT_EVENT_DISCONNECTED:
            		if(DEBUG)ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
          		break;

        case MQTT_EVENT_SUBSCRIBED:
          		if(DEBUG) ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
          		break;

        case MQTT_EVENT_UNSUBSCRIBED:
            		if(DEBUG)ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            		break;

        case MQTT_EVENT_PUBLISHED:
          		if(DEBUG)ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            		break;

       case MQTT_EVENT_DATA:
            		if(DEBUG)ESP_LOGI(TAG, "MQTT_EVENT_DATA");
           		if(DEBUG)printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            		if(DEBUG)printf("DATA=%.*s\r\n", event->data_len, event->data);

			ptr = event->data;
			if(DEBUG)printf("Data is %s \n", ptr);
			task_one(ptr);
            		break;
 
        case MQTT_EVENT_ERROR:
           		if(DEBUG)ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            		break;

	default:	
			break;

    }
	
	
    return ESP_OK;
}



void task_one(char* j)
{
	if(j == ptr1)
	{
		gpio_set_level(GPIO_NUM_2, 1);
	}
	if(j == ptr2)
	{
		gpio_set_level(GPIO_NUM_2, 0);	
	}	

}


esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    
	switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            break;
        case HTTP_EVENT_ON_CONNECTED:
            break;
        case HTTP_EVENT_HEADER_SENT:
            break;
        case HTTP_EVENT_ON_HEADER:
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
				strncpy(rcv_buffer, (char*)evt->data, evt->data_len);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            break;
        case HTTP_EVENT_DISCONNECTED:
            break;
    }
    return ESP_OK;
}


void set_gpio(void)
{
	gpio_pad_select_gpio(GPIO_NUM_2);
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	
}

void blink(void)
{
	gpio_set_level(GPIO_NUM_2, 1);
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	
	gpio_set_level(GPIO_NUM_2, 0);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}


static void mqtt_app_start(void)
{
	

	 const esp_mqtt_client_config_t mqtt_cfg = {
	      	 //.uri = "mqtt://iot.eclipse.org:1883",

		   .uri = "mqtt://test.mosquitto.org:1883",
        	   .event_handle = mqtt_event_handler,
    	};

        esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);
  	esp_mqtt_client_subscribe(client, "/Its_Topic@452", 0);	
}

void check_update_task(void *pvParameter) 
{	
	while(1) 
	{
		if(DEBUG)printf("Looking for a new firmware in server...\n");	
		esp_http_client_config_t config = {      
			 .url = UPDATE_JSON_URL,
      			 .event_handler = _http_event_handler,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);			
		esp_err_t err = esp_http_client_perform(client);	// downloading the json file
		if(err == ESP_OK) 
		{		
			// parse the json file	
			cJSON *json = cJSON_Parse(rcv_buffer);
			if(json == NULL) if(DEBUG) printf("Downloaded file is not a valid json, aborting...\n");
			else 
			{	
				cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");				
				// check the version
				if(!cJSON_IsNumber(version)) printf("unable to read new version, aborting...\n");
				else 
				{					
					double new_version = version->valuedouble;	
					if(new_version > FIRMWARE_VERSION) 
					{						
						cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");
						if(DEBUG)printf("current firmware version (%.1f) is lower than the available one (%.1f), upgrading...\n",FIRMWARE_VERSION, new_version);									
						if(cJSON_IsString(file) && (file->valuestring != NULL)) 
						{
							if(DEBUG)printf("downloading and installing new firmware (%s)...\n", file->valuestring);
							
							
							esp_http_client_config_t ota_client_config = {
								.url = file->valuestring,
								//.cert_pem = server_cert_pem_start,
							};
							esp_err_t ret = esp_https_ota(&ota_client_config);							
							if(DEBUG)printf("%d\n", ret);
							if (ret == ESP_OK) {
								if(DEBUG)printf("OTA OK, restarting...\n");								
								esp_restart();
	
							} else {
								if(DEBUG)printf("OTA failed...\n");
							}
						}
						else if(DEBUG)printf("unable to read the new file name, aborting...\n");
					}
					else if(DEBUG)printf("current firmware version (%.1f) is greater or equal to the available one (%.1f), nothing to do...\n", FIRMWARE_VERSION, new_version);
					mqtt_app_start();					

				}

			}
		}
		else if(DEBUG)printf("unable to download the json file, aborting...\n");
		
		// cleanup
		esp_http_client_cleanup(client);
		
		printf("\n");
       		vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}


void app_main()
{
	set_gpio();
	printf(" 2nd  HTTPS OTA, firmware %.1f\n\n", FIRMWARE_VERSION);
	wifi_initialise();
	wifi_wait_connected();
	xTaskCreate(&check_update_task, "check_update_task", 11024, NULL, 5, NULL);
}

