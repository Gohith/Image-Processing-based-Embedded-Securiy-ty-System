/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

*/

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
#include"driver/uart.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#define CONFIG_WIFI_SSID "Cognier-2"
#define CONFIG_WIFI_PASSWORD "bravia452"


#include "cJSON.h"


#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "wifi_functions.h"

static double FIRMWARE_VERSION	= 7.3;
#define UPDATE_JSON_URL		"http://192.168.1.6/firmware.json"
#define BLINK_GPIO 			GPIO_NUM_26

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


/*
====================================================================


//GPIO 2 functioning using FreeRTOS


static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
   printf("=============== EVENT LOOP =============\n");
   switch(event->event_id) {
        case MQTT_EVENT_CONNECTED:
           	 	 ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
          		 msg_id = esp_mqtt_client_subscribe(client, "/Its_Topic@452", 0);
          	 	 ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

           		// msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
          	 	// ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

         	  	// msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
         	  	// ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
         	  	 break; 

        case MQTT_EVENT_DISCONNECTED:
            		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
          		break;

        case MQTT_EVENT_SUBSCRIBED:
          		 ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
	    		// msg_id = esp_mqtt_client_publish(client, "/Its_Topic@452", "data", 0, 0, 0);
         	      //   ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
          		 break;

        case MQTT_EVENT_UNSUBSCRIBED:
            		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            		break;

        case MQTT_EVENT_PUBLISHED:
          		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            		break;

       case MQTT_EVENT_DATA:
            		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
           		printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            		printf("DATA=%.*s\r\n", event->data_len, event->data);

			ptr = event->data;
			printf("Data is %s \n", ptr);
			task_one(ptr);
            		break;
 
        case MQTT_EVENT_ERROR:
           		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
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

void uart_setup(void)
{
	
	uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

	uart_param_config(UART_NUM_0, &uart_config);
	uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(UART_NUM_0, 2048, 0, 10, NULL, 0);
}	

void uart_write(void)
{	
	const char str[] ="UART0 Using FreeRTOS && OTA Update";
	uart_write_bytes(UART_NUM_0, str, strlen(str));	
	vTaskDelay(3000 / portTICK_PERIOD_MS);
}



static void mqtt_app_start(void)
{

	//esp_event_loop_init(mqtt_event_handler, NULL);
	const char str[] = "Message from Tony Stark !!!";
	

	 const esp_mqtt_client_config_t mqtt_cfg = {
	
        //.uri = "mqtt://iot.eclipse.org:1883",

	.uri = "mqtt://test.mosquitto.org:1883",
        .event_handle = mqtt_event_handler,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);

  	esp_mqtt_client_subscribe(client, "/Its_Topic@452", 0);
	
	/*while(i<20)
	{
   		 esp_mqtt_client_publish(client, "/Its_Topic@452", str, strlen(str), 0, 0);  
		 esp_mqtt_client_start(client);

   		// esp_mqtt_client_subscribe(client, "/Its_Topic@452", 0);
		

  	  	
		i++;
	}
	*/
	
}

