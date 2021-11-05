					/* KEA SAMPLE CODE FOR UART AND CONNECTING TO SERVER */
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
#include "freertos/event_groups.h"

#include "esp_wifi.h"

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


#define SSID "Cognier-2"
#define PASSWORD "bravia452"


//char SSID[] = "Cognier-2", PASSWORD[] = "bravia452";
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

static QueueHandle_t uart2_queue;
const char *UART_data_jSON_parse;
char dev_time[100], dev_ID[] = {"21"}, http_response[200];
char success_mess[] = {"success"};

uint8_t UART_event_data[260];
char grant_msg[] = {"Access Granted!"};

uint32_t caps;
int temp, ret, err;

time_t now;
struct tm timeinfo;

void wifi_initialise(void);
void wifi_wait_connected(void);


/* Function time_sync_notification_cb() is call back function which shows the event */

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

/* Function open_lock() performs the selected GPIO PAD and gives output on that GPIO Pin ON.
	Delays for 5 seconds and goes PIN OFF   */

void open_lock(void)
{
	gpio_pad_select_gpio(BLINK_GPIO);
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(BLINK_GPIO, 1);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	gpio_set_level(BLINK_GPIO, 0);
 																						
}

/* Function http_response_json_parse() performs the parsing incoming JSON string form HTTP Server.
	 Decodes specfic item form object */

void http_response_json_parse(void)
{
	cJSON *json = cJSON_Parse(http_response);
		
/*	if(strstr(http_response, "msg") != NULL)
	{
		cJSON *message = cJSON_GetObjectItemCaseSensitive(json, "msg");
		char *message_A = message->valuestring;
		ret = strcmp(success_mess, message_A);
		if(ret == 0)
		{
			if(DEBUG) printf("Handshake code matched\n");
		}
		else
		{
			if(DEBUG) printf("Handshake code Not matched\n");
		}
		
	}
*/
	if(strstr(http_response, "status") != NULL)	
	{
		//if(DEBUG) printf("Entered into first IF \n");
		cJSON *status = cJSON_GetObjectItemCaseSensitive(json, "status");
		char *status_message = status->valuestring;
		ret = strcmp(success_mess, status_message);
		if(ret == 0)
		{
			if(DEBUG) printf("Door Unlocked \n");
			cJSON *message = cJSON_GetObjectItemCaseSensitive(json, "message");
			char *door_message = message->valuestring;
			ret = strcmp(grant_msg, door_message);
			if(ret == 0)
			{
				open_lock();
			}
		}
	}
}

/* Function _http_event_handler() is Event handler for HTTP */

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
				strncpy(http_response, (char*)evt->data, evt->data_len);
				http_response_json_parse();						      		
			}			
			temp = heap_caps_get_free_size(caps);
	        	printf(" in_event_handler_DATA Size %d in Bytes Free out of 520000 Bytes *********************************** \n", temp);
            		break;
        	case HTTP_EVENT_ON_FINISH:
            		ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            		break;
        	case HTTP_EVENT_DISCONNECTED:
            		ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED, Connecting again !!");
			//esp_http_client_perform(client);           		
			break;
    	}
    	return ESP_OK;
}

/* Function initialize_sntp() initializes SNTP */

static void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    	sntp_init();
}

/* Function obtain_time() for obtaining Time from SNTP*/

void obtain_time(void)
{
	sntp_init();
	vTaskDelay(3500 / portTICK_PERIOD_MS);
 	time(&now);
 	localtime_r(&now, &timeinfo);
	strftime(dev_time, sizeof(dev_time), "%c", &timeinfo);
	ESP_LOGI(TAG, "The current date/time in INDIA is: %s", dev_time);
}

/* FreeRTOS task for continuos incrementing of timestamp for every second
	and to connect to server once for every one hour to avoid long term time delay */

