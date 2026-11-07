#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define MY_DISP_HOR_RES 128
#define MY_DISP_VER_RES 128
#define LV_IMG_CF_INDEXED_1BIT 7
#define LV_OBJ_FLAG_HIDDEN 1
#define LV_OBJ_FLAG_SCROLLABLE 2
#define LV_OBJ_FLAG_CLICKABLE 4
#define LV_OPA_COVER 255
#define LV_TEXT_ALIGN_CENTER 1
#define LV_EVENT_SHORT_CLICKED 1
#define LV_EVENT_LONG_PRESSED 2
typedef int lv_style_t;
typedef int lv_group_t;
typedef struct lv_obj_t { struct lv_obj_t *parent; int flags; bool deleted; int x,y,w; char text[192]; const void *source; } lv_obj_t;
typedef struct { int code; void *user_data; } lv_event_t;
#define lv_event_get_user_data(e) ((e)->user_data)
typedef struct lv_timer_t { void (*callback)(struct lv_timer_t*); } lv_timer_t;
typedef struct { unsigned always_zero,w,h,cf; } lv_img_header_t;
typedef uint8_t lv_res_t;
enum { LV_RES_INV,LV_RES_OK };
typedef struct { struct { unsigned always_zero,w,h,cf; } header; uint32_t data_size; const uint8_t *data; } lv_img_dsc_t;
extern const int lv_font_montserrat_10;
uint32_t lv_tick_get(void);
uint32_t lv_tick_elaps(uint32_t tick);
lv_obj_t *lv_scr_act(void);
lv_obj_t *lv_obj_create(lv_obj_t *parent);
lv_obj_t *lv_label_create(lv_obj_t *parent);
lv_obj_t *lv_img_create(lv_obj_t *parent);
lv_obj_t *lv_btn_create(lv_obj_t *parent);
void lv_obj_set_size(lv_obj_t*,int,int);
void lv_obj_center(lv_obj_t*);
void lv_timer_del(lv_timer_t*);
#define lv_group_remove_all_objs(...) ((void)0)
void lv_obj_add_flag(lv_obj_t*,int);
void lv_obj_clear_flag(lv_obj_t*,int);
void lv_label_set_text(lv_obj_t*,const char*);
void lv_obj_set_pos(lv_obj_t*,int,int);
void lv_obj_set_width(lv_obj_t*,int);
void lv_img_set_src(lv_obj_t*,const void*);
lv_res_t lv_img_decoder_get_info(const void*,lv_img_header_t*);
void lv_img_cache_invalidate_src(const void*);
lv_timer_t *lv_timer_create(void(*callback)(lv_timer_t*),uint32_t,void*);
#define lv_color_white() 1
#define lv_color_black() 0
#define lv_obj_set_style_pad_all(...) ((void)0)
#define lv_obj_set_style_bg_color(...) ((void)0)
#define lv_obj_set_style_bg_opa(...) ((void)0)
#define lv_obj_set_style_text_color(...) ((void)0)
#define lv_obj_set_style_text_font(...) ((void)0)
#define lv_obj_set_style_text_align(...) ((void)0)
#define lv_obj_set_style_border_width(...) ((void)0)
#define lv_obj_set_style_border_color(...) ((void)0)
#define lv_obj_set_style_radius(...) ((void)0)
#define lv_obj_remove_style_all(...) ((void)0)
#define lv_obj_add_event_cb(...) ((void)0)
#define lv_group_add_obj(...) ((void)0)
