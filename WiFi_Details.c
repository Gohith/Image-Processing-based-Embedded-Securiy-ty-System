#include "wifi_functions.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_heap_caps.h"

wifi_mode_t mode;
wifi_ap_record_t ap_info;
wifi_country_t country;
wifi_auth_mode_t auth;

uint8_t power;
uint32_t caps;
float ret;

void app_main()
{
	wifi_initialise();
	wifi_wait_connected();

	esp_wifi_get_mode(&mode);
	printf("MODE is %d\n", mode);

	esp_wifi_sta_get_ap_info(&ap_info);
	printf("RSSI is %d\n", ap_info.rssi);
	printf("SSID of AP is %s\n", ap_info.ssid);

	esp_wifi_get_country(&country);
	printf("Country of AP is %s\n", country.cc);

	printf("MAX Power of AP is %d\n", country.max_tx_power);
	printf("Antenna of AP is %d\n", ap_info.ant);
	
	printf("Channel of AP is %d\n", ap_info.primary);

	esp_wifi_get_max_tx_power(power);
	printf("Max Transmission Power is %d\n", power);
	

	ret = heap_caps_get_free_size(caps);
	printf(" Size %f in Bytes Free out of 520000 Bytes \t", ret);

	ret = (ret * 100/ 520000) ;
	printf("Percentage All Free Menmory is %f\n", ret);




}