static void time_task(void *pvParameters)
{
	int count = 0;
	if(DEBUG) printf("NOW is %ld\n", now);
	for(;;)
	{
		temp = heap_caps_get_free_size(caps);
		if(DEBUG) printf(" Size %d in Bytes Free out of 520000 Bytes \t", temp);	

		vTaskDelay(1000 / portTICK_PERIOD_MS);
		now++;
		if(DEBUG) printf("NOW is %ld\n", now);
		time(&now);
	    	localtime_r(&now, &timeinfo);
		strftime(dev_time, sizeof(dev_time), "%c", &timeinfo);
	    	ESP_LOGI(TAG, "The current date/time in INDIA is: %s", dev_time);
		count++;
		if(count > 3600)
		{
			obtain_time();
			count = 0;
		}
	}
}

/* Function s_ntp(), Which Obtains time from function obtain_time() at device startup */

void s_ntp(void)
{
	initialize_sntp();
	time_t now;
	struct tm timeinfo;
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
	strftime(dev_time, sizeof(dev_time), "%c", &timeinfo);
	ESP_LOGI(TAG, "The current date/time in INDIA is: %s", dev_time);	
}


/* Function http_url_post_key(), used for posting data to a KEY URL */ 

void http_url_post_key(void)
{
	if(DEBUG) printf("\n Came http_url_post_key ..................\n");
	
	esp_http_client_config_t config = {
   		.url = "http://kea.cogniermail.com:8000/api/key/1/",
   		.event_handler = _http_event_handler,
	};

	esp_http_client_handle_t client_K = esp_http_client_init(&config);
	
	esp_http_client_set_method(client_K, HTTP_METHOD_POST);

	esp_http_client_set_header(client_K, "Content-Type", "application/json");
	
	esp_http_client_set_post_field(client_K, UART_data_jSON_parse, strlen(UART_data_jSON_parse));
		
	esp_err_t err = esp_http_client_perform(client_K);

	temp = heap_caps_get_free_size(caps);
	if(DEBUG) printf(" Size %d in Bytes Free out of 520000 Bytes && err is %d\n", temp, err);

	if (err == ESP_OK) 
	{
   		ESP_LOGI(TAG, "Status = %d, content_length = %d", esp_http_client_get_status_code(client_K), esp_http_client_get_content_length(client_K));	
		http_response_json_parse();
	}
	else 
	{
	        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
	}
}


void json_(void)
{
	cJSON *root;
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "user_id", cJSON_CreateString("2"));
	cJSON_AddItemToObject(root, "entity_id", cJSON_CreateString("6"));
	cJSON_AddItemToObject(root, "dev_id", cJSON_CreateString("6"));
	
	cJSON_AddItemToObject(root, "key", cJSON_CreateString("eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJ1c2VyX2lkIjoyLCJkYXRldGltZSI6IjIwMTktMDktMDUgMDk6NDI6NDcuNjgwMzE3IiwiZGV2X2lkIjoxLCJlbnRpdHlfaWQiOjF9.xGGpWhBESsqpxnPFflyZ7eQ6pPpXXotadaAAsPVc1Cg"));

	cJSON_AddItemToObject(root, "app_id", cJSON_CreateString("j5l1S4"));
	cJSON_AddItemToObject(root, "app_time", cJSON_CreateString("431392"));

	UART_data_jSON_parse = cJSON_Print(root);
	if(DEBUG) printf("string is %s\n", UART_data_jSON_parse); 
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	//http_url_post_Hshake();
}




/* Function json_parse(), used to parse the incoming json data from UART port 
	and ADD specific items to object */

void json_parse(void)
{
	cJSON *json = cJSON_Parse((const char *)UART_event_data);
	cJSON_AddStringToObject(json, "device_id", dev_ID);
	cJSON_AddStringToObject(json, "device_time", dev_time);
	UART_data_jSON_parse = cJSON_Print(json);

	if(DEBUG) printf("Sending cJSON to Server is ....................\t %s \n", UART_data_jSON_parse);

	if(strstr((const char *)UART_event_data, "key") != NULL)
	{
		http_url_post_key();
	}
}


