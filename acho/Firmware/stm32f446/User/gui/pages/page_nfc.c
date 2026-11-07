#include "page_nfc.h"
#include "page_wifi.h"
#include "nfc_service.h"
#include "transfer_link.h"
#include "acho_ndef.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *heading,*body,*footer,*menu[3];
static lv_obj_t *prompt_image;
static const char nfc_prompt_path[]="P:Static/nfc_bind.bin";
enum { PROMPT_UNCHECKED, PROMPT_FAILED, PROMPT_READY };
enum { PROMPT_TOP=14, PROMPT_HEIGHT=MY_DISP_VER_RES-42 };
static uint8_t prompt_state;
static lv_timer_t *timer;
static bool pairing,pending,failed;
static uint8_t token[ACHO_NFC_MAX_PAYLOAD+1];
static size_t token_length;
static uint32_t wanted_revision,request_at,next_request;
static void setup_screen(void *data);
static void delete_screen(void *data);
static void tick(lv_timer_t *unused);
lv_ui ui_nfc={.gui_setup_func=setup_screen,.gui_delete_func=delete_screen};

static int valid_token(void) {
    unsigned dots=0;size_t segment=0;
    if(!token_length || token_length>ACHO_NFC_MAX_PAYLOAD) return 0;
    for(size_t i=0;i<token_length;i++) {
        uint8_t c=token[i];
        if(c=='.') { if(!segment || ++dots>2) return 0;segment=0; }
        else {
            if(!((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='-')) return 0;
            segment++;
        }
    }
    return dots==2 && segment;
}
static void received(HAL_StatusTypeDef state,void *data) {
    (void)data;pending=false;
    if(!ui_nfc.is_init || !pairing) return;
    uint32_t age=lv_tick_elaps(request_at);
    if(state!=HAL_OK || !valid_token() || age>=55000) {
        failed=true;next_request=lv_tick_get()+5000;tick(NULL);return;
    }
    uint32_t ttl=55000-age;if(ttl>40000) ttl=40000;
    wanted_revision=NfcService_Publish((const char*)token,token_length,NFC_OWNER_PAIR,ttl);
    failed=!wanted_revision;
    next_request=lv_tick_get()+(ttl>15000?ttl-10000:5000);
    tick(NULL);
}
static void request_token(void) {
    if(pending) return;
    request_at=lv_tick_get();
    pending=Transfer_SubmitMemoryWithTimeout("{\"op\":\"get_bind_token\"}",token,sizeof(token)-1,&token_length,
        120000,received,NULL)!=0;
    if(!pending) { failed=true;next_request=lv_tick_get()+2000; }
}
static void focus_menu(void) {
    lv_group_remove_all_objs(ui_nfc.group);
    for(unsigned i=0;i<3;i++) {
        if(pairing) lv_obj_add_flag(menu[i],LV_OBJ_FLAG_HIDDEN);
        else { lv_obj_clear_flag(menu[i],LV_OBJ_FLAG_HIDDEN);lv_group_add_obj(ui_nfc.group,menu[i]); }
    }
    if(pairing) lv_group_add_obj(ui_nfc.group,ui_nfc.screen);
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
static void pair_status(bool illustrated,const char *text,const char *compact) {
    lv_label_set_text(body,illustrated?compact:text);
}
static void tick(lv_timer_t *unused) {
    (void)unused;
    if(!ui_nfc.screen || lv_scr_act()!=ui_nfc.screen) return;
    lv_obj_add_flag(prompt_image,LV_OBJ_FLAG_HIDDEN);
    NfcStatus state;NfcService_Status(&state);
    if(!pairing) {
        lv_label_set_text(heading,"NFC Menu");
        char message[80];
        if(state.state==NFC_UNAVAILABLE || state.state==NFC_ERROR)
            snprintf(message,sizeof(message),"ST25DV not ready\nCheck NFC hardware");
        else snprintf(message,sizeof(message),"ST25DV detected\n%s",state.uid);
        lv_label_set_text(body,message);lv_obj_set_pos(body,4,84);
        lv_label_set_text(footer,"Hold: Back");return;
    }
    lv_label_set_text(heading,"NFC Account Pair");lv_obj_set_pos(body,6,22);
    bool illustrated=show_prompt();
    if(illustrated) lv_obj_set_pos(body,4,104);
    lv_label_set_text(footer,"Click: Retry Hold: Back");
    uint8_t wifi[ACHO_WIFI_INFO_SIZE];
    if(state.state==NFC_UNAVAILABLE || state.state==NFC_ERROR) {
        pair_status(illustrated,"NFC chip not ready.\nCheck hardware\nand try again.","NFC not ready");return;
    }
    if(!WifiPage_GetInfo(wifi) || !(wifi[1]&ACHO_WIFI_CONNECTED)) {
        pair_status(illustrated,"Connect Wi-Fi first.\nUse NFC Wi-Fi Setup\nfrom the NFC menu.","Connect Wi-Fi first");return;
    }
    if(!pending && (int32_t)(lv_tick_get()-next_request)>=0) request_token();
    if(wanted_revision && state.state==NFC_READY && state.owner==NFC_OWNER_PAIR && state.published_revision==wanted_revision) {
        int32_t remaining=(int32_t)(state.expires_at-HAL_GetTick());
        if(remaining>0) {
            char text[110];
            if(illustrated) {
                snprintf(text,sizeof(text),"Tap phone: %lds left",(long)((remaining+999)/1000));
            } else snprintf(text,sizeof(text),"Ready: tap phone\nwith the Acho app.\n\nValid for %ld seconds.\nToken auto-refreshes.",(long)((remaining+999)/1000));
            lv_label_set_text(body,text);return;
        }
    }
    if(state.state==NFC_BUSY) pair_status(illustrated,"Preparing NFC...\nMove phone away,\nthen tap when ready.","Move phone away");
    else if(pending) pair_status(illustrated,"Getting pairing token\nfrom the server...\nPlease wait.","Getting token...");
    else if(failed) pair_status(illustrated,"Server request failed.\nRetrying shortly...\nCheck server / Wi-Fi.","Server failed");
    else pair_status(illustrated,"Updating NFC data...\nPlease wait before\ntapping the phone.","Updating NFC...");
}
static void screen_event(lv_event_t *event) {
    if(!pairing) return;
    if(event->code==LV_EVENT_LONG_PRESSED) {
        NfcService_Clear(NFC_OWNER_PAIR);pairing=false;wanted_revision=0;
        focus_menu();tick(NULL);
    } else if(event->code==LV_EVENT_SHORT_CLICKED) {
        WifiPage_RequestRefresh();next_request=lv_tick_get();tick(NULL);
    }
}
static void menu_event(lv_event_t *event) {
    if(event->code==LV_EVENT_LONG_PRESSED) { delete_ui(&ui_nfc,NULL);return; }
    if(event->code!=LV_EVENT_SHORT_CLICKED) return;
    unsigned item=(unsigned)(uintptr_t)lv_event_get_user_data(event);
    if(item==0) WifiPage_OpenNfc(&ui_nfc);
    else if(item==1) {
        pairing=true;failed=false;wanted_revision=0;next_request=lv_tick_get();
        prompt_state=PROMPT_UNCHECKED;
        WifiPage_RequestRefresh();focus_menu();tick(NULL);
    } else delete_ui(&ui_nfc,NULL);
}
static void setup_screen(void *data) {
    (void)data;pairing=false;failed=false;wanted_revision=0;prompt_state=PROMPT_UNCHECKED;
    ui_nfc.screen=lv_obj_create(NULL);
    lv_obj_set_style_pad_all(ui_nfc.screen,0,0);
    lv_obj_set_style_bg_color(ui_nfc.screen,lv_color_white(),0);
    lv_obj_set_style_text_color(ui_nfc.screen,lv_color_black(),0);
    lv_obj_set_style_text_font(ui_nfc.screen,&lv_font_montserrat_10,0);
    lv_obj_clear_flag(ui_nfc.screen,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_nfc.screen,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui_nfc.screen,screen_event,LV_EVENT_SHORT_CLICKED,NULL);
    lv_obj_add_event_cb(ui_nfc.screen,screen_event,LV_EVENT_LONG_PRESSED,NULL);
    heading=lv_label_create(ui_nfc.screen);lv_obj_set_pos(heading,0,0);lv_obj_set_width(heading,128);
    lv_obj_set_style_text_align(heading,LV_TEXT_ALIGN_CENTER,0);
    const char *items[]={"NFC Wi-Fi Setup","NFC Account Pair","Back"};
    for(unsigned i=0;i<3;i++) {
        menu[i]=lv_btn_create(ui_nfc.screen);
        lv_obj_remove_style_all(menu[i]);
        lv_obj_set_pos(menu[i],4,14+i*22);lv_obj_set_size(menu[i],120,20);
        lv_obj_set_style_pad_all(menu[i],0,0);
        /* The default color theme maps its button and text to white at 1 bpp. */
        lv_obj_set_style_bg_opa(menu[i],LV_OPA_COVER,0);
        lv_obj_set_style_bg_color(menu[i],lv_color_white(),0);
        lv_obj_set_style_text_color(menu[i],lv_color_black(),0);
        lv_obj_set_style_border_width(menu[i],1,0);
        lv_obj_set_style_border_color(menu[i],lv_color_black(),0);
        lv_obj_set_style_radius(menu[i],2,0);
        lv_obj_set_style_bg_color(menu[i],lv_color_black(),LV_STATE_FOCUSED);
        lv_obj_set_style_text_color(menu[i],lv_color_white(),LV_STATE_FOCUSED);
        lv_obj_set_style_bg_color(menu[i],lv_color_black(),LV_STATE_PRESSED);
        lv_obj_set_style_text_color(menu[i],lv_color_white(),LV_STATE_PRESSED);
        lv_obj_add_event_cb(menu[i],menu_event,LV_EVENT_SHORT_CLICKED,(void*)(uintptr_t)i);
        lv_obj_add_event_cb(menu[i],menu_event,LV_EVENT_LONG_PRESSED,(void*)(uintptr_t)i);
        lv_obj_t *label=lv_label_create(menu[i]);lv_label_set_text(label,items[i]);lv_obj_center(label);
        lv_group_add_obj(ui_nfc.group,menu[i]);
    }
    prompt_image=lv_img_create(ui_nfc.screen);lv_obj_add_flag(prompt_image,LV_OBJ_FLAG_HIDDEN);
    body=lv_label_create(ui_nfc.screen);lv_obj_set_width(body,120);lv_obj_set_pos(body,4,84);
    lv_obj_set_style_text_align(body,LV_TEXT_ALIGN_CENTER,0);
    footer=lv_label_create(ui_nfc.screen);lv_obj_set_width(footer,128);lv_obj_set_pos(footer,0,118);
    lv_obj_set_style_text_align(footer,LV_TEXT_ALIGN_CENTER,0);
    timer=lv_timer_create(tick,500,NULL);
    /* setup_ui marks the page active immediately after this function returns. */
    lv_label_set_text(heading,"NFC Menu");lv_label_set_text(body,"Checking ST25DV...");lv_label_set_text(footer,"Hold: Back");
    WifiPage_RequestRefresh();
}
static void delete_screen(void *data) {
    (void)data;pairing=false;NfcService_Clear(NFC_OWNER_PAIR);
    if(timer) { lv_timer_del(timer);timer=NULL; }
    lv_img_cache_invalidate_src(nfc_prompt_path);
    heading=body=footer=prompt_image=NULL;
}
