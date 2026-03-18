/*****************************************************************************
 * File        : ui_global_settings.c
 * Component   : ui
 * Description : Global Settings screen for the ADLS Core Node.
 *
 *   Split into two panels (left ≈ 3/5 of 1024px screen, right ≈ 2/5):
 *
 *   LEFT  (x=16,  w=590) — settings that apply to every drum simultaneously:
 *     ── LED ────────────────────────────────────────────────
 *     Idle Brightness    [−] [=========·====] [+]   20 %
 *     Flash Brightness   [−] [===================] [+]  100 %
 *     Flash Time         [−] [====·              ] [+]  150 ms
 *     Decay Time         [−] [=======·           ] [+]  300 ms
 *
 *     ── GLOBAL ─────────────────────────────────────────────
 *     Master Brightness  [−] [============·     ] [+]   80 %
 *     Power Limit (3A)   [  OFF  ]  Cap LED draw to 3 A
 *
 *   RIGHT (x=618, w=398) — connection management and diagnostics:
 *     ── CONNECTION ──────────────────────────────────────────
 *     [ ENTER PAIRING MODE ]
 *     Status: Ready
 *
 *     ── CONNECTED DRUMS ─────────────────────────────────────
 *     ●  Kick                     (green = connected)
 *     ●  Snare
 *     ○  Slot 3                   (gray  = empty)
 *     ○  Slot 4
 *     ○  Slot 5
 *     ○  Slot 6
 *
 *     Debug: --
 *
 *                          [   SAVE   ]   ✓ Saved
 *
 *   Slider changes are written live to all drums (or the global) immediately;
 *   the Save button is the explicit confirmation to transmit over BLE/ESP-NOW.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "ui_global_settings.h"
#include "ui.h"

// ---------------------------------------------------------------------------
// Slider ranges
// ---------------------------------------------------------------------------

#define BRIGHTNESS_MIN    0
#define BRIGHTNESS_MAX  100
#define FTIME_MIN        10
#define FTIME_MAX      2000
#define DECAY_MIN        50
#define DECAY_MAX      3000
#define MASTER_MIN        0
#define MASTER_MAX      100

// Step sizes used by the [−] / [+] buttons
#define STEP_BRIGHTNESS   5
#define STEP_FTIME       50
#define STEP_DECAY       50

// ---------------------------------------------------------------------------
// Slider control descriptor
// ---------------------------------------------------------------------------

typedef enum {
    SLD_IDLE_BR    = 0,
    SLD_FLASH_BR,
    SLD_FLASH_TIME,
    SLD_DECAY_TIME,
    SLD_MASTER_BR,
    SLD_COUNT,
} sld_id_t;

typedef struct {
    lv_obj_t  *slider;
    lv_obj_t  *val_lbl;
    sld_id_t   id;
    int        step;
    int        min;
    int        max;
    bool       is_pct;   /**< true → "N %",  false → "N ms" */
} sld_ctl_t;

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------

static lv_obj_t  *scr              = NULL;
static sld_ctl_t  s_sld[SLD_COUNT];

static lv_obj_t  *sw_power         = NULL;

static lv_obj_t  *btn_pair         = NULL;
static lv_obj_t  *lbl_pair_status  = NULL;

static lv_obj_t  *s_drum_dot[UI_MAX_DRUMS];
static lv_obj_t  *s_drum_lbl[UI_MAX_DRUMS];

static lv_obj_t  *lbl_debug        = NULL;

static lv_obj_t  *lbl_saved        = NULL;
static lv_timer_t *save_hide_timer = NULL;

// ---------------------------------------------------------------------------
// Slider data write helpers
// LED sliders broadcast the same value to every drum simultaneously.
// ---------------------------------------------------------------------------

