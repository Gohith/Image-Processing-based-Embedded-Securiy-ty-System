#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

/*void app_main()
{
	int err;
	char str[] = "Hello Again !!";
	err = nvs_flash_init();

	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
        	ESP_ERROR_CHECK(nvs_flash_erase());
        	err = nvs_flash_init();
	}
	
	nvs_handle_t handle;
	err = nvs_open("storage", NVS_READWRITE, &handle);	
	if (err == ESP_OK)
	{
		printf("Done opening\n");
	}	
	err = nvs_set_str(handle, "string", str);
	printf((err != ESP_OK) ? "Failed!\n" : "Writing Done\n");
	printf("str = %s\n", str);
	
	err = nvs_commit(handle);
	printf((err != ESP_OK) ? "Failed!\n" : " Commit Done\n");	
	nvs_close(handle);
}*/

void app_main()
{
	int err;
	size_t size = 20;
	char str[20];
	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
        	printf("nvs partition is not free\n");
	}
	nvs_handle_t handle;
	err = nvs_open("storage", NVS_READWRITE, &handle);
	if (err == ESP_OK)
	{ 
		printf("Done opening\n");
	}
	err = nvs_get_str(handle, "string", &str, &size);
	printf((err != ESP_OK) ? "Failed!\n" : "Writing Done\n");
	printf("stored string str = %s\n", str);
	
	err = nvs_commit(handle);
	printf((err != ESP_OK) ? "Failed!\n" : " Commited\n");	
	nvs_close(handle);
}




