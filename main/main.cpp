#include <esp_log.h>
#include <string>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <freertos/task.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <sys/time.h>
#include <string.h>
#include "SensorData.h"

#define	LED_GPIO_PIN			GPIO_NUM_4
#define	WIFI_CHANNEL_MAX		(13)
#define	WIFI_CHANNEL_SWITCH_INTERVAL	(500)
#define DIM_SSID 32
#define DIM_SEQ 4
#define DIM_ADDR 17

static wifi_country_t wifi_country = {.cc="CN", .schan=1, .nchan=13, .policy=WIFI_COUNTRY_POLICY_AUTO};

typedef struct {
	unsigned version:2;
	unsigned type:2;
	unsigned subtype:4;
	unsigned ToDSFromDS:2;
	unsigned MoreFrag:1;
	unsigned Retry:1;
	unsigned otherFlags:4;
	unsigned duration_id:16;
	uint8_t destination[6]; /* receiver address */
	uint8_t source[6]; /* sender address */
	uint8_t BSSID[6]; /* filtering address */
	unsigned sequence_ctrl:16;
	//uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
	wifi_ieee80211_mac_hdr_t hdr;
	uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

#ifdef __cplusplus
extern "C" {
#endif

static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sniffer_init(void);
static void wifi_sniffer_set_channel(uint8_t channel);
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);


void app_main(void)
{
	uint8_t level = 0, channel = 1;

	/* setup */
	wifi_sniffer_init();
	gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);

	/* loop */
	while (true) {
		gpio_set_level(LED_GPIO_PIN, level ^= 1);
		vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL/portTICK_PERIOD_MS);
		wifi_sniffer_set_channel(channel);
		//channel = (channel % WIFI_CHANNEL_MAX) + 1;
    	}
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void wifi_sniffer_init(void)
{

	nvs_flash_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_sniffer_set_channel(uint8_t channel)
{
	esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{

	switch(type) {
	case WIFI_PKT_MGMT: return "MGMT";
	case WIFI_PKT_DATA: return "DATA";
	default:
	case WIFI_PKT_MISC: return "MISC";
	}
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
	char SSID[DIM_SSID+1]="";   //+1 to include the '\0'
	char source[DIM_ADDR]="";
	char seq[DIM_SEQ]="";

	struct timeval time;

	if (type != WIFI_PKT_MGMT)
		return;

	if(gettimeofday(&time,NULL)==-1){
		//err-cannot retrieve information about time
	}

	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
	const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
	const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

	if((hdr->type==0) && (hdr->subtype==4) && (hdr->ToDSFromDS!=3) && (hdr->Retry==0)){  //filter packet probe request, retry and packet without IPv4

		if(ipkt->payload[0]==0){  //direct probe request
			int len=ipkt->payload[1];  //payload[1] contains the number of bytes used to store SSID
			memcpy(SSID,ipkt->payload+2,len);
			SSID[len]='\0';
		}

		sprintf(source,"%02x:%02x:%02x:%02x:%02x:%02x",hdr->source[0],hdr->source[1],hdr->source[2],hdr->source[3],hdr->source[4],hdr->source[5]);

		sprintf(seq,"%04x",hdr->sequence_ctrl);

		SensorData SD(ppkt->rx_ctrl.channel, ppkt->rx_ctrl.rssi,time, source, seq, SSID);

		SD.printData();

	}
}
#ifdef __cplusplus
}
#endif
