/* Exercise the real NFC page and its asynchronous pairing lifecycle. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define __BSP_USART_H__
typedef enum { HAL_OK,HAL_ERROR,HAL_BUSY,HAL_TIMEOUT } HAL_StatusTypeDef;
typedef void (*UartHandle_FinishCallBackFuncTypedef)(HAL_StatusTypeDef,void*);
uint32_t HAL_GetTick(void);
#include "../../stm32f446/User/gui/pages/page_nfc.c"
const int lv_font_montserrat_10=10;
static lv_obj_t objects[64],main_screen;
static lv_timer_t fake_timer;
static lv_obj_t *active;
static unsigned objects_used,clears,publishes,refreshes;
static unsigned wifi_nfc_entries;
static bool prompt_exists=true;
static uint32_t clock_ticks=100;
static int connected=1;
static NfcStatus mock_status={.state=NFC_READY};
lv_ui ui_wifi={0},ui_main={.screen=&main_screen,.is_init=true};
static struct {uint8_t *buffer;size_t capacity,*length;UartHandle_FinishCallBackFuncTypedef callback;void *context;} queued;
uint32_t lv_tick_get(void) {return clock_ticks;}
uint32_t HAL_GetTick(void) {return clock_ticks;}
uint32_t lv_tick_elaps(uint32_t at) {return clock_ticks-at;}
lv_obj_t *lv_scr_act(void) {return active;}
lv_obj_t *lv_obj_create(lv_obj_t *parent) {
    assert(objects_used<64);lv_obj_t *object=&objects[objects_used++];memset(object,0,sizeof(*object));object->parent=parent;return object;
}
lv_obj_t *lv_label_create(lv_obj_t *parent) {return lv_obj_create(parent);}
lv_obj_t *lv_img_create(lv_obj_t *parent) {return lv_obj_create(parent);}
void lv_img_set_src(lv_obj_t *o,const void *source) {assert(o && !o->deleted);o->source=source;}
void lv_img_cache_invalidate_src(const void *source) {(void)source;}
lv_res_t lv_img_decoder_get_info(const void *source,lv_img_header_t *header) {
    assert(!strcmp(source,"P:Static/nfc_bind.bin"));
    if(!prompt_exists) return LV_RES_INV;
    *header=(lv_img_header_t){.cf=LV_IMG_CF_INDEXED_1BIT,.w=104,.h=72};return LV_RES_OK;
}
lv_obj_t *lv_btn_create(lv_obj_t *parent) {return lv_obj_create(parent);}
void lv_obj_set_pos(lv_obj_t *o,int x,int y) {o->x=x;o->y=y;}
void lv_obj_set_width(lv_obj_t *o,int w) {o->w=w;}
void lv_obj_set_size(lv_obj_t *o,int w,int h) {(void)h;o->w=w;}
void lv_obj_center(lv_obj_t *o) {(void)o;}
void lv_obj_add_flag(lv_obj_t *o,int f) {o->flags|=f;}
void lv_obj_clear_flag(lv_obj_t *o,int f) {o->flags&=~f;}
void lv_label_set_text(lv_obj_t *o,const char *text) {assert(o && !o->deleted);assert(strlen(text)<sizeof(o->text));strcpy(o->text,text);}
lv_timer_t *lv_timer_create(void(*cb)(lv_timer_t*),uint32_t period,void *context) {(void)period;(void)context;fake_timer.callback=cb;return &fake_timer;}
void lv_timer_del(lv_timer_t *t) {assert(t==&fake_timer);fake_timer.callback=NULL;}
void setup_ui(lv_ui *page,lv_ui *last,void *data) {
    page->last_ui=last;if(page->gui_setup_func) page->gui_setup_func(data);page->is_init=true;active=page->screen;
}
void delete_ui(lv_ui *page,void *data) {
    page->gui_delete_func(data);page->is_init=false;
    for(unsigned i=0;i<objects_used;i++) objects[i].deleted=true;
    page->screen=NULL;active=page->last_ui->screen;
}
void WifiPage_OpenNfc(lv_ui *last) {wifi_nfc_entries++;setup_ui(&ui_wifi,last,NULL);}
int WifiPage_GetInfo(uint8_t out[31]) {memset(out,0,31);out[1]=connected?ACHO_WIFI_CONNECTED:0;return 1;}
void WifiPage_RequestRefresh(void) {refreshes++;}
void NfcService_Status(NfcStatus *out) {*out=mock_status;}
void NfcService_Clear(uint8_t owner) {assert(owner==NFC_OWNER_PAIR);clears++;mock_status.owner=NFC_OWNER_NONE;}
uint32_t NfcService_Publish(const char *text,size_t length,uint8_t owner,uint32_t ttl) {
    assert(owner==NFC_OWNER_PAIR && ttl && ttl<=40000 && length && text);
    publishes++;mock_status.owner=owner;mock_status.state=NFC_PREPARING;
    mock_status.expires_at=clock_ticks+ttl;return publishes;
}
int Transfer_SubmitMemoryWithTimeout(const char *json,uint8_t *buffer,size_t capacity,size_t *length,uint32_t timeout,UartHandle_FinishCallBackFuncTypedef cb,void *context) {
    assert(!queued.callback && strstr(json,"get_bind_token") && timeout==120000);
    queued.buffer=buffer;queued.capacity=capacity;queued.length=length;queued.callback=cb;queued.context=context;return 1;
}
static void item(unsigned which) {lv_event_t e={.code=LV_EVENT_SHORT_CLICKED,.user_data=(void*)(uintptr_t)which};menu_event(&e);}
static void press(int code) {lv_event_t e={.code=code};screen_event(&e);}
static void response(const char *text) {
    assert(queued.callback && strlen(text)<=queued.capacity);memcpy(queued.buffer,text,strlen(text));*queued.length=strlen(text);
    UartHandle_FinishCallBackFuncTypedef cb=queued.callback;void *context=queued.context;
    memset(&queued,0,sizeof(queued));cb(HAL_OK,context);
}
int main(void) {
    setup_ui(&ui_nfc,&ui_main,NULL);tick(NULL);assert(strstr(heading->text,"Menu"));
    mock_status.state=NFC_UNAVAILABLE;item(1);assert(!queued.callback && !(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"NFC not ready"));
    mock_status.state=NFC_READY;connected=0;tick(NULL);assert(!queued.callback && !(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"Wi-Fi first"));
    connected=1;tick(NULL);assert(queued.callback && pending && !(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"Getting token"));
    response("head.payload.signature");assert(publishes==1 && mock_status.expires_at==clock_ticks+40000);
    mock_status.state=NFC_READY;mock_status.published_revision=wanted_revision;tick(NULL);
    assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"40s left"));
    assert(!strcmp(prompt_image->source,"P:Static/nfc_bind.bin") && prompt_image->x==12 && prompt_image->y==21 && body->y==104);
    mock_status.state=NFC_BUSY;tick(NULL);assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"Move phone"));
    mock_status.state=NFC_READY;mock_status.published_revision=0;tick(NULL);assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && !strstr(body->text,"Tap phone"));
    mock_status.published_revision=wanted_revision;prompt_exists=false;prompt_state=PROMPT_UNCHECKED;tick(NULL);
    assert((prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && strstr(body->text,"Ready: tap")); // No SD image must not block pairing.
    prompt_exists=true;prompt_state=PROMPT_UNCHECKED;tick(NULL);assert(!(prompt_image->flags&LV_OBJ_FLAG_HIDDEN));
    clock_ticks+=30000;tick(NULL);assert(queued.callback && pending);response("invalid");assert(failed && publishes==1);
    clock_ticks+=5001;tick(NULL);assert(queued.callback);clock_ticks+=55000;
    response("head.payload.signature");assert(failed && publishes==1 && !(prompt_image->flags&LV_OBJ_FLAG_HIDDEN) && !strstr(body->text,"Tap phone")); // An expired token retains the illustration without claiming readiness.
    press(LV_EVENT_LONG_PRESSED);assert(!pairing && clears==1 && (prompt_image->flags&LV_OBJ_FLAG_HIDDEN));
    item(1);assert(queued.callback);delete_ui(&ui_nfc,NULL);response("head.payload.signature");
    assert(!pending && publishes==1 && active==&main_screen && clears==2); // No deleted widget access.
    clock_ticks=0x80000100u; // Menu still starts a request after 25 days of uptime.
    objects_used=0;setup_ui(&ui_nfc,&ui_main,NULL);item(1);assert(queued.callback);press(LV_EVENT_LONG_PRESSED);response("head.payload.signature");
    item(0);assert(active==ui_wifi.screen && ui_wifi.last_ui==&ui_nfc && wifi_nfc_entries==1);
    assert(refreshes);puts("NFC menu: SD illustration in preparing/error/ready states, missing-image fallback, hardware/offline guards, token refresh, stale rejection, clear-on-exit, late callbacks and NFC Wi-Fi navigation passed");
}
