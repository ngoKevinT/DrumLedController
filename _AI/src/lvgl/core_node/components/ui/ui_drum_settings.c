/*****************************************************************************
 * File        : ui_drum_settings.c
 * Component   : ui
 * Description : Connected Drum Settings screen — stub.
 *
 *               Will contain: per-node sensitivity slider, retrigger guard
 *               (refractory period), and hit-curve mapping selector.
 *****************************************************************************/

#include "lvgl.h"
#include "ui_drum_settings.h"
#include "ui.h"

static lv_obj_t *scr = NULL;

static void cb_back(lv_event_t *e)
{
    ui_navigate_to(UI_SCREEN_MAIN);
}

void ui_drum_settings_init(void)
{
    scr = lv_obj_create(NULL);

    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, "Drum Settings\n(WIP)");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 160, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn, cb_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(btn_lbl);
}

lv_obj_t *ui_drum_settings_get_screen(void)
{
    return scr;
}
