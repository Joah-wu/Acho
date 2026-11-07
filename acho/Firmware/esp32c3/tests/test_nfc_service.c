#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../../stm32f446/User/nfc_service.c"
static NfcJob queued;
static int has_job,step,rf,detected,write_failed,writes;
static uint8_t memory[ACHO_NFC_MEMORY],wifi[ACHO_WIFI_INFO_SIZE]={1,ACHO_WIFI_PROVISIONING};
static uint32_t clock_ticks=100,revision;
static jmp_buf done;
uint32_t HAL_GetTick(void) {return clock_ticks;}
void vTaskDelay(TickType_t ticks) {clock_ticks+=ticks;}
QueueHandle_t xQueueCreateStatic(unsigned length,size_t size,uint8_t *buffer,StaticQueue_t *control) {
    (void)buffer;assert(length==1 && size==sizeof(NfcJob));return control;
}
TaskHandle_t xTaskCreateStatic(void(*fn)(void*),const char *name,uint32_t stack,void *arg,unsigned priority,StackType_t *buffer,StaticTask_t *control) {
    (void)name;(void)stack;(void)arg;(void)priority;(void)buffer;assert(fn==worker);return control;
}
int xQueueOverwrite(QueueHandle_t queue,const void *job) {assert(queue==jobs);queued=*(const NfcJob*)job;has_job=1;return 1;}
int NfcTag_Init(NfcTagInfo *out) {memset(out,0,sizeof(*out));out->memory_bytes=512;strcpy(out->uid,"E002123456789ABC");return detected?0:-1;}
int NfcTag_FieldPresent(void) {return rf;}
int NfcTag_Publish(const uint8_t image[ACHO_NFC_MEMORY]) {writes++;if(write_failed) return -1;memcpy(memory,image,sizeof(memory));return 0;}
static void expected_payload(const char *text) {
    uint8_t expected[ACHO_NFC_MEMORY];assert(acho_ndef_encode(expected,sizeof(expected),(const uint8_t*)text,strlen(text),0));
    assert(!memcmp(expected,memory,sizeof(expected)));
}
int xQueueReceive(QueueHandle_t queue,void *job,TickType_t ticks) {
    assert(queue==jobs);clock_ticks+=ticks;step++;
    switch(step) {
    case 1: break; // Probe missing hardware.
    case 2: assert(status.state==NFC_UNAVAILABLE);clock_ticks+=2100;detected=1;rf=1;break;
    case 3: assert(status.state==NFC_BUSY && writes==0);rf=0;break;
    case 4:
        assert(status.state==NFC_READY && status.owner==NFC_OWNER_NONE);expected_payload(ACHO_NFC_IDLE);
        revision=NfcService_PublishWifi(wifi);assert(revision);
        assert(NfcService_PublishWifi(wifi)==revision); // Repeated polling does not queue more writes.
        rf=1;break;
    case 5: assert(status.state==NFC_BUSY && writes==1);rf=0;break;
    case 6:
        assert(status.state==NFC_READY && status.owner==NFC_OWNER_WIFI && writes==2);
        NfcService_Clear(NFC_OWNER_PAIR);assert(!has_job); // Foreign page cannot clear Wi-Fi payload.
        revision=NfcService_Publish("head.payload.signature",22,NFC_OWNER_PAIR,1000);assert(revision);break;
    case 7:
        assert(status.state==NFC_READY && status.owner==NFC_OWNER_PAIR);expected_payload("head.payload.signature");
        clock_ticks+=1001;break;
    case 8:
        assert(status.state==NFC_EXPIRED && status.owner==NFC_OWNER_NONE);expected_payload(ACHO_NFC_IDLE);
        assert(!NfcService_Publish("x",1,NFC_OWNER_PAIR,60001));
        NfcService_PublishWifi(wifi);write_failed=1;break;
    case 9: assert(status.state==NFC_ERROR);write_failed=0;break;
    case 10:
        assert(status.state==NFC_READY && status.owner==NFC_OWNER_WIFI);
        NfcService_Clear(NFC_OWNER_WIFI);break;
    case 11: expected_payload(ACHO_NFC_IDLE);assert(status.owner==NFC_OWNER_NONE);longjmp(done,1);
    }
    if(has_job) {memcpy(job,&queued,sizeof(queued));has_job=0;return pdTRUE;}
    return 0;
}
int main(void) {
    memcpy(wifi+2,"ACHO_AABBCC",12);memcpy(wifi+14,"0123456789abcdef",17);
    NfcService_Init();
    if(!setjmp(done)) worker(NULL);
    puts("NFC service: chip retry, RF busy deferral, Wi-Fi deduplication, owner isolation, token expiry erasure and write retry passed");
}
