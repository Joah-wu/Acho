#include "acho_wifi.h"
#include "acho_wifi_qr.h"
#include "acho_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "driver/uart.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_websocket_client.h"
#include "freertos/queue.h"

#define LINK UART_NUM_0
static const char *TAG="acho_bridge";
static acho_parser_t parser;
static acho_frame_t rx, tx;
static uint8_t wire[ACHO_HEADER+ACHO_CHUNK];
typedef struct { int kind, len; uint8_t data[ACHO_CHUNK+1]; } ws_message_t;
static QueueHandle_t messages;
static ws_message_t incoming, message;
static size_t message_used;
static bool fragmented;

static bool serial_receive(acho_frame_t *frame, TickType_t timeout) {
    TickType_t start=xTaskGetTickCount();
    do {
        uint8_t byte;
        if(uart_read_bytes(LINK,&byte,1,pdMS_TO_TICKS(10))==1 && acho_feed(&parser,byte,frame)) return true;
    } while(xTaskGetTickCount()-start<timeout);
    return false;
}
static void serial_send(const acho_frame_t *frame) {
    size_t len=acho_encode(wire,frame);
    uart_write_bytes(LINK,wire,len);
}
static bool serial_confirm(void) {
    for(int attempt=0;attempt<3;attempt++) {
        serial_send(&tx);
        TickType_t start=xTaskGetTickCount();
        while(xTaskGetTickCount()-start<pdMS_TO_TICKS(2000)) {
            if(serial_receive(&rx,pdMS_TO_TICKS(50))) {
                if(rx.id==tx.id && rx.seq==tx.seq && rx.type==ACHO_ACK) return true;
                if(rx.id==tx.id && rx.type==ACHO_ERROR) return false;
            }
        }
    }
    return false;
}
/* The callback only reassembles messages. UART/SD backpressure is handled by
 * the worker; never block WebSocket's event task waiting for a serial ACK. */
static void ws_event(void *arg, esp_event_base_t base, int32_t id, void *raw) {
    esp_websocket_event_data_t *event=raw;
    if(id==WEBSOCKET_EVENT_CONNECTED) {
        ws_message_t state={.kind=3};
        xQueueSend(messages,&state,0);
    } else if(id==WEBSOCKET_EVENT_DISCONNECTED || id==WEBSOCKET_EVENT_ERROR) {
        ws_message_t state={.kind=-1};
        xQueueSend(messages,&state,0);
    } else if(id==WEBSOCKET_EVENT_DATA) {
        if(event->op_code>=8) return; /* ping/pong/close */
        if(event->payload_offset==0 && event->op_code!=0) {
            if(fragmented) goto invalid;
            incoming.kind=event->op_code;
            message_used=0;
        }
        if(event->data_len<0 || message_used+(size_t)event->data_len>ACHO_CHUNK) goto invalid;
        memcpy(incoming.data+message_used,event->data_ptr,event->data_len);
        message_used+=event->data_len;
        if(event->payload_offset+event->data_len==event->payload_len) {
            fragmented=!event->fin;
            if(event->fin) {
                incoming.len=message_used;
                incoming.data[message_used]=0;
                if(xQueueSend(messages,&incoming,0)!=pdTRUE) goto invalid;
                message_used=0;
            }
        }
        return;
invalid:;
        ws_message_t state={.kind=-1};
        xQueueOverwrite(messages,&state);
        message_used=0;
        fragmented=false;
    }
}
static bool ws_receive(void) {
    return xQueueReceive(messages,&message,pdMS_TO_TICKS(15000))==pdTRUE && message.kind>0;
}
static bool ws_send_json(esp_websocket_client_handle_t ws, cJSON *root) {
    char *data=cJSON_PrintUnformatted(root);
    if(!data) return false;
    int len=strlen(data);
    bool ok=esp_websocket_client_send_text(ws,data,len,pdMS_TO_TICKS(5000))==len;
    free(data);
    return ok;
}
/* Register anew for each transfer: tokens expire after 10h on the existing
 * server, and the device's IP may change after a Wi-Fi reconnect. */
