
						/* MESH ROOT */

#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "tcpip_adapter.h"

#include "nvs_flash.h"

#define SSID "Cognier-2"
#define PASSWORD "bravia452"

#define CONFIG_MESH_ROUTER_SSID "Cognier-2"
#define CONFIG_MESH_ROUTER_PASSWD "bravia452"
#define CONFIG_MESH_AP_PASSWD "123456789"
#define CONFIG_MESH_CHANNEL 3

#define TX_SIZE 1024
#define RX_SIZE 1024


static const char *MESH_TAG = "mesh_main";
static const uint8_t MESH_ID[] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
static uint8_t tx_buf[TX_SIZE] = {0};
static uint8_t rx_buf[RX_SIZE] = {0};
static bool is_running = true;
static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int temp, mesh_layer = -1;
uint32_t caps;

mesh_addr_t route_table[3];
int route_table_size = 0;
// /////////////////////////////////////////////////////////////////////////////////////////////

void esp_mesh_p2p_rx_main(void* arg)
{
	//static const uint32_t from[6] = { 0xa4, 0xcf, 0x12, 0x0b, 0xa6, 0xc4 };
	mesh_addr_t from = {0};       	
	mesh_data_t data = {0};
    	int32_t flag = 0;
	data.data = rx_buf;
    	data.size = RX_SIZE;
	
	esp_err_t err;
	
	printf("Size of mesh_data_t is %u\n", sizeof(mesh_data_t));
	while(1)
	{		
		err = esp_mesh_recv((mesh_addr_t *) &from, &data, portMAX_DELAY, &flag, NULL, 0);
		//err = esp_mesh_recv((mesh_addr_t *)&from, (mesh_data_t *)&data, portMAX_DELAY, MESH_DATA_TODS, MESH_OPT_RECV_DS_ADDR, 0);		
		if(err != ESP_OK) ESP_LOGE(MESH_TAG, "err:0x%x ", err);
		printf("Received message was \t%s\n", data.data);
	}
	vTaskDelete(NULL);
}

esp_err_t esp_mesh_comm_p2p_start(void)
{

	xTaskCreate(esp_mesh_p2p_rx_main, "MPRX", 3072, NULL, 5, NULL);
    	return ESP_OK;
}

void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{	
	mesh_addr_t id = {0,};
	static uint8_t last_layer = 0;

	esp_err_t err;
	switch (event_id) 
	{
		case MESH_EVENT_STARTED: 
		{
        		esp_mesh_get_id(&id);
        		ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        		is_mesh_connected = false;
        		mesh_layer = esp_mesh_get_layer();			
    		}
    		break;
    
		case MESH_EVENT_STOPPED: 
		{
		        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
		        is_mesh_connected = false;
		        mesh_layer = esp_mesh_get_layer();
		}
    		break;

		case MESH_EVENT_CHILD_CONNECTED: 
		{
			mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"", child_connected->aid, MAC2STR(child_connected->mac));
			esp_mesh_comm_p2p_start();
		}
	    	break;

		case MESH_EVENT_CHILD_DISCONNECTED: 
		{
			mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"", child_disconnected->aid, MAC2STR(child_disconnected->mac));
	    	}
	    	break;

	    	case MESH_EVENT_ROUTING_TABLE_ADD: 
		{
        		mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        		ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d", routing_table->rt_size_change, routing_table->rt_size_new);
    		}
    		break;

		case MESH_EVENT_ROUTING_TABLE_REMOVE: 
		{
		        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        		ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d", routing_table->rt_size_change, routing_table->rt_size_new);
    		}
    		break;

	    	case MESH_EVENT_NO_PARENT_FOUND: 
		{
        		mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        	ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d", no_parent->scan_times);
    		}
    		break;

		case MESH_EVENT_PARENT_CONNECTED: 
		{
			mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
			esp_mesh_get_id(&id);
			mesh_layer = connected->self_layer;
			memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 	esp_mesh_is_root() ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
        		last_layer = mesh_layer;
       // mesh_connected_indicator(mesh_layer);
        		is_mesh_connected = true;
        		if (esp_mesh_is_root()) 
			{
            			tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        		}
        //esp_mesh_comm_p2p_start();
    		}
    		break;

		case MESH_EVENT_PARENT_DISCONNECTED: 
		{
			mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d", disconnected->reason);
			is_mesh_connected = false;
		//mesh_disconnected_indicator();
			mesh_layer = esp_mesh_get_layer();
	    	}
    		break;

		case MESH_EVENT_LAYER_CHANGE: 
		{
			mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
			mesh_layer = layer_change->new_layer;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s", last_layer, mesh_layer, esp_mesh_is_root() ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>" : "");
			last_layer = mesh_layer;
			//mesh_connected_indicator(mesh_layer);
		}
	 	break;

	    	case MESH_EVENT_ROOT_ADDRESS: 
		{
			mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"", MAC2STR(root_addr->addr));
    		}
    		break;

		case MESH_EVENT_VOTE_STARTED: 
		{
			mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"", vote_started->attempts, vote_started->reason, MAC2STR(vote_started->rc_addr.addr));
	    	}
	    	break;

		case MESH_EVENT_VOTE_STOPPED: 
		{
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
			break;
		}

		case MESH_EVENT_ROOT_SWITCH_REQ: 
		{
			mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
			ESP_LOGI(MESH_TAG,"<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"", switch_req->reason, MAC2STR( switch_req->rc_addr.addr));
	    	}
	    	break;

		case MESH_EVENT_ROOT_SWITCH_ACK: 
		{
			mesh_layer = esp_mesh_get_layer();
			esp_mesh_get_parent_bssid(&mesh_parent_addr);
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
	    	}
	    	break;

		case MESH_EVENT_TODS_STATE: 
		{
			mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
	    	}
	    	break;

		case MESH_EVENT_ROOT_FIXED: 
		{
			mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s", root_fixed->is_fixed ? "fixed" : "not fixed");
		}
	    	break;

		case MESH_EVENT_ROOT_ASKED_YIELD: 
		{
			mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d", MAC2STR(root_conflict->addr), root_conflict->rssi, root_conflict->capacity);
	    	}
	    	break;

		case MESH_EVENT_CHANNEL_SWITCH: 
		{
			mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
	    	}
	    	break;

		case MESH_EVENT_SCAN_DONE: 
		{
			mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d", scan_done->number);
	   	}
	    	break;

	    	case MESH_EVENT_NETWORK_STATE: 
		{
			mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d", network_state->is_rootless);
	    	}
		break;

		case MESH_EVENT_STOP_RECONNECTION: 
		{
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
		}
	    	break;

		case MESH_EVENT_FIND_NETWORK: 
		{
			mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"", find_network->channel, MAC2STR(find_network->router_bssid));
		}
		break;

		case MESH_EVENT_ROUTER_SWITCH: 
		{
			mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
			ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"", router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
		}
		break;

		default:
			ESP_LOGI(MESH_TAG, "unknown id:%d", event_id);
			break;
    	}
}

