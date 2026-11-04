//
// Created by Mr.WU on 2023/10/2.
//
#include <stdio.h>
#include <string.h>
#include "page_audio.h"
#include "fatfs.h"
#include "page_play.h"
#include "../../fops.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sdio.h"
#include "WM8960Apps.h"



static void setup_screen(void *user_data);

static void resume_screen(void *user_data);

static void delete_screen(void *user_data);

static uint16_t file_index = 0;
static uint16_t max_file_count = 0;
static lv_obj_t *audio_img;
static bool pending_track_start;
static bool track_open_ready;
static char selected_track_path[_MAX_LFN * 2 + 1];
static uint8_t audio_last_state;
static bool open_after_close;

typedef enum {
    BOX_CLOSED,
    BOX_DROPPING,
    BOX_OPENING,
    BOX_OPEN,
    BOX_CLOSING
} box_state_t;

static box_state_t box_state = BOX_CLOSED;

#define AUDIO_WORK_FRAME_FIRST  1
#define AUDIO_WORK_FRAME_LAST   10
#define AUDIO_WORK_ANIM_TIME    1000
#define AUDIO_WORK_ANIM_DELAY   200

static void startAnim(lv_obj_t *tar, const char *mode, uint32_t duration,
                      uint32_t delay, int32_t start, int32_t end);
static void try_start_selected_track(void);

static bool animation_mode_is(const lv_obj_t *obj, const char *mode)
{
    const char *current_mode = obj ? lv_obj_get_user_data((lv_obj_t *)obj) : NULL;
    return current_mode != NULL && strcmp(current_mode, mode) == 0;
}

static FRESULT select_audio_file(lv_obj_t *title, uint16_t index)
{
    DIR dir;
    char *file_name = NULL;
    FRESULT state = f_opendir(&dir, "Audios");

    if(state != FR_OK) return state;
    state = exf_scan_files(&dir, &file_name, NULL, index);
    if(state == FR_OK && file_name != NULL) {
        lv_label_set_text(title, file_name);
        snprintf(selected_track_path, sizeof(selected_track_path), "0:/Audios/%s", file_name);
        f_closedir(&dir);
    }
    return state;
}

static uint16_t count_audio_files(void)
{
    DIR dir;
    FILINFO info;
    uint16_t count = 0;

    if(f_opendir(&dir, "Audios") != FR_OK) return 0;
    while(f_readdir(&dir, &info) == FR_OK && info.fname[0] != '\0') {
        if(info.fname[0] != '.' && (info.fattrib & AM_DIR) == 0) count++;
    }
    f_closedir(&dir);
    return count;
}

lv_ui ui_audio = {
        .screen=NULL,
        .last_ui=NULL,
        .group=NULL,
        .is_init = false,
        .gui_setup_func = setup_screen,
        .gui_resume_func = resume_screen,
        .gui_delete_func = delete_screen
};


static void custom_anim_callback(void *obj, int32_t value) {
    char buff[60] = {0};
    char dir_path[] = "P:Static\\Animation\\audio\\";
    lv_obj_t *target = obj; //target -> imgxxx
    if (target->user_data) {
        if (animation_mode_is(target, "W") || animation_mode_is(target, "WE")) {
            if (!pending_track_start &&
                ((audio_play_state == AUDIO_PAUSE && audio_play_request == AUDIO_RESUME) ||
                 (audio_play_request == AUDIO_PRE_PLAY)) && value >= AUDIO_WORK_FRAME_LAST) {
                audio_play_request = AUDIO_PLAY;
            }
            snprintf(buff, 60, "%swork\\%ld.bin",dir_path, (long)value);
            lv_img_set_src(target, buff);
        } else if (animation_mode_is(target, "D-O")) {
            if (value <= 9) {
                snprintf(buff, 60, "%sdrop\\%ld.bin",dir_path, (long)value);
                lv_img_set_src(target, buff);
            } else {
                snprintf(buff, 60, "%sopen\\%ld.bin",dir_path, value - 8);
                lv_img_set_src(target, buff);
            }
        } else if (animation_mode_is(target, "O-W")) {
            if (value <= 7) {
                snprintf(buff, 60, "%sopen\\%ld.bin",dir_path, value);
                lv_img_set_src(target, buff);
            }
        } else if (animation_mode_is(target, "O")) {
            snprintf(buff, 60, "%sopen\\%ld.bin",dir_path, value);
            lv_img_set_src(target, buff);
        } else if (animation_mode_is(target, "C")) {
            snprintf(buff, 60, "%sclose\\%ld.bin",dir_path, value);
            lv_img_set_src(target, buff);
        } else if (animation_mode_is(target, "D")) {
            snprintf(buff, 60, "%sdrop\\%ld.bin",dir_path, (long)value);
            lv_img_set_src(target, buff);
        }
    }
}