static bool get_token(const char *server, char *token, size_t capacity) {
    uint8_t mac[6];
    esp_read_mac(mac,ESP_MAC_WIFI_STA);
    esp_netif_ip_info_t ip;
    esp_netif_t *netif=esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if(!netif || esp_netif_get_ip_info(netif,&ip)!=ESP_OK) return false;
    char url[384];
    snprintf(url,sizeof(url),"%s/home/device/init/?mac=%02X:%02X:%02X:%02X:%02X:%02X&ip=" IPSTR "&type=Acho",
        server,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],IP2STR(&ip.ip));
    esp_http_client_config_t config={.url=url,.timeout_ms=10000,
        .crt_bundle_attach=esp_crt_bundle_attach,.disable_auto_redirect=true};
    esp_http_client_handle_t http=esp_http_client_init(&config);
    if(!http) return false;
    bool ok=false;
    char response[1024];
    if(esp_http_client_open(http,0)==ESP_OK) {
        esp_http_client_fetch_headers(http);
        if(esp_http_client_get_status_code(http)==200) {
            int n=esp_http_client_read_response(http,response,sizeof(response)-1);
            if(n>0 && n<(int)sizeof(response)-1) {
                response[n]=0;
                cJSON *root=cJSON_Parse(response);
                cJSON *status=cJSON_GetObjectItem(root,"status");
                cJSON *tok=cJSON_GetObjectItem(root,"token");
                if(cJSON_IsString(status) && !strcmp(status->valuestring,"successful") &&
                   cJSON_IsString(tok) && strlen(tok->valuestring)<capacity) {
                    strlcpy(token,tok->valuestring,capacity);
                    ok=true;
                }
                cJSON_Delete(root);
            }
        }
    }
    esp_http_client_close(http);
    esp_http_client_cleanup(http);
    return ok;
}
static bool json_uint(cJSON *root, const char *key, uint32_t *value) {
    cJSON *item=cJSON_GetObjectItemCaseSensitive(root,key);
    if(!cJSON_IsNumber(item) || item->valuedouble<0 || item->valuedouble>UINT32_MAX ||
       item->valuedouble!=(double)(uint32_t)item->valuedouble) return false;
    *value=(uint32_t)item->valuedouble;
    return true;
}
static bool run_transfer(cJSON *request) {
    if(!acho_wifi_wait(pdMS_TO_TICKS(30000))) return false;
    char token[512];
    char server[192];
    acho_server_url(server,sizeof(server));
    if(!get_token(server,token,sizeof(token))) return false;
    cJSON_DeleteItemFromObjectCaseSensitive(request,"token");
    cJSON_AddStringToObject(request,"token",token);
    char url[256];
    const char *base=server;
    bool tls=!strncmp(base,"https://",8);
    snprintf(url,sizeof(url),"%s%s/broker/ws/files/",tls?"wss://":"ws://",base+(tls?8:7));
    esp_websocket_client_config_t config={.uri=url,.buffer_size=512,.task_stack=6144,
        .disable_auto_reconnect=true,.network_timeout_ms=10000,.crt_bundle_attach=esp_crt_bundle_attach};
    esp_websocket_client_handle_t ws=esp_websocket_client_init(&config);
    if(!ws) return false;
    bool ok=false;
    uint32_t size=0, offset=0, crc=0;
    xQueueReset(messages);
    message_used=0; fragmented=false;
    esp_websocket_register_events(ws,WEBSOCKET_EVENT_ANY,ws_event,NULL);
    if(esp_websocket_client_start(ws)!=ESP_OK || !ws_receive() || message.kind!=3) goto done;
    if(!ws_send_json(ws,request) || !ws_receive() || message.kind!=1) goto done;
    cJSON *root=cJSON_Parse((char*)message.data);
    cJSON *event=cJSON_GetObjectItem(root,"event");
    bool begin=cJSON_IsString(event) && !strcmp(event->valuestring,"begin") &&
        json_uint(root,"size",&size) && size<=ACHO_MAX_FILE;
    cJSON_Delete(root);
    if(!begin) goto done;
    tx.type=ACHO_BEGIN; tx.seq=0; tx.len=4;
    acho_put32(tx.data,size);
    if(!serial_confirm()) goto done;
    for(;;) {
        cJSON *next=cJSON_CreateObject();
        cJSON_AddStringToObject(next,"op","next");
        cJSON_AddNumberToObject(next,"offset",offset);
        bool sent=ws_send_json(ws,next);
        cJSON_Delete(next);
        if(!sent || !ws_receive()) goto done;
        if(message.kind==2) {
            if(message.len<=0 || (uint32_t)message.len>size-offset) goto done;
            tx.type=ACHO_DATA; tx.seq++; tx.len=message.len;
            memcpy(tx.data,message.data,message.len);
            if(!serial_confirm()) goto done;
            offset+=message.len;
            crc=acho_crc32(crc,message.data,message.len);
        } else if(message.kind==1) {
            root=cJSON_Parse((char*)message.data);
            event=cJSON_GetObjectItem(root,"event");
            uint32_t remote_crc,remote_size;
            bool end=cJSON_IsString(event) && !strcmp(event->valuestring,"end") && offset==size &&
                json_uint(root,"crc32",&remote_crc) && remote_crc==crc &&
                json_uint(root,"size",&remote_size) && remote_size==size;
            cJSON_Delete(root);
            if(!end) goto done;
            tx.type=ACHO_END; tx.seq++; tx.len=8;
            acho_put32(tx.data,size); acho_put32(tx.data+4,crc);
            ok=serial_confirm();
            break;
        } else goto done;
    }
done:
    esp_websocket_client_stop(ws);
    esp_websocket_client_destroy(ws);
    return ok;
}
static bool serial_blob(const uint8_t *data,size_t length) {
    tx.type=ACHO_BEGIN; tx.seq=0; tx.len=4;
    acho_put32(tx.data,length);
    if(!serial_confirm()) return false;
    size_t offset=0;
    while(offset<length) {
        size_t count=length-offset;
        if(count>ACHO_CHUNK) count=ACHO_CHUNK;
        tx.type=ACHO_DATA; tx.seq++; tx.len=count;
        memcpy(tx.data,data+offset,count);
        if(!serial_confirm()) return false;
        offset+=count;
    }
    tx.type=ACHO_END; tx.seq++; tx.len=8;
    acho_put32(tx.data,length); acho_put32(tx.data+4,acho_crc32(0,data,length));
    return serial_confirm();
}
static bool local_request(const char *operation) {
    static uint8_t image[ACHO_WIFI_QR_SIZE];
    if(!strcmp(operation,"get_wifi_qr")) {
        char payload[160];
        return acho_wifi_qr_payload(payload,sizeof(payload)) &&
            acho_wifi_qr_make(payload,image,sizeof(image)) && serial_blob(image,sizeof(image));
    }
    bool restart=false;
    if(!strcmp(operation,"start_wifi_provisioning") && acho_wifi_prepare_reprovision(&restart)!=ESP_OK) return false;
    uint8_t info[ACHO_WIFI_INFO_SIZE];
    acho_wifi_info(info);
    bool sent=serial_blob(info,sizeof(info));
    if(restart) acho_wifi_restart_for_provisioning();
    return sent;
}
static void bridge_task(void *arg) {
    for(;;) {
        if(!serial_receive(&rx,pdMS_TO_TICKS(100))) continue;
        if(rx.type!=ACHO_REQUEST || rx.len==0 || rx.len>=ACHO_CHUNK) continue;
        tx.id=rx.id; tx.seq=0;
        cJSON *request=cJSON_ParseWithLength((char*)rx.data,rx.len);
        cJSON *operation=cJSON_GetObjectItemCaseSensitive(request,"op");
        bool local=cJSON_IsString(operation) && (!strcmp(operation->valuestring,"get_wifi_info") ||
            !strcmp(operation->valuestring,"get_wifi_qr") || !strcmp(operation->valuestring,"start_wifi_provisioning"));
        bool ok=local?local_request(operation->valuestring):(cJSON_IsObject(request) && run_transfer(request));
        cJSON_Delete(request);
        if(!ok) {
            ESP_LOGW(TAG,"Transfer %u failed",tx.id);
            tx.type=ACHO_ERROR; tx.len=0;
            serial_send(&tx);
        }
    }
}
void acho_bridge_start(void) {
    uart_config_t uart={.baud_rate=115200,.data_bits=UART_DATA_8_BITS,
        .parity=UART_PARITY_DISABLE,.stop_bits=UART_STOP_BITS_1,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE,.source_clk=UART_SCLK_DEFAULT};
    ESP_ERROR_CHECK(uart_driver_install(LINK,4096,0,0,NULL,0));
    ESP_ERROR_CHECK(uart_param_config(LINK,&uart));
    ESP_ERROR_CHECK(uart_set_pin(LINK,CONFIG_ACHO_UART_TX,CONFIG_ACHO_UART_RX,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
    messages=xQueueCreate(1,sizeof(ws_message_t));
    if(!messages) abort();
    if(xTaskCreate(bridge_task,"acho_bridge",10240,NULL,5,NULL)!=pdPASS) abort();
}
