//
// Created by Mr.WU on 2023/9/30.
//

#include "page_main_select.h"
#include "../../broker_app.h"
#include "page_filelist.h"
#include "page_audio.h"
#include "page_play.h"
#include <stdio.h>
#include <string.h>

static void setup_screen(void *user_data);

static void resume_screen(void *user_data);

static lv_obj_t *preview_img;
static bool preview_exit_active;
static const char *pending_preview_mode;

#define PREVIEW_HOME_Y  (-5)

lv_ui ui_main_select = {
        .screen=NULL,
        .last_ui=NULL,
        .group=NULL,
        .is_init = false,
        .gui_setup_func = setup_screen,
        .gui_resume_func = resume_screen,
        .gui_delete_func = NULL
};

static void custom_anim_callback(void *obj, int32_t value) {
    char buff[64];
    lv_obj_t *target = obj;
    const char *mode_str = lv_obj_get_user_data(target);

    if (mode_str == NULL) return;

    if (strcmp(mode_str, "Audio") == 0) {
        snprintf(buff, sizeof(buff), "P:Static\\Animation\\main-selection\\music\\%ld.bin", (long)value);
        lv_img_set_src(target, buff);
    } else if (strcmp(mode_str, "Photo") == 0) {
        snprintf(buff, sizeof(buff), "P:Static\\Animation\\main-selection\\photo\\%ld.bin", (long)value);
        lv_img_set_src(target, buff);
    }
}

static void preview_start_enter(const char *mode_str)
{
    lv_anim_t anim_icon;

    if(preview_img == NULL) return;

    lv_anim_del(preview_img, NULL);
    lv_obj_set_user_data(preview_img, (void *)mode_str);
    lv_obj_set_y(preview_img, PREVIEW_HOME_Y);
    lv_obj_set_style_img_opa(preview_img, LV_OPA_100, LV_PART_MAIN);

    lv_anim_init(&anim_icon);
    lv_anim_set_var(&anim_icon, preview_img);
    lv_anim_set_exec_cb(&anim_icon, custom_anim_callback);
    lv_anim_set_time(&anim_icon, 400);
    lv_anim_set_delay(&anim_icon, 100);
    if(strcmp(mode_str, "Audio") == 0) {
        lv_anim_set_values(&anim_icon, 1, 9);
    } else if(strcmp(mode_str, "Photo") == 0) {
        lv_anim_set_values(&anim_icon, 1, 15);
    }
    lv_anim_start(&anim_icon);
}

static void preview_start_pending(void *user_data)
{
    const char *mode_str = pending_preview_mode;
    LV_UNUSED(user_data);

    pending_preview_mode = NULL;
    if(mode_str != NULL) preview_start_enter(mode_str);
}

static void preview_exit_ready_callback(struct _lv_anim_t *anim)
{
    LV_UNUSED(anim);

    preview_exit_active = false;
    if(pending_preview_mode != NULL) {
        lv_async_call(preview_start_pending, NULL);
    }
}

static void preview_start_exit(const char *mode_str)
{
    lv_anim_t anim_icon;

    if(preview_img == NULL || preview_exit_active) return;

    preview_exit_active = true;
    lv_anim_del(preview_img, NULL);
    lv_obj_set_user_data(preview_img, (void *)mode_str);

    lv_anim_init(&anim_icon);
    lv_anim_set_var(&anim_icon, preview_img);
    lv_anim_set_exec_cb(&anim_icon, custom_anim_callback);
    lv_anim_set_time(&anim_icon, 200);
    if(strcmp(mode_str, "Audio") == 0) {
        lv_anim_set_values(&anim_icon, 10, 13);
    } else if(strcmp(mode_str, "Photo") == 0) {
        lv_anim_set_values(&anim_icon, 16, 18);
    }
    lv_anim_set_ready_cb(&anim_icon, preview_exit_ready_callback);

    lv_anim_start(&anim_icon);
}