static void animation_end_callback(struct _lv_anim_t *anim) {
    lv_obj_t *img = anim->user_data;
    if (img) {
        if (lv_obj_get_user_data(img)) {
            if (animation_mode_is(img, "W")) {
                lv_anim_set_time(anim, AUDIO_WORK_ANIM_TIME);
                lv_anim_set_delay(anim, AUDIO_WORK_ANIM_DELAY);
                lv_anim_set_values(anim, AUDIO_WORK_FRAME_FIRST, AUDIO_WORK_FRAME_LAST);
                lv_anim_start(anim);
            } else if (animation_mode_is(img, "O-W")) {
                box_state = BOX_OPEN;
                lv_obj_set_user_data(img, "W");
                track_open_ready = true;
                try_start_selected_track();
                lv_anim_set_time(anim, AUDIO_WORK_ANIM_TIME);
                lv_anim_set_delay(anim, AUDIO_WORK_ANIM_DELAY);
                lv_anim_set_values(anim, AUDIO_WORK_FRAME_FIRST, AUDIO_WORK_FRAME_LAST);
                lv_anim_start(anim);
            } else if (animation_mode_is(img, "C")) {
                box_state = BOX_CLOSED;
                if(open_after_close) {
                    open_after_close = false;
                    startAnim(img, "O-W", 1500, 100, 1, 7);
                }
            } else if (animation_mode_is(img, "D")) {
                box_state = BOX_CLOSED;
            }
        }
    }
}

static void startAnim(lv_obj_t *tar, const char *mode, uint32_t duration, uint32_t delay, int32_t start, int32_t end) {
    lv_anim_del(tar, custom_anim_callback);
    if(strcmp(mode, "C") == 0) box_state = BOX_CLOSING;
    else if(strcmp(mode, "D") == 0) box_state = BOX_DROPPING;
    else if(strcmp(mode, "O-W") == 0 || strcmp(mode, "O") == 0 ||
            strcmp(mode, "D-O") == 0) box_state = BOX_OPENING;
    else if(strcmp(mode, "W") == 0 || strcmp(mode, "WE") == 0) box_state = BOX_OPEN;
    lv_anim_t anim_box;
    lv_anim_init(&anim_box);
    anim_box.user_data = tar;
    lv_obj_set_user_data(tar, (void *)mode);
    lv_anim_set_ready_cb(&anim_box, animation_end_callback);
    lv_anim_set_exec_cb(&anim_box, (lv_anim_exec_xcb_t) custom_anim_callback);
    lv_anim_set_var(&anim_box, tar); //pointer to a variable to animate
    lv_anim_set_time(&anim_box, duration);
    lv_anim_set_delay(&anim_box, delay);
    lv_anim_set_values(&anim_box, start, end);
    lv_anim_start(&anim_box);
}

