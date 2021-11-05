					/* This is Sample main code for KEA project */

#include "stdio.h"
#include "stdint.h"
#include "stddef.h"
#include "string.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "cJSON.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "esp_wifi.h"

#include "esp_vfs.h"
#include "sys/unistd.h"

#define SPP_TAG "BT_SPP"

#define SSID "Cognier-2"
#define PASS "bravia452"

uint8_t  main_err, ret, count = 0;

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

char Device_PASSWORD[1024];
char D_PASS[] = {"Heypassword"};
char Dev_PASS[1024];
char *PASSWORD;
char buf[1024];

char Password_Match_result[] = {"Device Password Matched"};
char WIFI_Details[] = {"WiFi Details required"};

char WIFI_Edit_Response[] = {"WiFi Details Successfully edited"};
char BT_response[] = {"Device Connected"};
char BT_request[] = {"Send the ESP32 Password"};

esp_bt_pin_code_t pin_code;

void wifi_initialise(void);
void wifi_wait_connected(void);

int device_password_check(void)
{
	ESP_ERROR_CHECK(nvs_flash_init());
	if (main_err == ESP_ERR_NVS_NO_FREE_PAGES || main_err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
        	printf("nvs partition is not free\n");
	}
	nvs_handle_t P_handle;
	again:
		main_err = nvs_open("D_Password", NVS_READONLY, &P_handle);
	if (main_err != ESP_OK)
	{ 
		printf("NVS Not Opened\n");
		goto again;
	}	
	
	main_err = nvs_get_str(P_handle, "Dev_PASS", &Dev_PASS, strlen(Dev_PASS));
	printf((main_err != ESP_OK) ? "Failed!\n" : "Got Dev_PASS\n");
	printf("stored Dev PASS = %s\n", Dev_PASS);

	return strcmp(Dev_PASS, Device_PASSWORD);
}



void nvs_init()
{
	main_err = nvs_flash_init();
	if (main_err == ESP_ERR_NVS_NO_FREE_PAGES || main_err == ESP_ERR_NVS_NEW_VERSION_FOUND) 		
	{
        	printf("nvs partition is not free\n");
	}
	nvs_handle_t handle;
	nvs_handle_t P_handle;
	main_err = nvs_open("WiFiDetails", NVS_READWRITE, &handle);
	if (main_err == ESP_OK)
	{ 
		printf("Done opening\n");
	}
	main_err = nvs_open("D_Password", NVS_READWRITE, &P_handle);
	if (main_err == ESP_OK)
	{ 
		printf("Done opening D_Pass\n");
	}
//Write the Device Password here, before compiling
	main_err = nvs_set_str(P_handle, "D_PASS", D_PASS);
	printf((main_err != ESP_OK) ? "Failed!\n" : "D_PASS Writing Done\n");
	printf("Written D_Pass = %s\n", D_PASS);

	main_err = nvs_commit(handle);
	printf((main_err != ESP_OK) ? "Failed!\n" : " Commited\n");	
	main_err = nvs_commit(P_handle);
	printf((main_err != ESP_OK) ? "Failed!\n" : " Commited\n");	
	nvs_close(P_handle);
	nvs_close(handle);

}



int Edit_WiFi_Details(void)
{
	main_err = nvs_flash_init();
	if (main_err == ESP_ERR_NVS_NO_FREE_PAGES || main_err == ESP_ERR_NVS_NEW_VERSION_FOUND) 		
	{
        	printf("nvs partition is not free\n");
	}
	nvs_handle_t handle;
	main_err = nvs_open("WiFiDetails", NVS_READWRITE, &handle);
	if (main_err == ESP_OK)
	{ 
		printf("Done opening\n");
	}
	main_err = nvs_set_str(handle, "SSID", SSID);
	printf((main_err != ESP_OK) ? "Failed!\n" : "Writing Done\n");
	printf("Written SSID = %s\n", SSID);

	main_err = nvs_set_str(handle, "PASSWORD", PASSWORD);
	printf((main_err != ESP_OK) ? "Failed!\n" : "Writing Done\n");
	printf("Written PASSWORD = %s\n", PASSWORD);
	
	main_err = nvs_commit(handle);
	printf((main_err != ESP_OK) ? "Failed!\n" : " Commited\n");	
	nvs_close(handle);
	
	return ESP_OK;
	
}



