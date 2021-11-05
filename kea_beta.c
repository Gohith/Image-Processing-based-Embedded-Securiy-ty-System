								/* Final Beta Firmware Code For KEA */
#include "stdio.h"
#include "string.h"
#include "stdint.h"

#include "e2000a.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
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

#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_wifi.h"

#define SSID "Cognier-2"
#define PASSWORD "bravia452"
#define DEBUG 1
#define TAG "ESP"
#define BUF_SIZE 2048
#define IO32 GPIO_NUM_32
#define MOSFET GPIO_NUM_18

const char JSON_URL[] = "http://kea.cogniermail.com:8000/api/firmware/main/latest";
char KEY_POST_URL[] = "http://kea.cogniermail.com:8000/api/key/";

char *user_id, JSON_rcv_buffer[200];
char dev_time[100], dev_ID[] = {"21"}, http_response[200];
char success_mess[] = {"success"};
char grant_msg[] = {"Access Granted!"};

const char FIRMWARE_VERSION[] = "15.0.0";
const char *UART_data_jSON_parse;

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

static QueueHandle_t uart2_queue, uart0_queue;
uint8_t UART_event_data[290];
uint32_t caps;
int temp, ret, err;

time_t now;
struct tm timeinfo;


/* This function compares the downloaded JSON version object with current firmware version (Linked to check_update_task function).
	 If Downloaded JSON Version is higher then returns true */
bool version_compare(const char *version_old, char *version_new)
{
	unsigned int major1 = 0, minor1 = 0, bugfix1 = 0;
	unsigned int major2 = 0, minor2 = 0, bugfix2 = 0;
	
	sscanf(version_old, "%u.%u.%u", &major1, &minor1, &bugfix1);
	sscanf(version_new, "%u.%u.%u", &major2, &minor2, &bugfix2);
	printf("Version 1 %u %u %u\n", major1, minor1, bugfix1);
	printf("Version 2 %u %u %u\n", major2, minor2, bugfix2);
	
	printf("%d", (major2 > major1) ? 1 : 0);
	printf("%d", (minor2 > minor1) ? 1 : 0);
	printf("%d", (bugfix2 > bugfix1) ? 1 : 0);

	if(major2 > major1) return 1;
	if(major2 == major1)
	{
		if(minor2 > minor1) return 1;
	}
	if(major2 == major1)
	{
		if(minor2 == minor1)
		{
			if(bugfix2 > bugfix1)return 1;
		}
	}

	return 0;
}

/* HTTP event handler for FOTA */
esp_err_t _http_event_handler_FOTA_(esp_http_client_event_t *evt) 
{    
	switch(evt->event_id) 
	{
	        case HTTP_EVENT_ERROR:
	            break;
	        case HTTP_EVENT_ON_CONNECTED:
	            break;
	        case HTTP_EVENT_HEADER_SENT:
	            break;
	        case HTTP_EVENT_ON_HEADER:
	            break;
	        case HTTP_EVENT_ON_DATA:
	            if (!esp_http_client_is_chunked_response(evt->client)) 
		    {
			strncpy(JSON_rcv_buffer, (char*)evt->data, evt->data_len);
	            }
	            break;
	        case HTTP_EVENT_ON_FINISH:
	            break;
	        case HTTP_EVENT_DISCONNECTED:
	            break;
	}
	return ESP_OK;
}

/* This function periodically Downloads JSON file and checks the version object.
	 The new firmware will dumped once the verifiation condition is true */