static void start_selected_track(lv_obj_t *img)
{
    if(img == NULL) return;
    if(box_state == BOX_CLOSING) {
        /* Finish the existing close instead of jumping back to close/1.bin. */
        open_after_close = true;
    } else if(box_state == BOX_OPEN || box_state == BOX_OPENING) {
        open_after_close = true;
        startAnim(img, "C", 500, 0, 1, 7);
    } else {
        /* A closed box (including the initial drop) only needs to open. */
        open_after_close = false;
        startAnim(img, "O-W", 1500, 100, 1, 7);
    }
}

static void try_start_selected_track(void)
{
    if(!pending_track_start || !track_open_ready) return;

    /* AUDIO_NONE is published by the worker only after its WAV file is closed. */
    taskENTER_CRITICAL();
    if(audio_play_request == AUDIO_NONE) {
        memcpy(global_BUFF, selected_track_path, sizeof(selected_track_path));
        audio_play_request = AUDIO_PRE_PLAY;
        pending_track_start = false;
    }
    taskEXIT_CRITICAL();
}

static void queue_selected_track(lv_obj_t *img)
{
    pending_track_start = true;
    track_open_ready = false;

    taskENTER_CRITICAL();
    if(audio_play_state == AUDIO_PLAY || audio_play_state == AUDIO_PAUSE ||
       audio_play_state == AUDIO_PRE_PLAY) {
        audio_play_request = AUDIO_CANCEL;
    } else if(audio_play_request != AUDIO_CANCEL) {
        /* Also cancel a queued PLAY that the audio worker has not claimed yet. */
        audio_play_request = AUDIO_NONE;
    }
    taskEXIT_CRITICAL();

    /* Visual feedback starts on the first click, independently of the worker. */
    start_selected_track(img);
}

static void TimerCallBack(lv_timer_t *timer){
    lv_obj_t *btn = timer->user_data; // enter btn
    try_start_selected_track();

    if(pending_track_start || open_after_close || animation_mode_is(audio_img, "O-W")) {
        lv_label_set_text(btn, LV_SYMBOL_PAUSE);
    } else if(audio_play_state == AUDIO_PLAY) {
        lv_label_set_text(btn, LV_SYMBOL_PAUSE);
    } else if(audio_play_state == AUDIO_PAUSE) {
        lv_label_set_text(btn, LV_SYMBOL_PLAY);
    } else if(audio_play_state == AUDIO_NONE && audio_play_request == AUDIO_NONE &&
              audio_last_error != 0U) {
        lv_label_set_text(btn, LV_SYMBOL_PLAY);
        if(animation_mode_is(audio_img, "W")) startAnim(audio_img, "C", 500, 0, 1, 7);
    } else if(audio_play_state == AUDIO_END && audio_last_state != AUDIO_END) {
        startAnim(audio_img, "C", 500, 100, 1, 7);
        lv_label_set_text(btn, LV_SYMBOL_PLAY);
    }
    audio_last_state = audio_play_state;
}
static void click_event(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t *) lv_event_get_target(e);
    if (btn) {
        if (e->code == LV_EVENT_LONG_PRESSED) {
            audio_play_request = AUDIO_CANCEL;
            delete_ui(&ui_audio, NULL);

        } else if (e->code == LV_EVENT_SHORT_CLICKED) {
            lv_obj_t *title = lv_obj_get_child(lv_obj_get_child(ui_audio.screen, 0), 0);
            if (memcmp(lv_label_get_text(btn), LV_SYMBOL_PAUSE, 3) == 0) {
                lv_label_set_text(btn, LV_SYMBOL_PLAY);
                pending_track_start = false;
                track_open_ready = false;
                open_after_close = false;
                taskENTER_CRITICAL();
                if(audio_play_state == AUDIO_PLAY || audio_play_state == AUDIO_PAUSE ||
                   audio_play_state == AUDIO_PRE_PLAY) {
                    audio_play_request = AUDIO_PAUSE;
                } else if(audio_play_request != AUDIO_CANCEL) {
                    audio_play_request = AUDIO_NONE;
                }
                taskEXIT_CRITICAL();
                startAnim(audio_img, "C", 500, 500, 1, 7);
            } else if (memcmp(lv_label_get_text(btn), LV_SYMBOL_PLAY, 3) == 0) {
                lv_label_set_text(btn, LV_SYMBOL_PAUSE);
                if (audio_play_request == AUDIO_NONE) {
                    if (select_audio_file(title, file_index) == FR_OK) {
                        queue_selected_track(audio_img);
                    }
                }else{
                    audio_play_request = AUDIO_RESUME;
                    start_selected_track(audio_img);
                }

            } else if (memcmp(lv_label_get_text(btn), LV_SYMBOL_PREV, 3) == 0) {
                if (max_file_count == 0) return;
                if (file_index == 0) {
                    file_index = max_file_count;
                }
                file_index--;
                if (select_audio_file(title, file_index) == FR_OK) {
                    queue_selected_track(audio_img);
                    lv_label_set_text(lv_obj_get_child(lv_obj_get_parent(btn), 1), LV_SYMBOL_PAUSE);
                }
            } else if (memcmp(lv_label_get_text(btn), LV_SYMBOL_NEXT, 3) == 0) {
                if (max_file_count == 0) return;
                file_index++;
                if (file_index >= max_file_count)file_index = 0;
                if (select_audio_file(title, file_index) == FR_OK) {
                    queue_selected_track(audio_img);
                    lv_label_set_text(lv_obj_get_child(lv_obj_get_parent(btn), 1), LV_SYMBOL_PAUSE);
                }
            } else if (memcmp(lv_label_get_text(btn), LV_SYMBOL_LIST, 3) == 0) {
                setup_ui(&ui_play, &ui_audio, "AUDIO");
            }
        } else {
            if (e->code == LV_EVENT_FOCUSED) {

            } else if (e->code == LV_EVENT_DEFOCUSED) {

            }
        }
    }
}