void check_update_task(void *pvParameter) {
	
	while(1) {

		
        
		printf("Looking for a new firmware in server...\n");
	
		
		esp_http_client_config_t config = {
       
			 .url = UPDATE_JSON_URL,
      			 .event_handler = _http_event_handler,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);
	
		// downloading the json file
		esp_err_t err = esp_http_client_perform(client);
		if(err == ESP_OK) {
			
			// parse the json file	
			cJSON *json = cJSON_Parse(rcv_buffer);
			if(json == NULL) printf("downloaded file is not a valid json, aborting...\n");
			else {	
				cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");
				
				//cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");
				
				// check the version
				if(!cJSON_IsNumber(version)) printf("unable to read new version, aborting...\n");
				else {
					
					double new_version = version->valuedouble;
					

					if(new_version > FIRMWARE_VERSION) {
					
						
						cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");

						printf("current firmware version (%.1f) is lower than the available one (%.1f), upgrading...\n",FIRMWARE_VERSION, new_version);
						
			
						if(cJSON_IsString(file) && (file->valuestring != NULL)) {
							printf("downloading and installing new firmware (%s)...\n", file->valuestring);
							
							
							esp_http_client_config_t ota_client_config = {
								.url = file->valuestring,
								//.cert_pem = server_cert_pem_start,
							};
							esp_err_t ret = esp_https_ota(&ota_client_config);							
							printf("%d\n \n", ret);
							if (ret == ESP_OK) {
								printf("OTA OK, restarting...\n");								
								esp_restart();
	
							} else {
								printf("OTA failed...\n");
							}
						}
						else printf("unable to read the new file name, aborting...\n");
					}
					else printf("current firmware version (%.1f) is greater or equal to the available one (%.1f), nothing to do...\n", FIRMWARE_VERSION, new_version);
					mqtt_app_start();					
					//xTaskCreate(&mqtt_app_start, "mqtt_app_start", 11024, NULL, 5, NULL);

				}

			}
		}
		else printf("unable to download the json file, aborting...\n");
		
		// cleanup
		esp_http_client_cleanup(client);
		
		printf("\n");
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

















void app_main()
{
	set_gpio();
	//uart_setup();	
	printf(" 2  HTTPS OTA, firmware %.1f\n\n", FIRMWARE_VERSION);
	wifi_initialise();
	wifi_wait_connected();
	xTaskCreate(&check_update_task, "check_update_task", 11024, NULL, 5, NULL);

}


*/

/*
=====================================================================
//UART0 functioning using FreerTOS

const char str[] ="UART0 Using FreeRTOS";
void uart_setup(void)
{
	
	uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

	uart_param_config(UART_NUM_0, &uart_config);
	uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(UART_NUM_0, 2048, 0, 10, NULL, 0);
}	

void task_one(void* pvParameters)
{
	for(;;)
	{
		uart_write_bytes(UART_NUM_0, str, strlen(str));	
	}
}

void app_main()
{
	uart_setup();
	xTaskCreate(task_one, "task_one", 1024, NULL, 5, NULL);
}	

=======================================================================
*/

/*


//MQTT Protocol Programming using FreeRTOS

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;
static const char *TAG = "MQTT_SAMPLE_Eg";


int j, i= 0;
void task_one(int);


static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id, msg_id_1;
   printf("=============== EVENT LOOP =============\n");
   switch(event->event_id) {
        case MQTT_EVENT_CONNECTED:
           	 	 ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
          		 msg_id = esp_mqtt_client_subscribe(client, "/Its_Topic@452", 0);
          	 	 ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

           		 msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
          	 	 ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

         	  	 msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
         	  	 ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
         	  	 break; 

        case MQTT_EVENT_DISCONNECTED:
            		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
          		break;

        case MQTT_EVENT_SUBSCRIBED:
          		 ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
	    		 msg_id = esp_mqtt_client_publish(client, "/Its_Topic@452", "data", 0, 0, 0);
         	         ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
          		 break;

        case MQTT_EVENT_UNSUBSCRIBED:
            		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            		break;

        case MQTT_EVENT_PUBLISHED:
          		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            		break;

       case MQTT_EVENT_DATA:
            		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
           		printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            		printf("DATA=%.*s\r\n", event->data_len, event->data);
			j = event->data_len;
			printf("J is %d \n", j);
			task_one(j);
            		break;
 
        case MQTT_EVENT_ERROR:
           		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            		break;

	default:	
			break;

    }
	
	task_one(j);
    return ESP_OK;
}


static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }

    return ESP_OK;
}

void set_gpio(void)
{
	gpio_pad_select_gpio(GPIO_NUM_2);
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	
}

void task_one(int j)
{
	int k = j%2;
	if(k == 0)
	{
		gpio_set_level(GPIO_NUM_2, 1);
	}
	else
	{
		gpio_set_level(GPIO_NUM_2, 0);	
	}	

}



static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
 //   ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}



static void mqtt_app_start(void)
{

	//esp_event_loop_init(mqtt_event_handler, NULL);
	const char str[] = "Message from Tony Stark !!!";
	

	 const esp_mqtt_client_config_t mqtt_cfg = {
	
        .uri = "mqtt://iot.eclipse.org",

	//.uri = "mqtt://test.mosquitto.org:1883",
        .event_handle = mqtt_event_handler,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
  
	//while(i<20)
	//{
   	//	 esp_mqtt_client_publish(client, "/Its_Topic@452", str, strlen(str), 0, 0);  
		

   		 esp_mqtt_client_subscribe(client, "/Its_Topic@452", 0);
		

  	  	 esp_mqtt_client_start(client);
	//	i++;
	//}
	
}
  

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    nvs_flash_init();
    wifi_init();
	
    mqtt_app_start();
	
	set_gpio();
	//task_one();
	//xTaskCreate(task_one, "task_one", 1024, NULL, 5, NULL);

	
}

*/

/*
==============================================================================

*/

/*
esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	if (event->event_id == SYSTEM_EVENT_SCAN_DONE) {
		printf("Number of access points found: %d\n", event->event_info.scan_done.number);
		uint16_t apCount = event->event_info.scan_done.number;
	if (apCount == 0) {
		return ESP_OK;
		}
	wifi_ap_record_t *list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));
	int i;
	for (i=0; i<apCount; i++) {
		char *authmode;
		switch(list[i].authmode) {
			case WIFI_AUTH_OPEN:
						authmode = "WIFI_AUTH_OPEN";
						break;
			case WIFI_AUTH_WEP:
						authmode = "WIFI_AUTH_WEP";
						break;
			case WIFI_AUTH_WPA_PSK:
						authmode = "WIFI_AUTH_WPA_PSK";
						break;
			case WIFI_AUTH_WPA2_PSK:
						authmode = "WIFI_AUTH_WPA2_PSK";
						break;
			case WIFI_AUTH_WPA_WPA2_PSK:
						authmode = "WIFI_AUTH_WPA_WPA2_PSK";
						break;
			default:
						authmode = "Unknown";
						break;
			}
	printf("ssid=%s, rssi=%d, authmode=%s\n", list[i].ssid, list[i].rssi, authmode);
	}
	free(list);
	}


	if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) 
	{
		printf("Our IP address is " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
		printf("We have now connected to a station and can do things...\n");
	}
	if (event->event_id == SYSTEM_EVENT_STA_START) 
	{
		ESP_ERROR_CHECK(esp_wifi_connect());
	
	}



	return ESP_OK;


}


int app_main(void)
{

	nvs_flash_init();
	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	ESP_ERROR_CHECK(esp_wifi_start());

	wifi_scan_config_t scanConf = {
		.ssid = NULL,
		.bssid = NULL,
		.channel = 0,
		.show_hidden = 1
		};

	ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, 0));

	wifi_config_t sta_config = {
		.sta = {
		.ssid = "Cognier-2",
		.password = "bravia452",
		.bssid_set = 0
		}
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());



	return 0;
}



*/