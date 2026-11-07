#include "page_wifi.h"
#include "page_main.h"
#include "transfer_link.h"
#include "nfc_service.h"
#include "acho_protocol.h"
#include "acho_wifi_info.h"
#include <stdio.h>
#include <string.h>

_Static_assert(ACHO_WIFI_QR_WIDTH+20<=MY_DISP_VER_RES,"Wi-Fi QR page exceeds screen height");
_Static_assert(ACHO_WIFI_QR_WIDTH<=MY_DISP_HOR_RES,"Wi-Fi QR page exceeds screen width");
enum { OP_NONE, OP_INFO, OP_QR, OP_START };
enum { VIEW_NORMAL, VIEW_DETAILS, VIEW_CONFIRM, VIEW_NFC };
enum { PROMPT_UNCHECKED, PROMPT_FAILED, PROMPT_READY };
enum { PROMPT_TOP=14, PROMPT_HEIGHT=MY_DISP_VER_RES-42 };
static const char nfc_prompt_path[]="P:Static/nfc_wifi.bin";
static uint8_t nfc_entry_marker;
static uint8_t info_rx[ACHO_WIFI_INFO_SIZE], info[ACHO_WIFI_INFO_SIZE];
static uint8_t qr_rx[ACHO_WIFI_QR_SIZE], qr_pixels[ACHO_WIFI_QR_SIZE];
static size_t info_length, qr_length;
static uint8_t pending, view;
static bool info_valid, qr_ready, auto_probe=true, start_requested, entered_provisioning;
static bool link_error;
static bool nfc_entry;
static uint8_t prompt_state;
static uint32_t nfc_revision;
static lv_obj_t *title, *body, *footer, *qr_image, *prompt_image;
static lv_timer_t *watcher;
static uint32_t last_query;
static lv_img_dsc_t qr_descriptor={
    .header={.always_zero=0,.w=ACHO_WIFI_QR_WIDTH,.h=ACHO_WIFI_QR_WIDTH,.cf=LV_IMG_CF_INDEXED_1BIT},
    .data_size=ACHO_WIFI_QR_SIZE-4,.data=qr_pixels+4
};
static void setup_screen(void *data);
static void delete_screen(void *data);
static void poll(lv_timer_t *timer);
static void render(void);
lv_ui ui_wifi={.gui_setup_func=setup_screen,.gui_delete_func=delete_screen};