// ////////////////////////////////////////////////////////////////////////////////////////////

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch(event->event_id)
	{		
		case SYSTEM_EVENT_STA_START:
			esp_wifi_connect();
			break;
	    
		case SYSTEM_EVENT_STA_GOT_IP:
			break;
	    
		case SYSTEM_EVENT_STA_DISCONNECTED:
			esp_wifi_connect();	
			break;
	    
		default:
			break;
	}	   
	return ESP_OK;
}

static void heap_task(void* pvParameters)
{
	while(1)
	{
		
		temp = heap_caps_get_free_size(caps);
		printf("*********************************** Size %d in Bytes Free out of 520000 Bytes \n", temp);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}



void app_main()
{
	
	
	nvs_flash_init();
	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	wifi_config_t wifi_config = {
        	.sta = {
            		.ssid = SSID,
            		.password = PASSWORD,
        	},
    	};
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

	esp_wifi_start();	
	printf("+++++++++++++++++++++++++++++++++++++++++++++++\n");
	

	esp_mesh_init();
	esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL);

	esp_mesh_set_max_layer(2);
	mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
	memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);

	cfg.channel = CONFIG_MESH_CHANNEL;
	cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
	
	memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
	memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD, strlen(CONFIG_MESH_ROUTER_PASSWD));

	esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK);
	cfg.mesh_ap.max_connection = 5;
	memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD, strlen(CONFIG_MESH_AP_PASSWD));
	esp_mesh_set_config(&cfg);

	esp_mesh_fix_root(true);
	esp_mesh_set_parent(&wifi_config, (mesh_addr_t *) MESH_ID, MESH_ROOT, MESH_ROOT_LAYER);
	printf("MESH_ROOT_LAYER is %d \n", MESH_ROOT_LAYER);
	
	esp_mesh_set_self_organized(false, false);	

	esp_mesh_start();
	
	ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s\n",  esp_get_free_heap_size(), esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed");
	ESP_LOGI(MESH_TAG, "layer:%d, rtableSize:%d, %s", mesh_layer, esp_mesh_get_routing_table_size(), (is_mesh_connected && esp_mesh_is_root()) ? "ROOT" : is_mesh_connected ? "NODE" : "DISCONNECT");
	esp_mesh_get_routing_table((mesh_addr_t *) &route_table, 3 * 6, &route_table_size);
	printf("Route table size is %d\n", route_table_size);
	printf("SSID of ROOT is %s \n", wifi_config.ap.ssid);

	xTaskCreate(heap_task, "time_task", 2048, NULL, 4, NULL);		
}