static void setup_screen(void *user_data) {
    audio_play_request = AUDIO_NONE;
    audio_img = NULL;
    pending_track_start = false;
    track_open_ready = false;
    selected_track_path[0] = '\0';
    audio_last_state = audio_play_state;
    open_after_close = false;
    box_state = BOX_CLOSED;
    ui_audio.screen = lv_obj_create(NULL);
    lv_obj_set_scrollbar_mode(ui_audio.screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *div = lv_obj_create(ui_audio.screen);
    lv_obj_set_flex_flow(div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(div, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(div, LV_PCT(98), LV_PCT(98));
    lv_obj_set_pos(div, LV_PCT(1), LV_PCT(1));
    lv_obj_set_style_radius(div, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(div, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(div, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(div, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_row(div,0,LV_PART_MAIN|LV_PART_ITEMS);
    lv_obj_set_scrollbar_mode(div, LV_SCROLLBAR_MODE_OFF);
    resume_screen(NULL);

    lv_obj_t *title = lv_label_create(div);
    lv_obj_set_size(title, LV_PCT(100), 16);
    lv_obj_set_pos(title, LV_PCT(0), LV_PCT(0));
    lv_label_set_text(title, "Hey!Music~");
    lv_label_set_long_mode(title, LV_LABEL_LONG_SCROLL);
    lv_obj_set_content_height(title, 16);
//    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
//    lv_obj_set_style_border_width(title, 1, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(title, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(title, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN);


    lv_obj_t *img_div = lv_obj_create(div);
    lv_obj_set_size(img_div, LV_PCT(100), 64);
    lv_obj_set_style_pad_all(img_div, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(img_div, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(img_div, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(img_div, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(img_div, LV_SCROLLBAR_MODE_OFF);

    audio_img = lv_img_create(img_div);
    lv_obj_set_size(audio_img, 128, 85);
    lv_obj_align(audio_img, LV_ALIGN_TOP_MID, 0, -20);

    lv_obj_t *progress_bar = lv_bar_create(div);
    lv_bar_set_range(progress_bar, 0, 100); // 设置进度条的范围
    lv_bar_set_value(progress_bar, 30, LV_ANIM_ON); // 设置进度条的当前值
    lv_obj_set_size(progress_bar, 100, 5);
    lv_obj_set_style_bg_color(progress_bar, lv_color_make(0, 0, 0), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(progress_bar, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_border_width(progress_bar, 1, LV_PART_MAIN);

    lv_obj_t *button_array = lv_obj_create(div);
    lv_obj_set_size(button_array, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(button_array, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_array, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_scrollbar_mode(button_array, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(button_array, 2, LV_PART_MAIN);
    lv_obj_t *btn;
    const char *mode_text[] = {LV_SYMBOL_PREV, LV_SYMBOL_PLAY, LV_SYMBOL_NEXT, "", LV_SYMBOL_LIST};
    for (int i = 0; i < 5; i++) {
        btn = lv_label_create(button_array);
        if (i == 3)
            lv_obj_set_size(btn, 0, 14);
        else
            lv_obj_set_size(btn, 14, 14);
        lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
        lv_label_set_text(btn, mode_text[i]);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE); //make text clickable
        lv_obj_add_style(btn, &style_screen_list_extra_btns_main_default, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(btn, &style_screen_list_extra_btns_main_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_style(btn, &style_screen_list_extra_btns_main_focused, LV_PART_MAIN | LV_STATE_FOCUSED);

        lv_obj_add_event_cb(btn, click_event, LV_EVENT_SHORT_CLICKED, NULL);
        lv_obj_add_event_cb(btn, click_event, LV_EVENT_LONG_PRESSED, NULL);
//        lv_obj_add_event_cb(btn, click_event, LV_EVENT_FOCUSED, NULL);
//        lv_obj_add_event_cb(btn, click_event, LV_EVENT_DEFOCUSED, NULL);
        if (i != 3)
            lv_group_add_obj(lv_group_get_default(), btn);
    }
    startAnim(audio_img, "D", 1000, 100, 1, 8);
    max_file_count = count_audio_files();

    lv_obj_t *play_btn = lv_obj_get_child(button_array,1);
    lv_timer_t *timer = lv_timer_create(TimerCallBack, 500, play_btn);
    lv_timer_set_repeat_count(timer,-1); // infinity
    ui_audio.screen->user_data = timer; // when page deleted, timer in screen's userdata must be freed using 'lv_timer_del'
}

static void resume_screen(void *user_data) {
    if (user_data) {
        uint16_t index = *(uint16_t *) user_data;
        lv_obj_t *title = lv_obj_get_child(lv_obj_get_child(ui_audio.screen, 0), 0);
        if (title) {
            lv_obj_t *play_btn = lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(ui_audio.screen,0),3),1);
            if (max_file_count > 0 && index < max_file_count &&
                select_audio_file(title, index) == FR_OK) {
                file_index = index;
                queue_selected_track(audio_img);
                lv_label_set_text(play_btn, LV_SYMBOL_PAUSE);
            }
        }
        lv_mem_free(user_data);
        ui_audio.user_data = NULL;
    }
    lv_style_set_text_font(&style_screen_list_extra_btns_main_default, &lv_font_montserrat_14);
    lv_style_set_text_font(&style_screen_list_extra_btns_main_pressed, &lv_font_montserrat_14);
    lv_style_set_text_font(&style_screen_list_extra_btns_main_focused, &lv_font_montserrat_14);
}

static void delete_screen(void *user_data) {
    if(ui_audio.screen->user_data)
        lv_timer_del(ui_audio.screen->user_data);
    file_index = 0;
    max_file_count = 0;
    pending_track_start = false;
    track_open_ready = false;
    open_after_close = false;
    box_state = BOX_CLOSED;
    audio_img = NULL;
    HeapManager.reset();
}
