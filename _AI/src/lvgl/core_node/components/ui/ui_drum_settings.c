/*****************************************************************************
 * File        : ui_drum_settings.c
 * Component   : ui
 * Description : Per-drum settings screen for the ADLS Core Node.
 *
 *               Always displays the drum selected on the main screen
 *               (g_selected_drum → g_drums[g_selected_drum]).
 *               Call ui_drum_settings_refresh() before navigating here so
 *               the title and info panel are in sync with the selection.
 *
 *               Layout (1024×600, wallpaper4.bin background):
 *                 ← Back   (top-left, 12px from corner)
 *                 Title    "Drum Settings — <name>"  (top-centre, below back)
 *                 Info     "Mode: <X>  |  Color: #RRGGBB  |  Threshold: NNN"
 *                          (centre of screen)
 *                 [WIP]    Full settings controls (sliders, toggles) pending.
 *****************************************************************************/

#include "lvgl.h"
#include "ui_drum_settings.h"
#include "ui.h"

static lv_obj_t *scr        = NULL;
static lv_obj_t *lbl_title  = NULL;   // "Drum Settings — Kick"
static lv_obj_t *lbl_info   = NULL;   // "Mode: PULSE  |  Color: #FF2200  |  Threshold: 150"

static void cb_back(lv_event_t *e)
{
    ui_navigate_to(UI_SCREEN_MAIN);
}

void ui_drum_settings_init(const lv_img_dsc_t *bg_dsc)
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

    // Back button — upper-left corner.
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 140, 50);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 12, 12);
    lv_obj_add_event_cb(btn, cb_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(btn_lbl);

    // Title label — drum name, centred horizontally near top.
    lbl_title = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 20);

    // Info label — mode / color / threshold, centred on screen.
    lbl_info = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_info, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_info, lv_color_make(220, 220, 220), 0);
    lv_obj_align(lbl_info, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(lbl_info, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_info, 800);

    // WIP notice — will be replaced by sliders/toggles later.
    lv_obj_t *lbl_wip = lv_label_create(scr);
    lv_label_set_text(lbl_wip, "(Settings controls — WIP)");
    lv_obj_set_style_text_font(lbl_wip, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_wip, lv_color_make(140, 140, 140), 0);
    lv_obj_align(lbl_wip, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Populate with the current selection right away.
    ui_drum_settings_refresh();
}

void ui_drum_settings_refresh(void)
{
    if (!lbl_title || !lbl_info) return;

    ui_drum_t *d = &g_drums[g_selected_drum];

    lv_label_set_text_fmt(lbl_title, "Drum Settings \xe2\x80\x94 %s", d->name);

    if (d->connected) {
        lv_label_set_text_fmt(lbl_info,
            "Mode: %s    Color: #%06X    Threshold: %u",
            ui_mode_names[d->mode_idx],
            (unsigned int)d->color,
            d->threshold);
    } else {
        lv_label_set_text_fmt(lbl_info, "%s is not connected.", d->name);
    }
}

lv_obj_t *ui_drum_settings_get_screen(void)
{
    return scr;
}
