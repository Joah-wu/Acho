#include "page_esp_download.h"
#include "bsp_usart.h"
#include "transfer_link.h"
#include "main.h"

static lv_obj_t *status_label;
static lv_timer_t *timer;
static bool resetting;
static uint32_t reset_at;
static void setup_screen(void *data);
static void delete_screen(void *data);

lv_ui ui_esp_download={
    .gui_setup_func=setup_screen,
    .gui_delete_func=delete_screen
};

static void update(lv_timer_t *unused) {
    LV_UNUSED(unused);
    if(resetting && lv_tick_elaps(reset_at)>=100) {
        HAL_GPIO_WritePin(WIFI_EN_GPIO_Port,WIFI_EN_Pin,GPIO_PIN_SET);
        resetting=false;
    }
    lv_label_set_text(status_label,resetting?"Resetting ESP32...":
        Transfer_IsPaused()?"Ready to flash\nUART1/2 released":"Releasing UARTs...\nPlease wait");
}

static void click(lv_event_t *event) {
    if(event->code==LV_EVENT_LONG_PRESSED) {
        delete_ui(&ui_esp_download,NULL);
    } else if(event->code==LV_EVENT_SHORT_CLICKED && Transfer_IsPaused() && !resetting) {
        /* Hold the physical ESP BOOT button while clicking to enter its ROM. */
        HAL_GPIO_WritePin(WIFI_EN_GPIO_Port,WIFI_EN_Pin,GPIO_PIN_RESET);
        reset_at=lv_tick_get();resetting=true;update(NULL);
    }
}

static void setup_screen(void *data) {
    LV_UNUSED(data);
    DebugUart_SetEnabled(0);
    Transfer_SetPaused(1);
    resetting=false;
    ui_esp_download.screen=lv_obj_create(NULL);
    lv_obj_clear_flag(ui_esp_download.screen,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(ui_esp_download.screen,0,0);
    lv_obj_set_style_bg_color(ui_esp_download.screen,lv_color_white(),0);
    lv_obj_set_style_bg_opa(ui_esp_download.screen,LV_OPA_COVER,0);
    lv_obj_set_style_text_color(ui_esp_download.screen,lv_color_black(),0);
    lv_obj_set_style_text_font(ui_esp_download.screen,&lv_font_montserrat_10,0);

    lv_obj_t *title=lv_label_create(ui_esp_download.screen);
    lv_label_set_text(title,"ESP32 Download");
    lv_obj_align(title,LV_ALIGN_TOP_MID,0,4);
    status_label=lv_label_create(ui_esp_download.screen);
    lv_obj_set_style_text_align(status_label,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_align(status_label,LV_ALIGN_TOP_MID,0,25);
    lv_obj_t *hint=lv_label_create(ui_esp_download.screen);
    lv_label_set_text(hint,"Hold ESP BOOT,\nclick to reset.\nFlash from PC.\nRelease BOOT; reset.\nHold knob: exit");
    lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_align(hint,LV_ALIGN_BOTTOM_MID,0,-6);

    lv_obj_add_flag(ui_esp_download.screen,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui_esp_download.screen,click,LV_EVENT_SHORT_CLICKED,NULL);
    lv_obj_add_event_cb(ui_esp_download.screen,click,LV_EVENT_LONG_PRESSED,NULL);
    lv_group_add_obj(ui_esp_download.group,ui_esp_download.screen);
    timer=lv_timer_create(update,20,NULL);
    update(NULL);
}

static void delete_screen(void *data) {
    LV_UNUSED(data);
    if(timer) {lv_timer_del(timer);timer=NULL;}
    if(resetting) HAL_GPIO_WritePin(WIFI_EN_GPIO_Port,WIFI_EN_Pin,GPIO_PIN_SET);
    resetting=false;
    Transfer_SetPaused(0);
    /* Restoring PA9 output here would drive against ESP32 TX. */
    DebugUart_SetEnabled(0);
    status_label=NULL;
}
