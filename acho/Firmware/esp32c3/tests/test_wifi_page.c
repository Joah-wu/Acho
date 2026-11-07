/* Tests the real page controller with checked widget doubles and serial replies. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define __BSP_USART_H__
typedef enum { HAL_OK,HAL_ERROR,HAL_BUSY,HAL_TIMEOUT } HAL_StatusTypeDef;
typedef void (*UartHandle_FinishCallBackFuncTypedef)(HAL_StatusTypeDef,void*);
#include "../../stm32f446/User/gui/pages/page_wifi.c"

static unsigned nfc_published,nfc_cleared;
static TransferStatus link_diag;
void Transfer_GetStatus(TransferStatus *out) { *out=link_diag; }
static bool prompt_exists=true;
static NfcStatus nfc_status={.state=NFC_READY,.owner=NFC_OWNER_WIFI};
uint32_t NfcService_PublishWifi(const uint8_t data[ACHO_WIFI_INFO_SIZE]) {
    assert(acho_wifi_info_valid(data,ACHO_WIFI_INFO_SIZE));
    if(!(data[1]&ACHO_WIFI_PROVISIONING) || (data[1]&ACHO_WIFI_CONNECTED)) return 0;
    nfc_published++;nfc_status.published_revision=nfc_published;return nfc_published;
}
void NfcService_Clear(uint8_t owner) { assert(owner==NFC_OWNER_WIFI);nfc_cleared++; }
void NfcService_Status(NfcStatus *out) { *out=nfc_status; }
const int lv_font_montserrat_10=10;
static lv_obj_t objects[64], main_screen;
static unsigned object_count;
static lv_obj_t *active;
static uint32_t clock_ticks=1;
static lv_timer_t fake_timer;
lv_ui ui_main={.screen=&main_screen,.is_init=true};
static struct {
    const char *json; uint8_t *data; size_t capacity,*length;
    UartHandle_FinishCallBackFuncTypedef callback; void *context;
} queued;
uint32_t lv_tick_get(void) { return clock_ticks; }
uint32_t lv_tick_elaps(uint32_t tick) { return clock_ticks-tick; }
lv_obj_t *lv_scr_act(void) { return active; }
static void alive(lv_obj_t *object) { assert(object && !object->deleted); }
lv_obj_t *lv_obj_create(lv_obj_t *parent) {
    assert(object_count<64);lv_obj_t *object=&objects[object_count++];memset(object,0,sizeof(*object));object->parent=parent;return object;
}
lv_obj_t *lv_label_create(lv_obj_t *parent) { return lv_obj_create(parent); }
lv_obj_t *lv_img_create(lv_obj_t *parent) { return lv_obj_create(parent); }
void lv_obj_add_flag(lv_obj_t *object,int flag) { alive(object);object->flags|=flag; }
void lv_obj_clear_flag(lv_obj_t *object,int flag) { alive(object);object->flags&=~flag; }
void lv_label_set_text(lv_obj_t *object,const char *text) { alive(object);assert(strlen(text)<sizeof(object->text));strcpy(object->text,text); }
void lv_obj_set_pos(lv_obj_t *object,int x,int y) { alive(object);object->x=x;object->y=y; }
void lv_obj_set_width(lv_obj_t *object,int width) { alive(object);object->w=width; }
void lv_img_set_src(lv_obj_t *object,const void *source) { alive(object);object->source=source; }
lv_res_t lv_img_decoder_get_info(const void *source,lv_img_header_t *header) {
    assert(!strcmp(source,"P:Static/nfc_wifi.bin"));
    if(!prompt_exists) return LV_RES_INV;
    *header=(lv_img_header_t){.cf=LV_IMG_CF_INDEXED_1BIT,.w=104,.h=65};return LV_RES_OK;
}
void lv_img_cache_invalidate_src(const void *source) { (void)source; }
lv_timer_t *lv_timer_create(void(*callback)(lv_timer_t*),uint32_t period,void *data) {
    (void)period;(void)data;fake_timer.callback=callback;return &fake_timer;
}
void setup_ui(lv_ui *page,lv_ui *last,void *data) {
    page->last_ui=last;page->gui_setup_func(data);page->is_init=true;active=page->screen;
}
void delete_ui(lv_ui *page,void *data) {
    if(page->gui_delete_func) page->gui_delete_func(data);
    for(unsigned i=0;i<object_count;i++)
        if(&objects[i]==page->screen || objects[i].parent==page->screen) objects[i].deleted=true;
    page->screen=NULL;page->is_init=false;active=page->last_ui->screen;
}
int Transfer_SubmitMemory(const char *json,uint8_t *buffer,size_t capacity,size_t *length,
                          UartHandle_FinishCallBackFuncTypedef callback,void *context) {
    assert(!queued.callback);
    queued.json=json;queued.data=buffer;queued.capacity=capacity;queued.length=length;
    queued.callback=callback;queued.context=context;return 1;
}
static void reply_info(uint8_t flags) {
    assert(queued.callback && queued.capacity==ACHO_WIFI_INFO_SIZE);
    uint8_t data[ACHO_WIFI_INFO_SIZE]={1,flags};
    memcpy(data+2,"ACHO_AABBCC",12);memcpy(data+14,"0123456789abcdef",17);
    memcpy(queued.data,data,sizeof(data));*queued.length=sizeof(data);
    UartHandle_FinishCallBackFuncTypedef callback=queued.callback;void *context=queued.context;
    memset(&queued,0,sizeof(queued));callback(HAL_OK,context);
}
static void tick(void) { clock_ticks+=2100;fake_timer.callback(&fake_timer); }
static void reply_qr(void) {
    assert(queued.callback && queued.capacity==ACHO_WIFI_QR_SIZE);
    FILE *file=fopen("/tmp/acho-wifi-qr.bin","rb");assert(file);
    assert(fread(queued.data,1,ACHO_WIFI_QR_SIZE,file)==ACHO_WIFI_QR_SIZE);fclose(file);
    *queued.length=ACHO_WIFI_QR_SIZE;
    UartHandle_FinishCallBackFuncTypedef callback=queued.callback;void *context=queued.context;
    memset(&queued,0,sizeof(queued));callback(HAL_OK,context);
}
static void event(int code) { lv_event_t e={.code=code};click(&e); }
int main(void) {
    active=&main_screen;
    WifiPage_Init();assert(queued.callback);
    reply_info(ACHO_WIFI_PROVISIONING);
    assert(ui_wifi.is_init && active==ui_wifi.screen); /* First boot opens Wi-Fi. */
    if(queued.callback) reply_info(ACHO_WIFI_PROVISIONING);
    tick();assert(queued.callback && queued.capacity==ACHO_WIFI_QR_SIZE);
    FILE *file=fopen("/tmp/acho-wifi-qr.bin","rb");assert(file);
    assert(fread(queued.data,1,ACHO_WIFI_QR_SIZE,file)==ACHO_WIFI_QR_SIZE);fclose(file);
    *queued.length=ACHO_WIFI_QR_SIZE;
    UartHandle_FinishCallBackFuncTypedef callback=queued.callback;void *context=queued.context;
    memset(&queued,0,sizeof(queued));callback(HAL_OK,context);
    assert(!(qr_image->flags&LV_OBJ_FLAG_HIDDEN));
    assert(qr_image->x==10 && qr_image->y==10 && footer->y==118);
    event(LV_EVENT_SHORT_CLICKED);assert(strstr(body->text,"0123456789abcdef"));
    event(LV_EVENT_SHORT_CLICKED);assert(!(qr_image->flags&LV_OBJ_FLAG_HIDDEN));
    tick();assert(queued.callback);
    event(LV_EVENT_LONG_PRESSED);assert(!ui_wifi.is_init);
    reply_info(ACHO_WIFI_PROVISIONING); /* Late callback cannot touch deleted widgets. */
    assert(active==&main_screen);
    auto_probe=true;last_query=0;tick();reply_info(ACHO_WIFI_CONFIGURED|ACHO_WIFI_CONNECTED);
    assert(!ui_wifi.is_init); /* Saved Wi-Fi does not steal the main screen. */
    setup_ui(&ui_wifi,&ui_main,NULL);
    if(queued.callback) reply_info(ACHO_WIFI_CONFIGURED|ACHO_WIFI_CONNECTED);
    event(LV_EVENT_SHORT_CLICKED);assert(view==VIEW_CONFIRM);
    event(LV_EVENT_LONG_PRESSED);assert(view==VIEW_NORMAL && !start_requested);
    event(LV_EVENT_SHORT_CLICKED);event(LV_EVENT_SHORT_CLICKED);
    assert(queued.callback && strstr(queued.json,"start_wifi_provisioning"));
    reply_info(ACHO_WIFI_CONFIGURED|ACHO_WIFI_CONNECTED|ACHO_WIFI_RESTARTING);
    assert(strstr(body->text,"Restarting"));
    tick();reply_info(ACHO_WIFI_PROVISIONING);
    assert(entered_provisioning && !(qr_image->flags&LV_OBJ_FLAG_HIDDEN));
    tick();reply_info(ACHO_WIFI_CONNECTED|ACHO_WIFI_CONFIGURED);
    assert(strstr(body->text,"Setup complete"));
    event(LV_EVENT_SHORT_CLICKED);assert(active==&main_screen);
    /* A user who manually leaves setup before the first reply stays on main. */
    auto_probe=true;info_valid=false;last_query=0;tick();
    setup_ui(&ui_wifi,&ui_main,NULL);event(LV_EVENT_LONG_PRESSED);
    reply_info(ACHO_WIFI_PROVISIONING);
    assert(active==&main_screen && !ui_wifi.is_init);
    /* NFC entry uses the illustration without requesting a QR, then allows QR/info fallback. */
    WifiPage_OpenNfc(&ui_main);
    assert(nfc_entry && view==VIEW_NFC && !(prompt_image->flags&LV_OBJ_FLAG_HIDDEN));
    assert(!strcmp(prompt_image->source,"P:Static/nfc_wifi.bin") && prompt_image->x==12 && prompt_image->y==24);
    assert(body->y==104 && (qr_image->flags&LV_OBJ_FLAG_HIDDEN));
    assert(queued.callback && strstr(queued.json,"get_wifi_info"));
    nfc_status.state=NFC_BUSY;poll(NULL);
    assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"Move phone")); // Updates even while UART is pending.
    nfc_status.state=NFC_ERROR;poll(NULL);
    assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"NFC not ready"));
    nfc_status.state=NFC_READY;nfc_status.published_revision=0;poll(NULL);assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && !strstr(body->text,"Ready"));
    nfc_status.published_revision=nfc_revision;poll(NULL);assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN));
    link_error=true;poll(NULL);
    assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"No UART response"));
    link_diag=(TransferStatus){.stage=TRANSFER_WAIT_BEGIN,.tx_status=HAL_ERROR};poll(NULL);
    assert(strstr(body->text,"UART TX failed"));
    link_diag=(TransferStatus){.stage=TRANSFER_WAIT_BEGIN,.rx_bytes=17};poll(NULL);
    assert(strstr(body->text,"Invalid UART frames"));
    link_diag=(TransferStatus){.stage=TRANSFER_WAIT_DATA,.rx_bytes=21,.rx_frames=1};poll(NULL);
    assert(strstr(body->text,"Transfer incomplete"));
    link_diag.uart_errors=1;poll(NULL);assert(strstr(body->text,"UART RX error"));
    link_diag=(TransferStatus){.stage=TRANSFER_DONE};poll(NULL);assert(strstr(body->text,"Invalid payload"));
    link_diag=(TransferStatus){.stage=TRANSFER_WAIT_END,.rx_bytes=79,.rx_frames=2,.uart_errors=1};
    prompt_exists=false;prompt_state=PROMPT_UNCHECKED;poll(NULL);
    assert((prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"Wait END") && strstr(body->text,"F=2 E=1"));
    prompt_exists=true;prompt_state=PROMPT_UNCHECKED;poll(NULL);
    info_valid=false;link_error=false;poll(NULL);
    assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"Reading Wi-Fi"));
    reply_info(ACHO_WIFI_PROVISIONING);
    event(LV_EVENT_SHORT_CLICKED);assert(view==VIEW_NORMAL && (prompt_image->flags&LV_OBJ_FLAG_HIDDEN));reply_qr();assert(!(qr_image->flags&LV_OBJ_FLAG_HIDDEN));
    event(LV_EVENT_SHORT_CLICKED);assert(view==VIEW_DETAILS && strstr(body->text,"0123456789abcdef"));
    event(LV_EVENT_SHORT_CLICKED);assert(view==VIEW_NFC && !(prompt_image->flags&LV_OBJ_FLAG_HIDDEN));
    event(LV_EVENT_LONG_PRESSED);
    prompt_exists=false;WifiPage_OpenNfc(&ui_main);
    assert((prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"Ready: tap"));
    if(queued.callback) reply_info(ACHO_WIFI_PROVISIONING);
    event(LV_EVENT_LONG_PRESSED);
    setup_ui(&ui_wifi,&ui_main,NULL);assert(!nfc_entry && view==VIEW_NORMAL); // Normal entry stays QR-first.
    if(queued.callback) reply_info(ACHO_WIFI_PROVISIONING);
    event(LV_EVENT_LONG_PRESSED);
    info[1]=ACHO_WIFI_CONNECTED|ACHO_WIFI_CONFIGURED;
    WifiPage_OpenNfc(&ui_main);
    if(queued.callback) reply_info(ACHO_WIFI_CONNECTED|ACHO_WIFI_CONFIGURED);
    prompt_exists=true;prompt_state=PROMPT_UNCHECKED;render();
    assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"Wi-Fi connected"));
    event(LV_EVENT_SHORT_CLICKED);assert(view==VIEW_CONFIRM && (prompt_image->flags&LV_OBJ_FLAG_HIDDEN));
    event(LV_EVENT_LONG_PRESSED);assert(view==VIEW_NFC && !(prompt_image->flags&LV_OBJ_FLAG_HIDDEN));
    event(LV_EVENT_LONG_PRESSED);
    assert(nfc_published && nfc_cleared);
    puts("Wi-Fi page: NFC SD illustration, revision guard, RF busy, missing-image fallback, QR/info cycle, normal entry, reconfigure, first boot and late callbacks passed");
}