static void BT_spp_CB(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
	
	switch(event)
	{
		case ESP_SPP_INIT_EVT:
			printf("Esp EVT SPP init\n");
			esp_bt_dev_set_device_name("ESP_DEVICE");
        		esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);			
			esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;    
			    pin_code[0] = '2';
			    pin_code[1] = '0';
			    pin_code[2] = '1';
			    pin_code[3] = 'B';
			    esp_bt_gap_set_pin(pin_type, 4, pin_code);

        		esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "SPP_SERVER");
			break;
		case ESP_SPP_DISCOVERY_COMP_EVT:
			printf("Discover complete\n");
			break;
		case ESP_SPP_OPEN_EVT:
			printf("ESP SPP open event\n");
			break;
		case ESP_SPP_DATA_IND_EVT:

			if(count == 0)
			{
				printf("ESP_SPP_DATA_IND_EVT len=%d handle=%d \n", param->data_ind.len, param->data_ind.handle);
				snprintf(buf, (size_t)param->data_ind.len, (char *)param->data_ind.data);
				printf("%s\n", buf);
				cJSON *json = cJSON_Parse(buf);	
				cJSON *Device_PASSWORD = cJSON_GetObjectItemCaseSensitive(json, "Device_PASSWORD");
				strcpy(Device_PASSWORD, Device_PASSWORD->valuestring);
				count++;
				ret = device_password_check();
				if(ret == 0)
				{
					esp_spp_write(param->write.handle, strlen(Password_Match_result), (uint8_t *)Password_Match_result);
					vTaskDelay(1000 / portTICK_PERIOD_MS);	
					esp_spp_write(param->write.handle, strlen(WIFI_Details), (uint8_t *)WIFI_Details);
					
				}
			}
			/* JSON string to be converted to JSON and access SSID and PASSWORD, Store in NVS */
			else
			{
				printf("ESP_SPP_DATA_IND_EVT len=%d handle=%d \n", param->data_ind.len, param->data_ind.handle);
				snprintf(buf, (size_t)param->data_ind.len, (char *)param->data_ind.data);
				printf("%s\n", buf);
				cJSON *json = cJSON_Parse(buf);			
				cJSON *SSID_I = cJSON_GetObjectItemCaseSensitive(json, "SSID");
				cJSON *PASSWORD_I = cJSON_GetObjectItemCaseSensitive(json, "PASSWORD");
				strcpy(SSID, SSID_I->valuestring);
				strcpy(PASSWORD, PASSWORD_I->valuestring);

				ret = Edit_WiFi_Details();
				if( ret == ESP_OK)
				{
					esp_spp_write(param->write.handle, strlen(WIFI_Edit_Response), (uint8_t *)WIFI_Edit_Response);

				}	
			}		
			
			break;			
		default:
			break;
	}

}
/*
static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
	
	switch(event)
	{
    		case ESP_BT_GAP_AUTH_CMPL_EVT:
        		if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
			{
            			ESP_LOGI("SPP_TAG", "Authentication success: %s", param->auth_cmpl.device_name);
            			esp_log_buffer_hex("SPP_TAG", param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
				esp_spp_write(param->write.handle, strlen(BT_response), (uint8_t *)BT_response);
				vTaskDelay(1000 / portTICK_PERIOD_MS);	
				esp_spp_write(param->write.handle, strlen(BT_request), (uint8_t *)BT_request);

        		}
 			else 
			{
            			ESP_LOGE("SPP_TAG", "authentication failed, status:%d", param->auth_cmpl.stat);
        		}
        		break;

	}
}
*/