static void commit_sld(sld_id_t id, int v)
{
    int i;
    switch (id) {
    case SLD_IDLE_BR:
        for (i = 0; i < UI_MAX_DRUMS; i++) g_drums[i].idle_brightness  = (uint8_t)v;
        break;
    case SLD_FLASH_BR:
        for (i = 0; i < UI_MAX_DRUMS; i++) g_drums[i].flash_brightness = (uint8_t)v;
        break;
    case SLD_FLASH_TIME:
        for (i = 0; i < UI_MAX_DRUMS; i++) g_drums[i].flash_time_ms    = (uint16_t)v;
        break;
    case SLD_DECAY_TIME:
        for (i = 0; i < UI_MAX_DRUMS; i++) g_drums[i].decay_time_ms    = (uint16_t)v;
        break;
    case SLD_MASTER_BR:
        g_master_brightness = (uint8_t)v;
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Slider apply helper (clamping, widget update, global write)
// ---------------------------------------------------------------------------

static void apply_sld(sld_ctl_t *sc, int v)
{
    if (v < sc->min) v = sc->min;
    if (v > sc->max) v = sc->max;
    lv_slider_set_value(sc->slider, v, LV_ANIM_OFF);
    if (sc->is_pct) lv_label_set_text_fmt(sc->val_lbl, "%d %%", v);
    else            lv_label_set_text_fmt(sc->val_lbl, "%d ms", v);
    commit_sld(sc->id, v);
}

// ---------------------------------------------------------------------------
// Event callbacks
// ---------------------------------------------------------------------------

static void cb_back(lv_event_t *e) { ui_navigate_to(UI_SCREEN_MAIN); }

static void cb_sld_changed(lv_event_t *e)
{
    sld_ctl_t *sc = lv_event_get_user_data(e);
    apply_sld(sc, lv_slider_get_value(lv_event_get_target(e)));
}

static void cb_minus(lv_event_t *e)
{
    sld_ctl_t *sc = lv_event_get_user_data(e);
    apply_sld(sc, lv_slider_get_value(sc->slider) - sc->step);
}

static void cb_plus(lv_event_t *e)
{
    sld_ctl_t *sc = lv_event_get_user_data(e);
    apply_sld(sc, lv_slider_get_value(sc->slider) + sc->step);
}

static void cb_power(lv_event_t *e)
{
    g_power_limit_3a = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
}

/**
 * Pair button callback.  The button has LV_OBJ_FLAG_CHECKABLE so LVGL toggles
 * the LV_STATE_CHECKED bit before firing LV_EVENT_VALUE_CHANGED.
 */
static void cb_pair(lv_event_t *e)
{
    bool active = lv_obj_has_state(btn_pair, LV_STATE_CHECKED);
    if (active) {
        lv_label_set_text(lbl_pair_status, "Pairing active — waiting for drums...");
        // TODO: start BLE / ESP-NOW pairing broadcast
    } else {
        lv_label_set_text(lbl_pair_status, "Ready");
        // TODO: stop pairing scan
    }
}

static void save_hide_cb(lv_timer_t *t)
{
    save_hide_timer = NULL;
    lv_obj_add_flag(lbl_saved, LV_OBJ_FLAG_HIDDEN);
}

static void cb_save(lv_event_t *e)
{
    // All values are already committed live — Save is the explicit BLE/ESP-NOW
    // transmit trigger.
    // TODO: broadcast g_drums[] LED fields + g_master_brightness + g_power_limit_3a
    lv_obj_clear_flag(lbl_saved, LV_OBJ_FLAG_HIDDEN);
    if (save_hide_timer) { lv_timer_del(save_hide_timer); save_hide_timer = NULL; }
    save_hide_timer = lv_timer_create(save_hide_cb, 2000, NULL);
    lv_timer_set_repeat_count(save_hide_timer, 1);
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

static lv_obj_t *make_panel(lv_obj_t *parent, lv_coord_t pad_row)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(p, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_shadow_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_pad_row(p, pad_row, 0);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(p, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return p;
}

static lv_obj_t *make_row(lv_obj_t *parent, lv_coord_t h, lv_coord_t pad_col)
{
    lv_obj_t *r = lv_obj_create(parent);
    lv_obj_set_size(r, lv_pct(100), h);
    lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_shadow_width(r, 0, 0);
    lv_obj_set_style_pad_all(r, 0, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(r, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(r, pad_col, 0);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return r;
}

static void make_row_label(lv_obj_t *row, const char *text, lv_coord_t w)
{
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_width(lbl, w);
}

static lv_obj_t *make_value_label(lv_obj_t *row, lv_coord_t w)
{
    lv_obj_t *lbl = lv_label_create(row);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_make(180, 180, 180), 0);
    lv_obj_set_width(lbl, w);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_RIGHT, 0);
    return lbl;
}

static void make_section_header(lv_obj_t *panel, const char *text)
{
    lv_obj_t *hdr = lv_label_create(panel);
    lv_label_set_text(hdr, text);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hdr, lv_color_make(140, 190, 255), 0);
    lv_obj_set_width(hdr, lv_pct(100));
    lv_obj_set_style_text_align(hdr, LV_TEXT_ALIGN_CENTER, 0);
}

/** Small [−] or [+] button that adjusts slider sc by sc->step on click. */
static void make_pm_btn(lv_obj_t *row, const char *symbol,
                        lv_event_cb_t cb, sld_ctl_t *sc)
{
    lv_obj_t *btn = lv_btn_create(row);
    lv_obj_set_size(btn, 30, 30);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_radius(btn, 4, 0);
    // Muted styling — less dominant than primary action buttons
    lv_obj_set_style_bg_color(btn, lv_color_make(60, 80, 120), 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, sc);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, symbol);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl);
}

// ---------------------------------------------------------------------------
// Slider row builder
//
// Left panel is 3/5 of screen width: ⌊3/5 × 1024⌋ − 16px margin = 598px.
// Row layout (pad_col = 8, 4 gaps = 32px):
//   label(150) | [−](30) | slider(284) | [+](30) | value(64)
//   150 + 30 + 284 + 30 + 64 + 32 = 590 px ✓  (panel inner w = 590)
// ---------------------------------------------------------------------------

static void make_slider_row(lv_obj_t *panel, const char *label_txt, sld_id_t id,
                             int min, int max, int step, bool is_pct)
{
    sld_ctl_t *sc = &s_sld[id];
    sc->id     = id;
    sc->step   = step;
    sc->min    = min;
    sc->max    = max;
    sc->is_pct = is_pct;

    lv_obj_t *row = make_row(panel, 44, 8);
    make_row_label(row, label_txt, 150);

    make_pm_btn(row, "-", cb_minus, sc);

    lv_obj_t *sld = lv_slider_create(row);
    lv_slider_set_range(sld, min, max);
    lv_obj_set_size(sld, 284, 10);
    lv_obj_set_ext_click_area(sld, 14);
    lv_obj_add_event_cb(sld, cb_sld_changed, LV_EVENT_VALUE_CHANGED, sc);
    sc->slider = sld;

    make_pm_btn(row, "+", cb_plus, sc);

    sc->val_lbl = make_value_label(row, 64);
}

// ---------------------------------------------------------------------------
// Left panel — LED + global settings
// ---------------------------------------------------------------------------

static void build_led_panel(lv_obj_t *scr_obj)
{
    lv_obj_t *panel = make_panel(scr_obj, 14);
    lv_obj_set_pos(panel, 16, 70);
    lv_obj_set_size(panel, 590, 480);

    // ── LED ──────────────────────────────────────────────────────────────
    make_section_header(panel, "LED");

    make_slider_row(panel, "Idle Brightness",  SLD_IDLE_BR,
                    BRIGHTNESS_MIN, BRIGHTNESS_MAX, STEP_BRIGHTNESS, true);
    make_slider_row(panel, "Flash Brightness", SLD_FLASH_BR,
                    BRIGHTNESS_MIN, BRIGHTNESS_MAX, STEP_BRIGHTNESS, true);
    make_slider_row(panel, "Flash Time",       SLD_FLASH_TIME,
                    FTIME_MIN, FTIME_MAX, STEP_FTIME, false);
    make_slider_row(panel, "Decay Time",       SLD_DECAY_TIME,
                    DECAY_MIN, DECAY_MAX, STEP_DECAY, false);

    // ── GLOBAL ───────────────────────────────────────────────────────────
    make_section_header(panel, "GLOBAL");

    make_slider_row(panel, "Master Brightness", SLD_MASTER_BR,
                    MASTER_MIN, MASTER_MAX, STEP_BRIGHTNESS, true);

    // 3A Power Limit toggle row
    lv_obj_t *pwr_row = make_row(panel, 44, 12);
    make_row_label(pwr_row, "Power Limit (3A)", 150);
    sw_power = lv_switch_create(pwr_row);
    lv_obj_set_size(sw_power, 74, 32);
    lv_obj_add_event_cb(sw_power, cb_power, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *pwr_note = lv_label_create(pwr_row);
    lv_label_set_text(pwr_note, "Cap LED draw to 3 A");
    lv_obj_set_style_text_font(pwr_note, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(pwr_note, lv_color_make(140, 140, 140), 0);

    // Save row — button centred in the left panel; toast appears to its right.
    // make_row() defaults to START alignment; override to CENTER so the button
    // sits in the middle of the 590px panel when the toast is hidden.
    lv_obj_t *save_row = make_row(panel, 50, 20);
    lv_obj_set_flex_align(save_row,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *btn_save = lv_btn_create(save_row);
    lv_obj_set_size(btn_save, 200, 50);
    lv_obj_add_event_cb(btn_save, cb_save, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_lbl_inner = lv_label_create(btn_save);
    lv_label_set_text(save_lbl_inner, "SAVE");
    lv_obj_set_style_text_font(save_lbl_inner, &lv_font_montserrat_20, 0);
    lv_obj_center(save_lbl_inner);

    lbl_saved = lv_label_create(save_row);
    lv_label_set_text(lbl_saved, LV_SYMBOL_OK "  Saved");
    lv_obj_set_style_text_font(lbl_saved, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_saved, lv_color_make(80, 220, 120), 0);
    lv_obj_add_flag(lbl_saved, LV_OBJ_FLAG_HIDDEN);
}

// ---------------------------------------------------------------------------
// Right panel — connection management + drum status + debug
// ---------------------------------------------------------------------------

static void build_connection_panel(lv_obj_t *scr_obj)
{
    lv_obj_t *panel = make_panel(scr_obj, 12);
    lv_obj_set_pos(panel, 618, 70);
    lv_obj_set_size(panel, 398, 480);

    // ── CONNECTION ───────────────────────────────────────────────────────
    make_section_header(panel, "CONNECTION");

    // Pairing mode toggle button (CHECKABLE — state toggled automatically by LVGL)
    btn_pair = lv_btn_create(panel);
    lv_obj_set_size(btn_pair, lv_pct(100), 50);
    lv_obj_add_flag(btn_pair, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(btn_pair, cb_pair, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *pair_lbl_inner = lv_label_create(btn_pair);
    lv_label_set_text(pair_lbl_inner, LV_SYMBOL_WIFI "  ENTER PAIRING MODE");
    lv_obj_set_style_text_font(pair_lbl_inner, &lv_font_montserrat_16, 0);
    lv_obj_center(pair_lbl_inner);

    // Pairing status
    lbl_pair_status = lv_label_create(panel);
    lv_label_set_text(lbl_pair_status, "Status: Ready");
    lv_obj_set_style_text_font(lbl_pair_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_pair_status, lv_color_make(160, 160, 160), 0);

    // ── CONNECTED DRUMS ──────────────────────────────────────────────────
    make_section_header(panel, "CONNECTED DRUMS");

    // One row per drum slot: [●] [name]
    for (int i = 0; i < UI_MAX_DRUMS; i++) {
        lv_obj_t *row = make_row(panel, 32, 10);

        // Status dot — 14×14 filled circle, green or gray
        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_set_size(dot, 14, 14);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_shadow_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        s_drum_dot[i] = dot;

        // Drum name or "Slot N (empty)"
        lv_obj_t *name_lbl = lv_label_create(row);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_16, 0);
        s_drum_lbl[i] = name_lbl;
    }

    // Debug label — bottom of the right panel
    lbl_debug = lv_label_create(panel);
    lv_label_set_text(lbl_debug, "Debug: --");
    lv_obj_set_style_text_font(lbl_debug, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_debug, lv_color_make(120, 120, 120), 0);
    lv_obj_set_width(lbl_debug, lv_pct(100));
}

// ---------------------------------------------------------------------------
// Screen construction
// ---------------------------------------------------------------------------

void ui_global_settings_init(const lv_img_dsc_t *bg_dsc)
{
    scr = lv_obj_create(NULL);
    lv_obj_set_style_pad_all(scr, 0, 0);

    if (bg_dsc) {
        lv_obj_t *bg = lv_img_create(scr);
        lv_img_set_src(bg, bg_dsc);
        lv_obj_set_pos(bg, 0, 0);
        lv_obj_set_size(bg, 1024, 600);
        lv_img_set_zoom(bg, 256);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    }

    // Dark overlay for text legibility
    lv_obj_t *overlay = lv_obj_create(scr);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_size(overlay, 1024, 600);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_60, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // Back button — upper-left corner
    lv_obj_t *btn_back = lv_btn_create(scr);
    lv_obj_set_size(btn_back, 140, 50);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 12, 12);
    lv_obj_add_event_cb(btn_back, cb_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(btn_back);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(back_lbl);

    // Title — centred near top
    lv_obj_t *lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, "Global Settings");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 14);

    // Content panels
    build_led_panel(scr);
    build_connection_panel(scr);

    // Vertical divider line between the two panels
    lv_obj_t *divider = lv_obj_create(scr);
    lv_obj_set_pos(divider, 612, 70);
    lv_obj_set_size(divider, 2, 480);
    lv_obj_set_style_bg_color(divider, lv_color_make(80, 80, 80), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_60, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_radius(divider, 0, 0);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // Populate all widgets with current state
    ui_global_settings_refresh();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ui_global_settings_refresh(void)
{
    if (!s_sld[0].slider) return;   // not yet initialised

    // ── LED sliders — seed from drum 0 (after the first Save all drums share
    // the same value, so any index is representative)
    apply_sld(&s_sld[SLD_IDLE_BR],    g_drums[0].idle_brightness);
    apply_sld(&s_sld[SLD_FLASH_BR],   g_drums[0].flash_brightness);
    apply_sld(&s_sld[SLD_FLASH_TIME], g_drums[0].flash_time_ms);
    apply_sld(&s_sld[SLD_DECAY_TIME], g_drums[0].decay_time_ms);

    // ── Global controls
    apply_sld(&s_sld[SLD_MASTER_BR], g_master_brightness);

    if (g_power_limit_3a) lv_obj_add_state(sw_power,   LV_STATE_CHECKED);
    else                  lv_obj_clear_state(sw_power, LV_STATE_CHECKED);

    // ── Drum status list
    for (int i = 0; i < UI_MAX_DRUMS; i++) {
        if (g_drums[i].connected) {
            lv_obj_set_style_bg_color(s_drum_dot[i], lv_color_make(0, 200, 80), 0);
            lv_label_set_text(s_drum_lbl[i], g_drums[i].name);
            lv_obj_set_style_text_color(s_drum_lbl[i], lv_color_white(), 0);
        } else {
            lv_obj_set_style_bg_color(s_drum_dot[i], lv_color_make(70, 70, 70), 0);
            lv_label_set_text_fmt(s_drum_lbl[i], "Slot %d", i + 1);
            lv_obj_set_style_text_color(s_drum_lbl[i], lv_color_make(110, 110, 110), 0);
        }
    }

    // ── Pairing button — always reset to inactive when re-entering this screen
    lv_obj_clear_state(btn_pair, LV_STATE_CHECKED);
    lv_label_set_text(lbl_pair_status, "Status: Ready");

    // ── Hide any lingering save toast
    if (lbl_saved)       lv_obj_add_flag(lbl_saved, LV_OBJ_FLAG_HIDDEN);
    if (save_hide_timer) { lv_timer_del(save_hide_timer); save_hide_timer = NULL; }
}

lv_obj_t *ui_global_settings_get_screen(void)
{
    return scr;
}