void check_update_task(void *pvParameter) 
{	
	while(1) 
	{     
		printf("Looking for a new firmware...\n");
		esp_http_client_config_t config = {
        		.url = JSON_URL,
      			.event_handler = _http_event_handler_FOTA_,
		};
		esp_http_client_handle_t client_F = esp_http_client_init(&config);
		esp_err_t err = esp_http_client_perform(client_F);
		if(err == ESP_OK) 
		{
			cJSON *json = cJSON_Parse(JSON_rcv_buffer);
			if(json == NULL) printf("Downloaded file is not a valid json, aborting...\n");
			else 
			{	
				cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");
				cJSON *path = cJSON_GetObjectItemCaseSensitive(json, "path");					
				char* new_version = version->valuestring;
				
				if(version_compare(FIRMWARE_VERSION, new_version))				
					{						
						printf("Current firmware version is lower than the available one, upgrading...\n");
						if(cJSON_IsString(path) && (path->valuestring != NULL)) 
						{
							printf("Downloading and installing new firmware (%s)...\n", path->valuestring);
							esp_http_client_config_t ota_client_config = {
								.url = path->valuestring,
							};
							esp_err_t ret = esp_https_ota(&ota_client_config);
							if (ret == ESP_OK) 
							{
								printf("OTA OK, restarting...\n");
								esp_restart();
							} 
							else 
							{
								printf("OTA failed...\n");
							}
						}
						else printf("unable to read the new file name, aborting...\n");
					}
					else printf("Current firmware version is greater or equal to the available one, nothing to do...\n");
			}
		}
		else printf("Unable to download the json file, aborting...\n");
		esp_http_client_cleanup(client_F);
        	vTaskDelay(600000 / portTICK_PERIOD_MS);
    	}
	vTaskDelete(NULL);
}

void obtain_time(void)
{
	sntp_init();
	vTaskDelay(3000 / portTICK_PERIOD_MS);
 	time(&now);
 	localtime_r(&now, &timeinfo);
	strftime(dev_time, sizeof(dev_time), "%c", &timeinfo);
	ESP_LOGI(TAG, "The current date/time in INDIA after updating is: %s", dev_time);
}

