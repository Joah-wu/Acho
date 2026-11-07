#include "page_flashlight.h"
#include "bsp_flashlight.h"

static lv_obj_t *state_label;
static void setup_screen(void *user_data);
static void delete_screen(void *user_data);

lv_ui ui_flashlight = {
    .gui_setup_func = setup_screen,
    .gui_delete_func = delete_screen
};

static void update_state(void)
{
    lv_label_set_text(state_label, Flashlight_IsEnabled() ? "ON" : "OFF");
}

static void click_event(lv_event_t *event)
{
    if (event->code == LV_EVENT_SHORT_CLICKED) {
        Flashlight_SetEnabled(!Flashlight_IsEnabled());
        update_state();
    } else if (event->code == LV_EVENT_LONG_PRESSED) {
        delete_ui(&ui_flashlight, NULL);
    }
}

static void setup_screen(void *user_data)
{
    lv_obj_t *title;
    lv_obj_t *hint;
    LV_UNUSED(user_data);

    Flashlight_SetEnabled(false);
    ui_flashlight.screen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_flashlight.screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(ui_flashlight.screen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_flashlight.screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_flashlight.screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_flashlight.screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui_flashlight.screen, &lv_font_montserrat_14, LV_PART_MAIN);

    title = lv_label_create(ui_flashlight.screen);
    lv_label_set_text(title, "Flashlight");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    state_label = lv_label_create(ui_flashlight.screen);
    lv_obj_align(state_label, LV_ALIGN_CENTER, 0, -4);
    update_state();

    hint = lv_label_create(ui_flashlight.screen);
    lv_label_set_text(hint, "Click: ON / OFF\nHold: OFF + Back");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_add_flag(ui_flashlight.screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui_flashlight.screen, click_event, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_flashlight.screen, click_event, LV_EVENT_LONG_PRESSED, NULL);
    lv_group_add_obj(lv_group_get_default(), ui_flashlight.screen);
}

static void delete_screen(void *user_data)
{
    LV_UNUSED(user_data);
    Flashlight_SetEnabled(false);
    state_label = NULL;
}
