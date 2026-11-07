/* ST25DV is owned by a low-priority worker; GUI/audio never wait for EEPROM. */
#include "nfc_service.h"
#include "nfc_tag.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    char payload[ACHO_NFC_MAX_PAYLOAD+1];
    size_t length;
    uint32_t revision,expires_at;
    uint8_t owner;
} NfcJob;
static StaticQueue_t queue_control;
static uint8_t queue_memory[sizeof(NfcJob)];
static QueueHandle_t jobs;
static StaticTask_t worker_control;
static StackType_t worker_stack[768];
static NfcJob requested;
static NfcStatus status;
static uint8_t image[ACHO_NFC_MEMORY];

void NfcService_Status(NfcStatus *out) {
    taskENTER_CRITICAL();*out=status;taskEXIT_CRITICAL();
}
static void update(uint8_t state,uint8_t owner,uint32_t published,uint32_t expires,const char *uid) {
    taskENTER_CRITICAL();
    status.state=state;status.owner=owner;status.published_revision=published;status.expires_at=expires;
    if(uid) memcpy(status.uid,uid,sizeof(status.uid));
    taskEXIT_CRITICAL();
}
uint32_t NfcService_Publish(const char *payload,size_t length,uint8_t owner,uint32_t valid_ms) {
    if(!jobs || !payload || !length || length>ACHO_NFC_MAX_PAYLOAD || valid_ms>60000) return 0;
    taskENTER_CRITICAL();
    if(requested.revision && !valid_ms && requested.owner==owner && requested.length==length && !memcmp(requested.payload,payload,length)) {
        uint32_t revision=requested.revision;taskEXIT_CRITICAL();return revision;
    }
    requested.revision++;if(!requested.revision) requested.revision++;
    memcpy(requested.payload,payload,length);requested.payload[length]=0;
    requested.length=length;requested.owner=owner;requested.expires_at=valid_ms?HAL_GetTick()+valid_ms:0;
    if(valid_ms && !requested.expires_at) requested.expires_at=1;
    status.requested_revision=requested.revision;
    xQueueOverwrite(jobs,&requested);
    uint32_t revision=requested.revision;
    taskEXIT_CRITICAL();return revision;
}
void NfcService_Clear(uint8_t owner) {
    taskENTER_CRITICAL();
    int clear=requested.owner==owner;
    taskEXIT_CRITICAL();
    if(clear) NfcService_Publish(ACHO_NFC_IDLE,strlen(ACHO_NFC_IDLE),NFC_OWNER_NONE,0);
}
uint32_t NfcService_PublishWifi(const uint8_t info[ACHO_WIFI_INFO_SIZE]) {
    if(!acho_wifi_info_valid(info,ACHO_WIFI_INFO_SIZE) || !(info[1]&ACHO_WIFI_PROVISIONING) || (info[1]&ACHO_WIFI_CONNECTED)) return 0;
    char text[160];
    int size=snprintf(text,sizeof(text),"{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"softap\",\"security\":1}",
        (const char*)info+ACHO_WIFI_SSID_OFFSET,(const char*)info+ACHO_WIFI_POP_OFFSET);
    return size>0 && (size_t)size<sizeof(text)?NfcService_Publish(text,size,NFC_OWNER_WIFI,0):0;
}
static void worker(void *argument) {
    (void)argument;
    NfcJob active={0};NfcTagInfo tag_info={0};
    int present=0,dirty=0,expired=0;
    uint32_t next_probe=0;
    for(;;) {
        if(xQueueReceive(jobs,&active,pdMS_TO_TICKS(100))==pdTRUE) { dirty=1;expired=0; }
        uint32_t now=HAL_GetTick();
        if(active.expires_at && (int32_t)(now-active.expires_at)>=0) {
            strcpy(active.payload,ACHO_NFC_IDLE);active.length=strlen(ACHO_NFC_IDLE);
            active.owner=NFC_OWNER_NONE;active.expires_at=0;dirty=1;expired=1;
        }
        if(!present) {
            if((int32_t)(now-next_probe)<0) continue;
            next_probe=now+2000;
            present=NfcTag_Init(&tag_info)==0;
            update(present?NFC_PREPARING:NFC_UNAVAILABLE,active.owner,0,active.expires_at,present?tag_info.uid:NULL);
            if(!present) continue;
            dirty=1;
        }
        if(!dirty || !active.length) continue;
        int field=NfcTag_FieldPresent();
        if(field>0) { update(NFC_BUSY,active.owner,0,active.expires_at,NULL);continue; }
        if(field<0) { present=0;update(NFC_ERROR,active.owner,0,active.expires_at,NULL);continue; }
        update(NFC_PREPARING,active.owner,0,active.expires_at,NULL);
        size_t size=acho_ndef_encode(image,sizeof(image),(const uint8_t*)active.payload,active.length,tag_info.memory_bytes>512);
        if(!size || NfcTag_Publish(image)) {
            update(NFC_ERROR,active.owner,0,active.expires_at,NULL);
            vTaskDelay(pdMS_TO_TICKS(300));continue;
        }
        dirty=0;
        update(expired?NFC_EXPIRED:NFC_READY,active.owner,active.revision,active.expires_at,NULL);
    }
}
void NfcService_Init(void) {
    jobs=xQueueCreateStatic(1,sizeof(NfcJob),queue_memory,&queue_control);
    configASSERT(jobs);
    NfcService_Publish(ACHO_NFC_IDLE,strlen(ACHO_NFC_IDLE),NFC_OWNER_NONE,0);
    TaskHandle_t task=xTaskCreateStatic(worker,"nfc",sizeof(worker_stack)/sizeof(worker_stack[0]),NULL,
        tskIDLE_PRIORITY+2,worker_stack,&worker_control);
    configASSERT(task);
}