//void http_connect(void *pvParameters)
void http_connect(void)
{	
	//for(;;)
	//{
		temp = heap_caps_get_free_size(caps);
		if(DEBUG) printf("*********************************** Size %d in Bytes Free out of 520000 Bytes \n", temp);
		
		
		if(DEBUG) printf("\n ................................ 	HTTP PINGING	...................................\n");		

		esp_http_client_config_t config = {
   			.url = "http://kea.cogniermail.com:8000/api/key/2/",
   			.event_handler = _http_event_handler,
		};

		esp_http_client_handle_t client_H = esp_http_client_init(&config);

		esp_http_client_set_method(client_H, HTTP_METHOD_POST);

		esp_http_client_set_header(client_H, "Content-Type", "application/json");

		esp_http_client_set_post_field(client_H, UART_data_jSON_parse, strlen(UART_data_jSON_parse));

		esp_err_t err = esp_http_client_perform(client_H);
				
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
		if (err == ESP_OK) 
		{
	        	ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client_H), esp_http_client_get_content_length(client_H));
		}
		else
		{
        		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		}
		
		ESP_ERROR_CHECK(esp_http_client_cleanup(client_H));
		//ESP_LOGE(TAG, " %s", esp_err_to_name(err));
	//}
}



/* Function uart_2_event_task(), is UART event handler */
static void uart_2_event_task(void *pvParameters)
{
	if(DEBUG) printf("UART EVENT +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	uart_event_t event;
	uint8_t UART_data[2];
	for(;;)
	{
		
		//uart_reset_rx_fifo(UART_NUM_2);
		if(xQueueReceive(uart2_queue, (void * )&event, (portTickType)portMAX_DELAY))
		{
			uint8_t data = 0;
			switch(event.type)
			{	
				case UART_DATA:	
											
						ESP_LOGI(TAG, "Event DATA SIZE [UART DATA]: %d", event.size);
					
					/*	if(event.size < 120)
						{
							uart_read_bytes(UART_NUM_2, UART_event_data, event.size, portMAX_DELAY);
							
						
						}
						else
						{
							uart_read_bytes(UART_NUM_2, UART_event_data, 230, portMAX_DELAY);
						
						}*/
						
						do
						{
							uart_read_bytes(UART_NUM_2, &UART_data, 1, portMAX_DELAY);
							UART_event_data[data++] = UART_data[0];
							if(data > 250) break;
						}while(UART_data[0] != '}');						
						
						ESP_LOGI(TAG, "READ DATA SIZE [UART DATA]: %d",data);
                	    			
			    			if(DEBUG) printf("Received data from QR Code is ........... %s\n", UART_event_data);	
						uart_flush_input(UART_NUM_2);											
						if(data < 250 )json_parse();
						break;
	
				case UART_FIFO_OVF:
                	   			ESP_LOGI(TAG, "H/W fifo overflow");
                	    			uart_flush_input(UART_NUM_2);
                	   			xQueueReset(uart2_queue);
                	    			break;
	
				case UART_BUFFER_FULL:
                	   			ESP_LOGI(TAG, "ring buffer full");
                	    			uart_flush_input(UART_NUM_2);
                	   			xQueueReset(uart2_queue);
                	   			break;
				case UART_BREAK:
                	    			ESP_LOGI(TAG, "uart rx break");
                	   			break;
				
				case UART_PARITY_ERR:
                	    			ESP_LOGI(TAG, "uart parity error");
                	    			break;

                		case UART_FRAME_ERR:
                	    			ESP_LOGI(TAG, "uart frame error");
                	    			break;			

				default :
						ESP_LOGI(TAG, "Event raised: %d", event.type);
						break;
			}
			esp_event_loop_delete(uart2_queue);
		}	
	}
}

/* Function uart_init(), initalizes UART port setup on device startup */

void uart_init(void)
{
	uart_config_t uart_config = {
        	.baud_rate = 9600,
        	.data_bits = UART_DATA_8_BITS,
        	.parity = UART_PARITY_DISABLE,
        	.stop_bits = UART_STOP_BITS_1,
        	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    	};
	uart_param_config(UART_NUM_2, &uart_config);
	uart_set_pin(UART_NUM_2, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);  
	uart_driver_install(UART_NUM_2, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart2_queue, 0);
}

/* Main program that runs after device startups and successive reset */

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch(event->event_id)
	{		
		case SYSTEM_EVENT_STA_START:
			esp_wifi_connect();
			break;
	    
		case SYSTEM_EVENT_STA_GOT_IP:
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
			break;
	    
		case SYSTEM_EVENT_STA_DISCONNECTED:
			esp_wifi_connect();	
			break;
	    
		default:
			break;
	}	   
	return ESP_OK;
}

