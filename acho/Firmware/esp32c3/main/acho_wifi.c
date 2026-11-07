#include "acho_wifi.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "mdns.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"

static const char *TAG="acho_wifi";
static EventGroupHandle_t events;
static char server[192]=CONFIG_ACHO_SERVER_URL;
static char pop[17];
static char service_name[12];
static esp_timer_handle_t restart_timer;
static portMUX_TYPE server_lock=portMUX_INITIALIZER_UNLOCKED;
#define PROV_ACTIVE BIT1
#define CONFIGURED BIT2
#define RESTARTING BIT3
#define CONNECTED BIT0

void acho_server_url(char *out, size_t capacity) {
    portENTER_CRITICAL(&server_lock);
    strlcpy(out,server,capacity);
    portEXIT_CRITICAL(&server_lock);
}
bool acho_wifi_wait(TickType_t ticks) {
    return (xEventGroupWaitBits(events,CONNECTED,pdFALSE,pdTRUE,ticks)&CONNECTED)!=0;
}
static bool valid_url(const char *url) {
    size_t prefix=!strncmp(url,"http://",7)?7:!strncmp(url,"https://",8)?8:0;
    size_t n=strlen(url);
    if(!prefix || n<=prefix || n>=sizeof(server)) return false;
    for(size_t i=prefix;i<n;i++) {
        if(!(isalnum((unsigned char)url[i]) || strchr(".-:[]",url[i]))) return false;
    }
    return true;
}
/* Send custom-data before applying Wi-Fi credentials; value: {"server_url":"http://host:8787"}. */
static esp_err_t config_endpoint(uint32_t session, const uint8_t *in, ssize_t len,
                                 uint8_t **out, ssize_t *outlen, void *ctx) {
    bool ok=false;
    if(len>0 && len<256 && !acho_wifi_wait(0)) {
        cJSON *root=cJSON_ParseWithLength((const char*)in,len);
        cJSON *url=cJSON_GetObjectItemCaseSensitive(root,"server_url");
        if(cJSON_IsString(url) && valid_url(url->valuestring)) {
            nvs_handle_t nvs;
            if(nvs_open("acho",NVS_READWRITE,&nvs)==ESP_OK) {
                ok=nvs_set_str(nvs,"server",url->valuestring)==ESP_OK && nvs_commit(nvs)==ESP_OK;
                if(ok) {
                    portENTER_CRITICAL(&server_lock);
                    strlcpy(server,url->valuestring,sizeof(server));
                    portEXIT_CRITICAL(&server_lock);
                }
                nvs_close(nvs);
            }
        }
        cJSON_Delete(root);
    }
    *out=(uint8_t*)strdup(ok?"{\"status\":\"successful\"}":"{\"status\":\"failed\"}");
    if(!*out) return ESP_ERR_NO_MEM;
    *outlen=strlen((char*)*out);
    return ESP_OK;
}
static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    if(base==WIFI_PROV_EVENT) {
        if(id==WIFI_PROV_START) {
            xEventGroupSetBits(events,PROV_ACTIVE);
            ESP_LOGI(TAG,"SoftAP provisioning started");
        }
        else if(id==WIFI_PROV_CRED_FAIL) {
            ESP_LOGW(TAG,"Wi-Fi credentials rejected; retry provisioning");
            wifi_prov_mgr_reset_sm_state_on_failure();
        } else if(id==WIFI_PROV_END) {
            xEventGroupClearBits(events,PROV_ACTIVE);
            wifi_prov_mgr_deinit();
            mdns_service_remove("_esp_wifi_prov","_tcp");
        }
    } else if(base==WIFI_EVENT) {
        if(id==WIFI_EVENT_STA_START) esp_wifi_connect();
        else if(id==WIFI_EVENT_STA_DISCONNECTED) {
            xEventGroupClearBits(events,CONNECTED);
            /* Provisioning manager owns retries while testing credentials. */
            if(!(xEventGroupGetBits(events)&PROV_ACTIVE)) esp_wifi_connect();
        }
    } else if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(events,CONNECTED|CONFIGURED);
        ESP_LOGI(TAG,"Wi-Fi connected");
    }
}
void acho_wifi_start(void) {
    esp_err_t err=nvs_flash_init();
    if(err==ESP_ERR_NVS_NO_FREE_PAGES || err==ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err=nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open("acho",NVS_READWRITE,&nvs));
    size_t n=sizeof(server);
    if(nvs_get_str(nvs,"server",server,&n)!=ESP_OK || !valid_url(server))
        strlcpy(server,CONFIG_ACHO_SERVER_URL,sizeof(server));
    n=sizeof(pop);
    if(nvs_get_str(nvs,"pop",pop,&n)!=ESP_OK || strlen(pop)!=16) {
        snprintf(pop,sizeof(pop),"%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random());
        ESP_ERROR_CHECK(nvs_set_str(nvs,"pop",pop));
        ESP_ERROR_CHECK(nvs_commit(nvs));
    }
    uint8_t force_provision=0;
    nvs_get_u8(nvs,"reprov",&force_provision);
    nvs_close(nvs);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    events=xEventGroupCreate();
    if(!events) abort();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT,ESP_EVENT_ANY_ID,event_handler,NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,event_handler,NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,event_handler,NULL));
    if(!esp_netif_create_default_wifi_sta()) abort();
    if(!esp_netif_create_default_wifi_ap()) abort();
    wifi_init_config_t wifi=WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi));
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA,mac));
    char *name=service_name;
    snprintf(name,sizeof(service_name),"ACHO_%02X%02X%02X",mac[3],mac[4],mac[5]);
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(name));
    ESP_ERROR_CHECK(mdns_instance_name_set(name));
    wifi_prov_mgr_config_t config={.scheme=wifi_prov_scheme_softap,
        .scheme_event_handler=WIFI_PROV_EVENT_HANDLER_NONE};
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    if(force_provision) {
        ESP_ERROR_CHECK(wifi_prov_mgr_reset_provisioning());
        ESP_ERROR_CHECK(nvs_open("acho",NVS_READWRITE,&nvs));
        ESP_ERROR_CHECK(nvs_erase_key(nvs,"reprov"));
        ESP_ERROR_CHECK(nvs_commit(nvs));
        nvs_close(nvs);
    }
