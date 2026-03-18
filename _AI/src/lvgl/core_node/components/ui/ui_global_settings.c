/*****************************************************************************
 * File        : ui_global_settings.c
 * Component   : ui
 * Description : Global Settings screen.
 *
 *               Will contain: master brightness slider, 3.0A power-limit
 *               toggle, and drum node pairing menu.
 *
 *               Layout:
 *                 • wallpaper1.bin — full-screen background
 *                 • "Global Settings" title — centred, white, Montserrat-24
 *                 • ← Back button — upper-left, 12px from corner
 *****************************************************************************/

#include "lvgl.h"
#include "ui_global_settings.h"
#include "ui.h"

static lv_obj_t *scr = NULL;

static void cb_back(lv_event_t *e)
{
    ui_navigate_to(UI_SCREEN_MAIN);
}

void ui_global_settings_init(const lv_img_dsc_t *bg_dsc)
{
    scr = lv_obj_create(NULL);
    lv_obj_set_style_pad_all(scr, 0, 0);

    // Full-screen wallpaper background (PSRAM-resident, no FS seek at draw time).
    if (bg_dsc) {
        lv_obj_t *bg = lv_img_create(scr);
        lv_img_set_src(bg, bg_dsc);
        lv_obj_set_pos(bg, 0, 0);
        lv_obj_set_size(bg, 1024, 600);
        lv_img_set_zoom(bg, 256);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    }

    // Title label — centred on screen, white text.
    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, "Global Settings");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

    // Back button — upper-left corner.
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 140, 50);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 12, 12);
    lv_obj_add_event_cb(btn, cb_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(btn_lbl);
}

lv_obj_t *ui_global_settings_get_screen(void)
{
    return scr;
}