void NvsInitialise(void)
{
	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
        	printf("NVS partition is not free\n");
	}
	nvs_handle_t handle;
	err = nvs_open("WiFi_Details", NVS_READWRITE, &handle);
	if (err == ESP_OK)
	{ 
		printf("Done opening\n");
	}
	err = nvs_commit(handle);
	nvs_close(handle);

}


void wifi_initialise(void) 
{
		
	uint8_t i, times = 0;

	/*ESP_ERROR_CHECK(nvs_flash_init());
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
        	printf("nvs partition is not free\n");
	}
	nvs_handle_t handle;
	again:
		err = nvs_open("WiFi_Details", NVS_READONLY, &handle);
	if (err != ESP_OK)
	{ 
		printf("NVS Not Opened\n");
		goto again;
	}	
	
	err = nvs_get_str(handle, "SSID", &SSID, strlen(SSID));
	printf((err != ESP_OK) ? "Failed!\n" : "Got SSID\n");
	printf("stored SSID = %s\n", SSID);

	err = nvs_get_str(handle, "PASSWORD", &PASSWORD, strlen(PASSWORD));
	printf((err != ESP_OK) ? "Failed!\n" : "Got PASSWORD\n");
	printf("stored PASSWORD = %s\n", PASSWORD);
	
	err = nvs_commit(handle);
	//printf((err != ESP_OK) ? "Failed!\n" : " Commited\n");	
	nvs_close(handle);
		
	*/	
	//ESP_ERROR_CHECK(nvs_flash_init());
	
	// connect to the wifi network
	wifi_event_group = xEventGroupCreate();
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	wifi_config_t wifi_config = {
        	.sta = {
            		.ssid = SSID,
            		.password = PASSWORD
        	},
    	};
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	esp_wifi_start();
	
/*	wifi_config_t wifi_config;
	strcpy((char*)wifi_config.sta.ssid, (const char*)SSID);
	strcpy((char*)wifi_config.sta.password, (const char*)PASSWORD);
//	printf("SSID: %s, len = %d \n", wifi_config.sta.ssid, strlen((uint8_t)wifi_config.sta.ssid));
//	printf("PASS: %s, len = %d\n", wifi_config.sta.password, strlen((uint8_t)wifi_config.sta.password));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	wifi:	
		ret = esp_wifi_start();
		
		printf("ret is %d\n", ret);
	if(ret != ESP_OK)
	{
		for(i = 1; i < 30; i++)
		{
			vTaskDelay(500 / portTICK_PERIOD_MS);	
		}
		times++;
		goto wifi;
	}*/
}

void wifi_wait_connected(void)
{
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

void DeviceStartUp(void)
{
	uart_init();	
	NvsInitialise();
	wifi_initialise();
	wifi_wait_connected();
	s_ntp();
}

void app_main()
{
	DeviceStartUp();

	int res = GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S);
    	if(DEBUG) printf("res=%d\n", res);	
	json_();
	http_connect();	
		
	xTaskCreate(uart_2_event_task, "uart_2_event_task", 2048, NULL, 6, NULL);
	//xTaskCreate(http_connect, "http_connect", 5000, NULL, 5, NULL);
	xTaskCreate(time_task, "time_task", 2048, NULL, 4, NULL);	
}