static uint8_t default_view(void) { return nfc_entry?VIEW_NFC:VIEW_NORMAL; }
static bool nfc_ready(const NfcStatus *state) {
    return nfc_revision && state->state==NFC_READY && state->owner==NFC_OWNER_WIFI &&
        state->published_revision==nfc_revision;
}
static bool show_prompt(void) {
    if(prompt_state==PROMPT_UNCHECKED) {
        lv_img_header_t header;
        prompt_state=PROMPT_FAILED;
        if(lv_img_decoder_get_info(nfc_prompt_path,&header)==LV_RES_OK &&
           header.cf==LV_IMG_CF_INDEXED_1BIT && header.w>0 && header.w<=MY_DISP_HOR_RES-4 &&
           header.h>0 && header.h<=PROMPT_HEIGHT) {
            lv_img_set_src(prompt_image,nfc_prompt_path);
            /* Center within the area between the title and status line. */
            lv_obj_set_pos(prompt_image,(MY_DISP_HOR_RES-header.w)/2,PROMPT_TOP+(PROMPT_HEIGHT-header.h)/2);
            prompt_state=PROMPT_READY;
        }
    }
    if(prompt_state==PROMPT_READY) lv_obj_clear_flag(prompt_image,LV_OBJ_FLAG_HIDDEN);
    return prompt_state==PROMPT_READY;
}
static void show_link_error(bool compact) {
    TransferStatus state;Transfer_GetStatus(&state);
    static const char *const stages[]={"Idle","Wait BEGIN","Wait DATA","Wait END","Done","Paused"};
    const char *stage=state.stage<=TRANSFER_PAUSED?stages[state.stage]:"Unknown";
    const char *reason=state.stage==TRANSFER_PAUSED?"UART paused":
        state.tx_status!=HAL_OK?"UART TX failed":
        state.uart_errors?"UART RX error":
        state.stage==TRANSFER_DONE?"Invalid payload":
        !state.rx_bytes?"No UART response":
        !state.rx_frames?"Invalid UART frames":"Transfer incomplete";
    if(compact) lv_label_set_text(body,reason);
    else {
        char text[176];
        snprintf(text,sizeof(text),"ESP32 not ready.\n%s\nTX=%u R=%u\nRX=%lu F=%lu E=%lu\n%s",reason,
            (unsigned)state.tx_status,(unsigned)state.result,(unsigned long)state.rx_bytes,(unsigned long)state.rx_frames,
            (unsigned long)state.uart_errors,stage);
        lv_label_set_text(body,text);
    }
}
static void render_nfc(void) {
    /* The SD image explains the action; only the status confirms tag readiness. */
    bool illustrated=show_prompt();
    const char *text,*compact;
    const char *controls="Click: QR Hold: Back";
    if(start_requested || (info_valid && (info[1]&ACHO_WIFI_RESTARTING))) {
        text="Restarting ESP32...\nPlease wait.";compact="Restarting ESP32...";
        controls="Hold: Back";
    } else if(!info_valid || link_error) {
        controls="Click: Retry Hold: Back";
        if(link_error) {
            if(illustrated) lv_obj_set_pos(body,6,104);
            show_link_error(illustrated);lv_label_set_text(footer,controls);return;
        }
        text="Reading Wi-Fi info...\nPlease wait.";compact="Reading Wi-Fi...";
    } else if(info[1]&ACHO_WIFI_CONNECTED) {
        text=entered_provisioning?"Wi-Fi connected!\nSetup complete.\nOpen Bind next to\nlink your account.":"Wi-Fi connected.\nClick to configure\na different Wi-Fi.";
        compact="Wi-Fi connected";
        controls=entered_provisioning?"Click / Hold: Back":"Click: Change Hold: Back";
    } else if(!(info[1]&ACHO_WIFI_PROVISIONING)) {
        text="Wi-Fi saved,\nbut not connected.\nClick to configure\na different Wi-Fi.";compact="Wi-Fi offline";
        controls="Click: Change Hold: Back";
    } else {
        NfcStatus nfc;NfcService_Status(&nfc);
        if(nfc_ready(&nfc)) {
            text="Ready: tap phone\nwith the Acho app.\nNFC Wi-Fi setup";compact="Ready: tap Acho app";
        } else if(nfc.state==NFC_UNAVAILABLE || nfc.state==NFC_ERROR) {
            text="NFC chip not ready.\nCheck hardware,\nor click for QR.";compact="NFC not ready";
        } else if(nfc.state==NFC_BUSY) {
            text="Preparing NFC...\nMove phone away,\nthen tap when ready.";compact="Move phone away";
        } else {
            text="Updating NFC data...\nPlease wait before\ntapping the phone.";compact="Updating NFC...";
        }
    }
    if(illustrated) lv_obj_set_pos(body,6,104);
    lv_label_set_text(body,illustrated?compact:text);
    lv_label_set_text(footer,controls);
}
static void render(void) {
    if(!ui_wifi.screen) return;
    lv_obj_add_flag(qr_image,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(prompt_image,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(body,LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(body,6,20);
    lv_label_set_text(title,nfc_entry?"NFC Wi-Fi Setup":"Wi-Fi Setup");
    if(view==VIEW_CONFIRM) {
        lv_label_set_text(body,"Change Wi-Fi?\nSaved Wi-Fi will\nbe cleared.\nServer and PoP stay.");
        lv_label_set_text(footer,"Click: Yes  Hold: Back");
    } else if(view==VIEW_NFC) {
        render_nfc();
    } else if(start_requested || (info_valid && (info[1]&ACHO_WIFI_RESTARTING))) {
        lv_label_set_text(body,"Restarting ESP32...\nPlease wait for\nthe setup QR code.");
        lv_label_set_text(footer,"Hold: Back");
    } else if(!info_valid || link_error) {
        if(link_error) show_link_error(false);
        else lv_label_set_text(body,"Reading Wi-Fi info...\nPlease wait.");
        lv_label_set_text(footer,"Click: Retry Hold: Back");
    } else if(info[1]&ACHO_WIFI_CONNECTED) {
        lv_label_set_text(body,entered_provisioning?"Wi-Fi connected!\nSetup complete.\nOpen Bind next to\nlink your account.":"Wi-Fi connected.\nClick to configure\na different Wi-Fi.");
        lv_label_set_text(footer,entered_provisioning?"Click / Hold: Back":"Click: Change Hold: Back");
    } else if(!(info[1]&ACHO_WIFI_PROVISIONING)) {
        lv_label_set_text(body,"Wi-Fi saved,\nbut not connected.\nClick to configure\na different Wi-Fi.");
        lv_label_set_text(footer,"Click: Change Hold: Back");
    } else if(view==VIEW_DETAILS) {
        char text[150];
        snprintf(text,sizeof(text),"AP: %s\n\nPoP / AP password:\n%s\n\nUse the Acho app.",
            (const char*)info+ACHO_WIFI_SSID_OFFSET,(const char*)info+ACHO_WIFI_POP_OFFSET);
        lv_label_set_text(body,text);
        lv_label_set_text(footer,nfc_entry?"Click: NFC Hold: Back":"Click: QR  Hold: Back");
    } else if(qr_ready) {
        lv_obj_add_flag(body,LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(qr_image,LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(qr_image,&qr_descriptor);
        NfcStatus nfc;NfcService_Status(&nfc);
        lv_label_set_text(footer,nfc_ready(&nfc)?"Scan or tap Acho app":"Scan Acho / Click: Info");
    } else {
        lv_label_set_text(body,"Preparing setup QR...\nNo internet or\nSD card required.");
        lv_label_set_text(footer,"Hold: Back");
    }
}
static void received(HAL_StatusTypeDef status,void *data) {
    uint8_t operation=(uint8_t)(uintptr_t)data;
    pending=OP_NONE;
    if(status!=HAL_OK) {
        link_error=true;
        if(operation==OP_START) start_requested=false;
        render(); return;
    }
    if(operation==OP_QR) {
        uint32_t expected=7u|(ACHO_WIFI_QR_WIDTH<<10)|(ACHO_WIFI_QR_WIDTH<<21);
        if(qr_length!=sizeof(qr_rx) || acho_u32(qr_rx)!=expected) { link_error=true; render(); return; }
        lv_img_cache_invalidate_src(&qr_descriptor);
        memcpy(qr_pixels,qr_rx,sizeof(qr_pixels)); qr_ready=true; link_error=false;
    } else {
        if(!acho_wifi_info_valid(info_rx,info_length)) { link_error=true; start_requested=false; render(); return; }
        if(!info_valid || memcmp(info+2,info_rx+2,ACHO_WIFI_INFO_SIZE-2)) qr_ready=false;
        memcpy(info,info_rx,sizeof(info)); info_valid=true; link_error=false;
        if(ui_wifi.is_init && (info[1]&ACHO_WIFI_PROVISIONING) && !(info[1]&ACHO_WIFI_CONNECTED))
            nfc_revision=NfcService_PublishWifi(info);
        else { nfc_revision=0;NfcService_Clear(NFC_OWNER_WIFI); }
        if(operation==OP_START) start_requested=false;
        if(ui_wifi.is_init && (info[1]&ACHO_WIFI_PROVISIONING) && !(info[1]&ACHO_WIFI_CONNECTED)) entered_provisioning=true;
        if(auto_probe) {
            auto_probe=false;
            if((info[1]&ACHO_WIFI_PROVISIONING) && !(info[1]&ACHO_WIFI_CONFIGURED) && lv_scr_act()==ui_main.screen)
                setup_ui(&ui_wifi,&ui_main,NULL);
        }
    }
    render();
    if(ui_wifi.is_init && !qr_ready && (info[1]&ACHO_WIFI_PROVISIONING)) last_query=0;
}
static int submit(uint8_t operation) {
    const char *json=operation==OP_QR?"{\"op\":\"get_wifi_qr\"}":
        operation==OP_START?"{\"op\":\"start_wifi_provisioning\"}":"{\"op\":\"get_wifi_info\"}";
    uint8_t *destination=operation==OP_QR?qr_rx:info_rx;
    size_t capacity=operation==OP_QR?sizeof(qr_rx):sizeof(info_rx);
    size_t *length=operation==OP_QR?&qr_length:&info_length;
    if(!Transfer_SubmitMemory(json,destination,capacity,length,received,(void*)(uintptr_t)operation)) return 0;
    pending=operation; last_query=lv_tick_get(); return 1;
}
static void poll(lv_timer_t *timer) {
    (void)timer;
    /* EEPROM completion is independent of a pending UART request. */
    if(ui_wifi.is_init && view==VIEW_NFC) render();
    if(pending) return;
    uint32_t interval=ui_wifi.is_init || auto_probe?2000u:10000u;
    if(last_query && lv_tick_elaps(last_query)<interval) return;
    if(start_requested) { submit(OP_START); return; }
    if(ui_wifi.is_init && view!=VIEW_NFC && info_valid && !link_error && !qr_ready &&
       (info[1]&ACHO_WIFI_PROVISIONING) && !(info[1]&ACHO_WIFI_CONNECTED)) {
        submit(OP_QR); return;
    }
    submit(OP_INFO);
}
static void click(lv_event_t *event) {
    if(event->code==LV_EVENT_LONG_PRESSED) {
        if(view==VIEW_CONFIRM) { view=default_view(); render(); }
        else delete_ui(&ui_wifi,NULL);
    } else if(event->code==LV_EVENT_SHORT_CLICKED) {
        if(view==VIEW_CONFIRM) {
            view=default_view(); start_requested=true; entered_provisioning=true; last_query=0;
            render(); poll(NULL);
        } else if(!info_valid || link_error) { last_query=0; poll(NULL); }
        else if((info[1]&ACHO_WIFI_CONNECTED) && entered_provisioning) delete_ui(&ui_wifi,NULL);
        else if(!(info[1]&ACHO_WIFI_PROVISIONING) || (info[1]&ACHO_WIFI_CONNECTED)) {
            view=VIEW_CONFIRM; render();
        } else {
            view=view==VIEW_NFC?VIEW_NORMAL:view==VIEW_NORMAL?VIEW_DETAILS:default_view();
            render();
            if(view==VIEW_NORMAL && !qr_ready) { last_query=0;poll(NULL); }
        }
    }
}
static void setup_screen(void *data) {
    nfc_entry=data==&nfc_entry_marker;
    auto_probe=false;view=default_view();prompt_state=PROMPT_UNCHECKED;
    entered_provisioning=info_valid && (info[1]&ACHO_WIFI_PROVISIONING) && !(info[1]&ACHO_WIFI_CONNECTED);
    ui_wifi.screen=lv_obj_create(NULL);
    lv_obj_set_style_pad_all(ui_wifi.screen,0,0);
    lv_obj_set_style_bg_color(ui_wifi.screen,lv_color_white(),0);
    lv_obj_set_style_bg_opa(ui_wifi.screen,LV_OPA_COVER,0);
    lv_obj_set_style_text_color(ui_wifi.screen,lv_color_black(),0);
    lv_obj_set_style_text_font(ui_wifi.screen,&lv_font_montserrat_10,0);
    lv_obj_clear_flag(ui_wifi.screen,LV_OBJ_FLAG_SCROLLABLE);
    title=lv_label_create(ui_wifi.screen); lv_obj_set_pos(title,0,0); lv_obj_set_width(title,MY_DISP_HOR_RES);
    lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
    body=lv_label_create(ui_wifi.screen); lv_obj_set_pos(body,6,20); lv_obj_set_width(body,MY_DISP_HOR_RES-12);
    lv_obj_set_style_text_align(body,LV_TEXT_ALIGN_CENTER,0);
    qr_image=lv_img_create(ui_wifi.screen);
    lv_obj_set_pos(qr_image,(MY_DISP_HOR_RES-ACHO_WIFI_QR_WIDTH)/2,10);
    prompt_image=lv_img_create(ui_wifi.screen);
    footer=lv_label_create(ui_wifi.screen); lv_obj_set_pos(footer,0,MY_DISP_VER_RES-10);
    lv_obj_set_width(footer,MY_DISP_HOR_RES); lv_obj_set_style_text_align(footer,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_add_flag(ui_wifi.screen,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui_wifi.screen,click,LV_EVENT_SHORT_CLICKED,NULL);
    lv_obj_add_event_cb(ui_wifi.screen,click,LV_EVENT_LONG_PRESSED,NULL);
    lv_group_add_obj(ui_wifi.group,ui_wifi.screen);
    nfc_revision=info_valid?NfcService_PublishWifi(info):0;
    render(); last_query=0; poll(NULL);
}
static void delete_screen(void *data) {
    (void)data;
    NfcService_Clear(NFC_OWNER_WIFI);
    lv_img_cache_invalidate_src(nfc_prompt_path);
    title=body=footer=qr_image=prompt_image=NULL;
    nfc_revision=0;
    /* Requests use static RAM and never retain pointers to deleted widgets. */
}
void WifiPage_OpenNfc(lv_ui *last_ui) {
    setup_ui(&ui_wifi,last_ui,&nfc_entry_marker);
}
void WifiPage_Init(void) {
    if(!watcher) watcher=lv_timer_create(poll,500,NULL);
    last_query=0; poll(NULL);
}

int WifiPage_GetInfo(uint8_t out[31]) {
    if(!info_valid) return 0;
    memcpy(out,info,sizeof(info));return 1;
}
void WifiPage_RequestRefresh(void) { last_query=0;poll(NULL); }