static void BT_gap_CB(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
	switch(event)
	{
		case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
			ESP_LOGI(SPP_TAG, "ESP_BT_GAP_DISC_STATE_CHANGED_EVT");
			break;
		case ESP_BT_GAP_RMT_SRVCS_EVT:
			ESP_LOGI(SPP_TAG, "ESP_BT_GAP_RMT_SRVCS_EVT");
			break;
		case ESP_BT_GAP_RMT_SRVC_REC_EVT:
	       		break;
	    	case ESP_BT_GAP_AUTH_CMPL_EVT:
			if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) 
			{
			    	ESP_LOGI(SPP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
			    	esp_log_buffer_hex(SPP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
			}
			else
			{
		    		ESP_LOGE("SPP_TAG", "authentication failed, status:%d", param->auth_cmpl.stat);
			}
			break;
		case ESP_BT_GAP_PIN_REQ_EVT:
			ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
			if (param->pin_req.min_16_digit) 
			{
			    ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
			    esp_bt_pin_code_t pin_code = {0};
			    esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
			} 
			else 
			{
			    ESP_LOGI(SPP_TAG, "Input pin code: 1234");
			    esp_bt_pin_code_t pin_code;
			    pin_code[0] = '1';
			    pin_code[1] = '2';
			    pin_code[2] = '3';
			    pin_code[3] = '4';
			    esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
			}
			break;

#if (CONFIG_BT_SSP_ENABLED == true)
		case ESP_BT_GAP_CFM_REQ_EVT:
			ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
			esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
			break;
		case ESP_BT_GAP_KEY_NOTIF_EVT:
			ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
			break;
		case ESP_BT_GAP_KEY_REQ_EVT:
			ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
			break;
	#endif

		default:
			break;
	}
}


void bluetooth_initial(void)
{
	nvs_flash_init();
	esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
	
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	esp_bt_controller_init(&bt_cfg);

	esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
	esp_bluedroid_init();
	esp_bluedroid_enable();
	esp_bt_gap_register_callback(BT_gap_CB);
	esp_spp_register_callback(BT_spp_CB);
	esp_spp_init(ESP_SPP_MODE_CB);
}


void app_main()
{
	
	nvs_init();
	//wifi_initialise();
	
	if(main_err != ESP_OK)
	{		
		bluetooth_initial();

		main_err = nvs_flash_init();
		if (main_err == ESP_ERR_NVS_NO_FREE_PAGES || main_err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
		{
        		printf("nvs partition is not free\n");
		}
		nvs_handle_t handle;
		main_err = nvs_open("WiFiDetails", NVS_READWRITE, &handle);
		if (main_err == ESP_OK)
		{ 
			printf("Done opening\n");
		}
		main_err = nvs_set_str(handle, "SSID", SSID);
		printf((main_err != ESP_OK) ? "Failed!\n" : "Writing Done\n");
		//printf("Written SSID = %s\n", SSID);

		main_err = nvs_set_str(handle, "PASSWORD", PASSWORD);
		printf((main_err != ESP_OK) ? "Failed!\n" : "Writing Done\n");
		printf("Written PASSWORD = %s\n", PASSWORD);
	
		main_err = nvs_commit(handle);
		printf((main_err != ESP_OK) ? "Failed!\n" : " Commited\n");	
		nvs_close(handle);
			
	}
	wifi_wait_connected();
}

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

void wifi_initialise(void) 
{
	/*	
	uint8_t times = 0;
	// initialize NVS, required for wifi
	ESP_ERROR_CHECK(nvs_flash_init());
	
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
        	//printf("nvs partition is not free\n");
	}
	nvs_handle_t handle;
	again:
		err = nvs_open("WiFiDetails", NVS_READONLY, &handle);
	if (err != ESP_OK)
	{ 
		//printf("NVS Not Opened\n");
		goto again;
	}	
	
	err = nvs_get_str(handle, "SSID", &SSID, strlen(SSID));
	//printf((err != ESP_OK) ? "Failed!\n" : "Got SSID\n");
	//printf("stored SSID = %s\n", SSID);

	err = nvs_get_str(handle, "PASSWORD", &PASSWORD, strlen(PASSWORD));
	//printf((err != ESP_OK) ? "Failed!\n" : "Got PASSWORD\n");
	//printf("stored PASSWORD = %s\n", PASSWORD);
	
	err = nvs_commit(handle);
	//printf((err != ESP_OK) ? "Failed!\n" : " Commited\n");	
	nvs_close(handle);
	*/	
		
	ESP_ERROR_CHECK(nvs_flash_init());
	
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
            		.password = PASS,
        	},
    	};
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	esp_wifi_start();
	/*	
	wifi_config_t wifi_config;
	//memset(&wifi_config, 0, sizeof(wifi_config));
	strcpy((char*)wifi_config.sta.ssid, (const char*)SSID);
	strcpy((char*)wifi_config.sta.password, (const char*)PASSWORD);
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	wifi:	ret = esp_wifi_start();
	//printf("ret is %d\n", ret);
	if(ret == 0)
	{
		for(i = 1; i < 60; i++)
		{
			vTaskDelay(500 / portTICK_PERIOD_MS);	
			//printf("%d ", i);
			
			if(times > 0) return ret;
			
		}
		times++;
		goto wifi;
	}
	return ret;
	*/
}

void wifi_wait_connected(void)
{
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}