static void click_event(lv_event_t *e) {
    lv_obj_t *label_div = (lv_obj_t *) lv_event_get_target(e);
    if (label_div) {
        lv_anim_t anim_mode_text;
        const char *mode_str = "";
        lv_obj_t *mode_text = lv_obj_get_child(label_div, 0);
        if (mode_text)
            mode_str = lv_label_get_text(mode_text);
        if (e->code == LV_EVENT_LONG_PRESSED) {
            delete_ui(&ui_main_select, NULL);

        } else if (e->code == LV_EVENT_SHORT_CLICKED) {
            if(ui_main_select.screen->user_data){
                if(memcmp(ui_main_select.screen->user_data,"PLAY",4)==0){
                    if (memcmp(mode_str, "Audio", 6) == 0) {
                        setup_ui(&ui_audio,&ui_main_select,NULL);
                    }else if (memcmp(mode_str, "Photo", 6) == 0){
                        setup_ui(&ui_play,&ui_main_select,"IMG");
                    }
                }
                else if(memcmp(ui_main_select.screen->user_data,"DOWNLOAD",8)==0){
                    if (memcmp(mode_str, "Audio", 6) == 0) {
                        setup_ui(&ui_filelist,&ui_main_select,"AUDIO");
                    }else if (memcmp(mode_str, "Photo", 6) == 0){
                        setup_ui(&ui_filelist,&ui_main_select,"IMG");
                    }
                }

            }

        } else if(e->code == LV_EVENT_FOCUSED || e->code == LV_EVENT_DEFOCUSED) {
            lv_anim_init(&anim_mode_text);
            lv_anim_set_var(&anim_mode_text, label_div);
            lv_anim_set_time(&anim_mode_text, 300);
            lv_anim_set_delay(&anim_mode_text, 0);
            lv_anim_set_exec_cb(&anim_mode_text, (lv_anim_exec_xcb_t) lv_obj_set_x);
            if(preview_img == NULL) return;

            if (e->code == LV_EVENT_FOCUSED) {
                if (strcmp(mode_str, "Audio") == 0) {
                    lv_anim_set_values(&anim_mode_text, -128, 0);
                } else if (strcmp(mode_str, "Photo") == 0) {
                    lv_anim_set_values(&anim_mode_text, -128, 0);
                }
                lv_anim_start(&anim_mode_text);

                /* LVGL emits FOCUSED immediately after DEFOCUSED.  Keep only
                 * the latest requested preview until the exit is complete. */
                if(preview_exit_active) {
                    pending_preview_mode = mode_str;
                } else {
                    preview_start_enter(mode_str);
                }
            } else {
                if (strcmp(mode_str, "Audio") == 0) {
                    lv_anim_set_values(&anim_mode_text, 0, 128);
                } else if (strcmp(mode_str, "Photo") == 0) {
                    lv_anim_set_values(&anim_mode_text, 0, 128);
                }
                lv_anim_start(&anim_mode_text);
                preview_start_exit(mode_str);
            }
        }

    }


}

static void setup_screen(void *user_data) {
    preview_exit_active = false;
    pending_preview_mode = NULL;
    ui_main_select.screen = lv_obj_create(NULL);
    lv_obj_set_user_data(ui_main_select.screen,user_data);
    lv_obj_set_scrollbar_mode(ui_main_select.screen,LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *div = lv_obj_create(ui_main_select.screen);
    lv_obj_set_flex_flow(div, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(div, 2, LV_STATE_DEFAULT);
    lv_obj_set_size(div, LV_PCT(96), LV_PCT(96));
    lv_obj_set_pos(div, LV_PCT(2), LV_PCT(2));
    lv_obj_set_style_radius(div, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(div, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(div, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(div, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(div, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(div, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(div, 4, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(div, LV_SCROLLBAR_MODE_OFF);
    resume_screen(NULL);

    lv_obj_t *title = lv_label_create(div);
    lv_obj_set_pos(title, LV_PCT(0), LV_PCT(0));
    lv_obj_set_size(title, LV_PCT(100), 16);
    lv_obj_set_content_height(title, 16);
    lv_label_set_text(title, "Select Mode");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_set_style_bg_color(title, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(title, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN);


    lv_obj_t *img_div = lv_obj_create(div);
    lv_obj_set_size(img_div, LV_PCT(100), 52);
    lv_obj_set_style_pad_all(img_div, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(img_div, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(img_div, LV_BORDER_SIDE_TOP|LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(img_div, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(img_div,LV_SCROLLBAR_MODE_OFF);
    lv_obj_align_to(img_div, title,LV_ALIGN_BOTTOM_MID,0,-5);

    preview_img = lv_img_create(img_div);
    lv_obj_set_size(preview_img, 64, 64);
    lv_obj_align(preview_img, LV_ALIGN_TOP_MID, 0, PREVIEW_HOME_Y);
    lv_img_set_src(preview_img, "P:Static\\Animation\\main-selection\\music\\1.bin");

    const char *mode_text[] = {"Audio", "Photo"};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *label_div = lv_obj_create(ui_main_select.screen);
        lv_obj_set_size(label_div, LV_PCT(60), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(label_div, 1, LV_PART_MAIN);
        lv_obj_t *label_mode = lv_label_create(label_div);
        lv_label_set_text(label_mode, mode_text[i]);
        lv_obj_center(label_mode);

        lv_obj_align(label_div, LV_ALIGN_BOTTOM_MID, 0, -15);
        lv_obj_add_style(label_div, &style_screen_list_extra_btns_main_default, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(label_div, &style_screen_list_extra_btns_main_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_style(label_div, &style_screen_list_extra_btns_main_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_set_x(label_div, -128);

        lv_obj_add_event_cb(label_div, click_event, LV_EVENT_SHORT_CLICKED, NULL);
        lv_obj_add_event_cb(label_div, click_event, LV_EVENT_LONG_PRESSED, NULL);
        lv_obj_add_event_cb(label_div, click_event, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(label_div, click_event, LV_EVENT_DEFOCUSED, NULL);
        lv_group_add_obj(lv_group_get_default(), label_div);

    }


}

static void resume_screen(void *user_data) {
    lv_style_set_text_font(&style_screen_list_extra_btns_main_default, &lv_font_montserrat_14);
    lv_style_set_text_font(&style_screen_list_extra_btns_main_pressed, &lv_font_montserrat_14);
    lv_style_set_text_font(&style_screen_list_extra_btns_main_focused, &lv_font_montserrat_14);
}