static void time_task(void *pvParameters)
{
	int count = 0;
	if(DEBUG) printf("NOW is %ld\n", now);
	for(;;)
	{
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
	vTaskDelete(NULL);
}

/* This function is to setting up the required GPIO pads*/
void gpio_setup(void)
{
	gpio_pad_select_gpio(IO32);
	gpio_pad_select_gpio(MOSFET);
	gpio_set_direction(IO32, GPIO_MODE_OUTPUT);
	gpio_set_direction(MOSFET, GPIO_MODE_OUTPUT);
}

/* This function is to customize the sound output from buzzer */
void buzz_sound(uint8_t sound)
{
	switch(sound)
	{
		case 1 :gpio_set_level(IO32, 1);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			gpio_set_level(IO32, 0); 
			break;
		case 2 :gpio_set_level(IO32, 1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			gpio_set_level(IO32, 0); 
			break;
		case 0 :gpio_set_level(IO32, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			gpio_set_level(IO32, 0); 
			vTaskDelay(200 / portTICK_PERIOD_MS);			
			gpio_set_level(IO32, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			gpio_set_level(IO32, 0);
			break;
		default:break;
	}
}

/* Function to Open Lock  */
void open_lock(void)
{
	gpio_pad_select_gpio(MOSFET);
	gpio_set_direction(MOSFET, GPIO_MODE_OUTPUT);
	gpio_set_level(MOSFET, 0);
	buzz_sound(1);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	gpio_set_level(MOSFET, 1);
 																						
}

/* Function to parse HTTP input from server and 
	verify the message from server inorder to Open the door */
void http_response_json_parse(void)
{
	cJSON *json = cJSON_Parse(http_response);
	if(strstr(http_response, "status") != NULL)	
	{
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
		else buzz_sound(0);
	}
}

/* HTTP event handler */
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
			break;
    	}
    	return ESP_OK;
}

/* Posting UART Input data 'KEY' to server */
void http_url_post_key(void)
{
	if(DEBUG) printf("\nCame http_url_post_key ..................\n");
	esp_http_client_config_t config = {
   		.url = KEY_POST_URL,
   		.event_handler = _http_event_handler,
	};
	esp_http_client_handle_t client_K = esp_http_client_init(&config);
	esp_http_client_set_method(client_K, HTTP_METHOD_POST);
	esp_http_client_set_header(client_K, "Content-Type", "application/json");
	
	esp_http_client_set_post_field(client_K, UART_data_jSON_parse, strlen(UART_data_jSON_parse));
	esp_err_t err = esp_http_client_perform(client_K);
	
	if (err == ESP_OK) 
	{
   		ESP_LOGI(TAG, "Status = %d, content_length = %d", esp_http_client_get_status_code(client_K), esp_http_client_get_content_length(client_K));	
		ESP_ERROR_CHECK(esp_http_client_cleanup(client_K));		
		http_response_json_parse();
	}
	else 
	{
	        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
	}
}

/* Function to JSON parse UART Iput Daa and append Time and ID of device */
int json_parse(void)
{	
	cJSON *json = cJSON_Parse((const char *)&UART_event_data);
	if(json == NULL) return 0;
	cJSON *usr = cJSON_GetObjectItemCaseSensitive(json, "user_id");
	user_id = usr->valuestring;
	strcat(KEY_POST_URL, user_id);
	
	cJSON_AddStringToObject(json, "device_id", dev_ID);
	cJSON_AddStringToObject(json, "device_time", dev_time);

	
	UART_data_jSON_parse = cJSON_Print(json);	
	printf("Sending cJSON to Server is ....................\n %s \n", UART_data_jSON_parse);
		
	if(strstr((const char *)UART_event_data, "key") != NULL)
	{
		http_url_post_key();
	}
	else printf("Error in HTTP posting KEY\n");

	return 0;
}

/* Function to initialize UART */
void uart_init(void)
{
	uart_config_t uart_2_config = {
        	.baud_rate = 9600,
        	.data_bits = UART_DATA_8_BITS,
        	.parity = UART_PARITY_DISABLE,
        	.stop_bits = UART_STOP_BITS_1,
        	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,		
    	};
	uart_param_config(UART_NUM_2, &uart_2_config);
	uart_set_pin(UART_NUM_2, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);  
	uart_driver_install(UART_NUM_2, BUF_SIZE * 3, BUF_SIZE * 3, 2048, &uart2_queue, 0);


/*	uart_config_t uart_0_config = {
        	.baud_rate = 9600,
        	.data_bits = UART_DATA_8_BITS,
        	.parity = UART_PARITY_DISABLE,
        	.stop_bits = UART_STOP_BITS_1,
        	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,		
    	};
	uart_param_config(UART_NUM_0, &uart_0_config);
	uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);  
	uart_driver_install(UART_NUM_0, BUF_SIZE * 3, BUF_SIZE * 3, 3072, &uart0_queue, 0);*/

}

void time_sync_notification_cb(struct timeval *tv)
{
	ESP_LOGI(TAG, "Notification of a time synchronization event");
}

/* Function to initialize Simple Network Time Protocol */
static void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    	sntp_init();
}

/* Function to get time and date information */
void s_ntp(void)
{
	initialize_sntp();
	vTaskDelay(3000 / portTICK_PERIOD_MS);
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

/* WiFi event handler*/
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

/* Function to Initialize WiFi */
void wifi_initialise(void) 
{
	ESP_ERROR_CHECK(nvs_flash_init());
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
}

void wifi_wait_connected(void)
{
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

/* Function to setup required device flow in order */
void DeviceStartUp(void)
{
	gpio_setup();
	uart_init();	
	wifi_initialise();
	wifi_wait_connected();
	s_ntp();
}

/* UART 2 Event HAndler */
void uart_2_event_task(void *pvParameters)
{
	if(DEBUG) printf("UART EVENT +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	uart_event_t event;
	while(1)
	{
		uart_flush_input(UART_NUM_2);
		//xQueueReset(uart2_queue);
		if(xQueueReceive(uart2_queue, (void * )&event, (portTickType)portMAX_DELAY))
		{
			switch(event.type)
			{	
				case UART_DATA:						
						ESP_LOGI(TAG, "Event DATA SIZE [UART DATA]: %d", event.size);
						uart_read_bytes(UART_NUM_2, &UART_event_data, 289, portMAX_DELAY);
						if(DEBUG) printf("Received data from QR Code is ...........\n %s \n", UART_event_data);
						json_parse();
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
			//esp_event_loop_delete(uart2_queue);
		}	
	}
	vTaskDelete(NULL);
}


void app_main()
{
	DeviceStartUp();
	gpio_set_level(MOSFET, 1);

	printf("Current Firmware version %s, This FOTA Code\n", FIRMWARE_VERSION);
	xTaskCreate(uart_2_event_task, "uart_2_event_task", 4096, NULL, 13, NULL);
	//xTaskCreate(uart_0_event_task, "uart_0_event_task", 2048, NULL, 7, NULL);
	xTaskCreate(check_update_task, "check_update_task", 2048, NULL, 5, NULL);
	xTaskCreate(time_task, "time_task", 2048, NULL, 6, NULL);	
}						
