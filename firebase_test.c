#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "esp_http_client.h"

#include "wifi_functions.h"
#include "esp_log.h"
#include "esp_err.h"
#include "cJSON.h"

char *string, *Json_string;
int dev =25;
int devt = 1735;

void fcm_start(void)
{
	int ret;
	esp_http_client_config_t config = {
       		.url = "http://fcm.googleapis.com/fcm/send",
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_http_client_set_method(client,  HTTP_METHOD_POST);

	esp_http_client_set_header(client, "Authorization", "key=++++++++");

	esp_http_client_set_header(client, "Content-Type", "application/json");
	
	esp_http_client_set_post_field(client, string, strlen(string));
	ret = esp_http_client_perform(client);
	printf("ret  is %d\n", ret);
}

void json(void) // Creating JSON format using own API
{
	cJSON *root,*fmt, *amt;
	root = cJSON_CreateObject();	
	cJSON_AddItemToObject(root, "to", cJSON_CreateString("fPHWNh_unjg:APA91bH3Pt2zEPhCiOfEae3_dTlcM-o-yA0hkCxp19P3gqS9f8M0zlccpb5NvFFEiFd-_C-r_AciMCdLwMUUaaMXWkU691ik85l6mjXX1NBjaaqSSH-uBL5X3nQwu2insB_4i4WGYu6v"));
	cJSON_AddStringToObject(root, "collapse_key", "type_a");

	cJSON_AddItemToObject(root, "notification", amt = cJSON_CreateObject());
	cJSON_AddStringToObject(amt,"title","ICC World CUP");
	cJSON_AddStringToObject(amt,"body","INDIA WON 2019 World cup");

	cJSON_AddItemToObject(root, "data", fmt=cJSON_CreateObject());
	cJSON_AddStringToObject(fmt,"title","ICC World CUP");
	cJSON_AddStringToObject(fmt,"title","INDIA WON 2019 World cup");

	string = cJSON_Print(root);
	//cJSON_Delete(root);
	printf("string is %s\n", string); 

	cJSON *json = cJSON_Parse(string);
	cJSON_AddNumberToObject(json, "device_id", dev);
	cJSON_AddNumberToObject(json, "device_time", devt);
	Json_string = cJSON_Print(json);
	printf("string is %s\n", Json_string); 


}

void app_main()
{
	//wifi_initialise();
//	wifi_wait_connected();
	json();
//	fcm_start();
}