#if CONFIG_ACHO_PROV_RESET_GPIO >= 0
    gpio_config_t button={.pin_bit_mask=1ULL<<CONFIG_ACHO_PROV_RESET_GPIO,
        .mode=GPIO_MODE_INPUT,.pull_up_en=GPIO_PULLUP_ENABLE};
    ESP_ERROR_CHECK(gpio_config(&button));
    bool reset=true;
    for(int i=0;i<30;i++) {
        if(gpio_get_level(CONFIG_ACHO_PROV_RESET_GPIO)) { reset=false; break; }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if(reset) ESP_ERROR_CHECK(wifi_prov_mgr_reset_provisioning());
#endif
    bool provisioned=false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if(provisioned) {
        xEventGroupSetBits(events,CONFIGURED);
        wifi_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    } else {
        xEventGroupSetBits(events,PROV_ACTIVE);
        ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create("custom-data"));
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1,pop,name,pop));
        ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register("custom-data",config_endpoint,NULL));
        mdns_txt_item_t txt[]={{"name",name},{"transport","softap"}};
        ESP_ERROR_CHECK(mdns_service_add(name,"_esp_wifi_prov","_tcp",80,txt,2));
        ESP_LOGI(TAG,"AP password / PoP: %s",pop);
        ESP_LOGI(TAG,"Provisioning QR payload: {\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"softap\"}",name,pop);
    }
}

void acho_wifi_info(uint8_t out[ACHO_WIFI_INFO_SIZE]) {
    EventBits_t bits=xEventGroupGetBits(events);
    memset(out,0,ACHO_WIFI_INFO_SIZE);
    out[0]=1;
    if(bits&CONNECTED) out[1]|=ACHO_WIFI_CONNECTED;
    if(bits&PROV_ACTIVE) out[1]|=ACHO_WIFI_PROVISIONING;
    if(bits&CONFIGURED) out[1]|=ACHO_WIFI_CONFIGURED;
    if(bits&RESTARTING) out[1]|=ACHO_WIFI_RESTARTING;
    memcpy(out+ACHO_WIFI_SSID_OFFSET,service_name,sizeof(service_name));
    memcpy(out+ACHO_WIFI_POP_OFFSET,pop,sizeof(pop));
}
bool acho_wifi_qr_payload(char *out,size_t capacity) {
    int length=snprintf(out,capacity,"{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"softap\",\"security\":1}",service_name,pop);
    return length>0 && (size_t)length<capacity;
}
esp_err_t acho_wifi_prepare_reprovision(bool *restart_needed) {
    *restart_needed=false;
    EventBits_t bits=xEventGroupGetBits(events);
    if((bits&PROV_ACTIVE) && !(bits&CONFIGURED)) return ESP_OK;
    nvs_handle_t nvs;
    esp_err_t result=nvs_open("acho",NVS_READWRITE,&nvs);
    if(result!=ESP_OK) return result;
    result=nvs_set_u8(nvs,"reprov",1);
    if(result==ESP_OK) result=nvs_commit(nvs);
    nvs_close(nvs);
    if(result==ESP_OK) { xEventGroupSetBits(events,RESTARTING); *restart_needed=true; }
    return result;
}
static void restart_to_provision(void *arg) { esp_restart(); }
void acho_wifi_restart_for_provisioning(void) {
    if(!restart_timer) {
        esp_timer_create_args_t timer={.callback=restart_to_provision,.name="acho_reprov"};
        ESP_ERROR_CHECK(esp_timer_create(&timer,&restart_timer));
    }
    if(!esp_timer_is_active(restart_timer)) ESP_ERROR_CHECK(esp_timer_start_once(restart_timer,500000));
}
