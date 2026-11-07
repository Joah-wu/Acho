//
// Created by Mr.WU on 2023/9/23.
//

#include "page_filelist.h"
#include "../../broker_app.h"
#include "sdio.h"
#include "transfer_link.h"

static void setup_screen(void *user_data);

static void delete_screen(void *user_data);

static void resume_screen(void *user_data);
lv_ui ui_filelist = {
        .screen=NULL,
        .last_ui=NULL,
        .is_init = false,
        .gui_setup_func = setup_screen,
        .gui_resume_func = resume_screen,
        .gui_delete_func = delete_screen
};

static void waitingTimerCallBack(lv_timer_t *t) {
    static uint8_t flag = 0;
    lv_obj_t *hint = (lv_obj_t *) t->user_data;
    if (hint) {
        switch (flag) {
            case 0: {
                lv_label_set_text(hint, "DownLoading");flag = 1;break;
            }
            case 1: {
                lv_label_set_text(hint, "DownLoading.");flag = 2;break;
            }
            case 2: {
                lv_label_set_text(hint, "DownLoading..");flag = 3;break;
            }
            case 3: {
                lv_label_set_text(hint, "DownLoading...");flag = 0;break;
            }
            default:break;
        }
    }
}

static void dismissDownloadHint(lv_timer_t *timer) {
    lv_obj_t *hint=timer->user_data;
    hint->user_data=NULL;
    lv_obj_del(hint);
    lv_timer_del(timer);
    lv_group_focus_freeze(ui_filelist.group,false);
}
static void finishCallBackFunc(HAL_StatusTypeDef state,void *data) {
    lv_timer_t *timer=data;
    if(!timer) return;
    lv_obj_t *hint=timer->user_data;
    if(!hint) return;
    lv_label_set_text(hint,state==HAL_OK ? LV_SYMBOL_OK "Successful!" :
                           state==HAL_TIMEOUT ? LV_SYMBOL_CLOSE "TimeOut!" : LV_SYMBOL_CLOSE "ERROR!");
    lv_timer_set_cb(timer,dismissDownloadHint);
    lv_timer_set_period(timer,2000);
    lv_timer_reset(timer);
}
static void deleteDownloadHint(lv_event_t *event) {
    lv_obj_t *hint=lv_event_get_target(event);
    if(hint->user_data) lv_timer_del(hint->user_data);
}
static void click_event(lv_event_t *e) {
    char *id;
    char *name;
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    if (e->code == LV_EVENT_LONG_PRESSED) {
        delete_ui(&ui_filelist, NULL);
    } else if (e->code == LV_EVENT_SHORT_CLICKED) {
        id = (char *) target->user_data;
        name = id + strlen((char*)id) + 1;
        lv_group_focus_freeze(ui_filelist.group, true);
        lv_obj_t *wait_hint = lv_label_create(ui_filelist.screen);
        lv_obj_set_size(wait_hint, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_set_pos(wait_hint, LV_PCT(5), LV_PCT(40));
        lv_obj_set_style_bg_color(wait_hint, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(wait_hint, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(wait_hint, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(wait_hint, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(wait_hint, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(wait_hint, "DownLoading");
        lv_obj_set_style_text_color(wait_hint, lv_color_make(0x00,0x00,0x00),LV_PART_MAIN);
        lv_obj_set_style_text_align(wait_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(wait_hint, &lv_font_montserrat_14, LV_PART_MAIN);

        lv_timer_t *timer = lv_timer_create(waitingTimerCallBack, 100, wait_hint);
        wait_hint->user_data=timer;
        lv_obj_add_event_cb(wait_hint,deleteDownloadHint,LV_EVENT_DELETE,NULL);
        char *type = lv_obj_get_user_data(ui_filelist.screen);
        if(memcmp(type,"IMG",3)==0)
            downLoadFile(IMG,id, name,finishCallBackFunc,timer);
        else if (memcmp(type,"AUDIO",5)==0)
            downLoadFile(AUDIO,id, name,finishCallBackFunc,timer);

    } else if (e->code == LV_EVENT_FOCUSED) {
        /* 当焦点进入列表时，滚动列表以确保焦点可见 */
        lv_obj_scroll_to_view_recursive(target, LV_ANIM_OFF);
        lv_label_set_long_mode(target, LV_LABEL_LONG_SCROLL_CIRCULAR);
    } else if (e->code == LV_EVENT_DEFOCUSED) {
        lv_label_set_long_mode(target, LV_LABEL_LONG_DOT);
    }
}



static void setup_screen(void *user_data) {
    ui_filelist.screen = lv_obj_create(NULL);
    lv_obj_set_user_data(ui_filelist.screen,user_data);
    lv_obj_t *div = lv_obj_create(ui_filelist.screen);
    lv_obj_set_flex_flow(div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(div, LV_PCT(94), LV_PCT(94));
    lv_obj_set_pos(div, LV_PCT(3), LV_PCT(3));
    lv_obj_set_style_radius(div, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(div, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(div, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(div, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(div, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(div, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(div, 5, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(div, LV_SCROLLBAR_MODE_OFF);


    lv_obj_t *title = lv_label_create(div);
    lv_obj_set_size(title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_pos(title, LV_PCT(0), LV_PCT(0));
    lv_label_set_text(title, "Select A File");
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(title, 1, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_t *screen_list = lv_obj_create(div);
    lv_obj_set_flex_flow(screen_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(screen_list, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_left(screen_list, 10, LV_PART_MAIN);
    lv_obj_set_size(screen_list, LV_PCT(100), LV_PCT(82));
    lv_obj_set_pos(screen_list, LV_PCT(0), LV_PCT(0));
    lv_obj_set_style_bg_color(screen_list, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen_list, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(screen_list, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_scroll_dir(screen_list, LV_DIR_VER);
    lv_obj_set_style_bg_color(screen_list, lv_color_make(0x00, 0x00, 0x00), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(screen_list, LV_OPA_100, LV_PART_SCROLLBAR);

    resume_screen(NULL);
    File_InfoTypedef file_info;
    FILE_Info_StateTypedef state;
    char buff[FILE_INFO_TOTAL_SIZE] = {0};
    FIL *file = &SDFile;
    if(memcmp(user_data,"IMG",3)==0)
        f_open(file, "img_list.txt", FA_OPEN_ALWAYS | FA_READ);
    else if(memcmp(user_data,"AUDIO",3)==0)
        f_open(file, "audio_list.txt", FA_OPEN_ALWAYS | FA_READ);
    uint16_t file_count = 0;
    HeapManager.init(256);
    if (file) {
        while (true) {
            state = loadNextFileData(file, &file_info);
            if (state == FILE_INFO_OK) {
                lv_obj_t *label = lv_label_create(screen_list);
                memset(buff, 0, sizeof(buff));
                snprintf(buff, sizeof(buff), "%s %s", file_info.name, file_info.description);
                lv_label_set_text(label, buff);
                char metadata[FILE_INFO_ID_SIZE+FILE_INFO_NAME_SIZE];
                size_t id_size=strlen(file_info.id)+1;
                size_t name_size=strlen(file_info.name)+1;
                memcpy(metadata,file_info.id,id_size);
                memcpy(metadata+id_size,file_info.name,name_size);
                label->user_data=HeapManager.push(metadata,id_size+name_size);
                if(!label->user_data) { lv_obj_del(label); break; }
                lv_obj_set_size(label, LV_PCT(95), LV_SIZE_CONTENT);
                lv_obj_add_style(label, &style_screen_list_extra_btns_main_default, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_add_style(label, &style_screen_list_extra_btns_main_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
                lv_obj_add_style(label, &style_screen_list_extra_btns_main_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
                lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_event_cb(label, click_event, LV_EVENT_SHORT_CLICKED, NULL);
                lv_obj_add_event_cb(label, click_event, LV_EVENT_LONG_PRESSED, NULL);
                lv_obj_add_event_cb(label, click_event, LV_EVENT_FOCUSED, NULL);
                lv_obj_add_event_cb(label, click_event, LV_EVENT_DEFOCUSED, NULL);
                lv_group_add_obj(lv_group_get_default(), label);
                file_count++;
            } else {
                break;
            }
        }
    }
    f_close(file);
    if (file_count == 0) {
        lv_obj_add_flag(div, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(div, click_event, LV_EVENT_LONG_PRESSED, NULL);
        lv_group_add_obj(lv_group_get_default(), div);
    }
}
static void resume_screen(void *user_data){
    lv_style_set_text_font(&style_screen_list_extra_btns_main_default, &lv_font_montserrat_14);
    lv_style_set_text_font(&style_screen_list_extra_btns_main_pressed, &lv_font_montserrat_14);
    lv_style_set_text_font(&style_screen_list_extra_btns_main_focused, &lv_font_montserrat_14);
}
static void delete_screen(void *user_data) {
    Transfer_CancelCallbacks();
    HeapManager.reset();
}

