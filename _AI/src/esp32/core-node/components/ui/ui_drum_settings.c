/*****************************************************************************
 * File        : ui_drum_settings.c
 * Component   : ui
 * Description : Per-drum trigger settings screen for the ADLS Core Node.
 *
 *   Always reflects g_drums[g_selected_drum].  Call ui_drum_settings_refresh()
 *   before navigating here so all widgets are in sync with the selection.
 *
 *   Layout (1024×600, wallpaper4.bin + dark overlay for legibility):
 *
 *     ← Back          Drum Settings — Kick
 *
 *     Drum Name   [ Kick____________________________ ]
 *
 *     ── TRIGGER ──────────────────────────────────────
 *     Sensitivity     [==================·   ]   512
 *     Retrigger Guard [======·            ]    30 ms
 *     Hit Curve       [ LINEAR ]  [ LOG ]  [ EXP ]
 *
 *                          [   SAVE   ]   ✓ Saved
 *
 *     [ keyboard — hidden; appears at bottom when name field is focused ]
 *
 *   Saving:
 *     Writes textarea text back to g_drums[g_selected_drum].name.
 *     Slider values are written to g_drums[] live on change so the drum
 *     monitor card on the main screen stays in sync immediately.
 *     The Save button commits the name edit and shows the confirmation.
 *     TODO: transmit the updated struct to the drum node via BLE/ESP-NOW.
 *****************************************************************************/

#include <string.h>
#include "lvgl.h"
#include "ui_drum_settings.h"
#include "ui.h"

// ---------------------------------------------------------------------------
// Slider ranges
// ---------------------------------------------------------------------------

#define THRESH_MIN        0
#define THRESH_MAX     1023
#define RETRIG_MIN        0
#define RETRIG_MAX      500

// ---------------------------------------------------------------------------
// Module constants
// ---------------------------------------------------------------------------

static const char *const k_curve_lbl[HIT_CURVE_COUNT] = { "LINEAR", "LOG", "EXP" };

/** Context passed to the ± button callbacks for a slider. */
typedef struct { lv_obj_t **sld; int step; int min; int max; } pm_sld_t;

// ---------------------------------------------------------------------------
// Module-level widget references
// ---------------------------------------------------------------------------

static lv_obj_t  *scr              = NULL;
static lv_obj_t  *lbl_title        = NULL;

// Name input
static lv_obj_t  *ta_name          = NULL;
static lv_obj_t  *keyboard         = NULL;

// Trigger sliders
static lv_obj_t  *sld_sensitivity  = NULL;
static lv_obj_t  *lbl_sens_val     = NULL;
static lv_obj_t  *sld_retrigger    = NULL;
static lv_obj_t  *lbl_retrig_val   = NULL;

// ± button descriptors — filled during init, used by pm_minus/pm_plus callbacks
static pm_sld_t   pm_sens;
static pm_sld_t   pm_retr;

// Hit curve segmented buttons
static lv_obj_t  *btn_curve[HIT_CURVE_COUNT];

// Save confirmation
static lv_obj_t  *lbl_saved        = NULL;
static lv_timer_t *save_hide_timer = NULL;

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

/**
 * Transparent, non-interactive flex-column container.
 * pad_row controls the gap between direct children.
 */
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

/**
 * Transparent flex-row container, full-width, fixed height.
 * Children are packed left with pad_column gaps (START alignment).
 */
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

/** Row label (left side): caller-specified width, white, Montserrat-16. */
static void make_row_label(lv_obj_t *row, const char *text, lv_coord_t w)
{
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_width(lbl, w);
}

/** Value label (right side of slider row): fixed width, right-aligned, muted. */
static lv_obj_t *make_value_label(lv_obj_t *row)
{
    lv_obj_t *lbl = lv_label_create(row);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_make(180, 180, 180), 0);
    lv_obj_set_width(lbl, 80);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_RIGHT, 0);
    return lbl;
}

/**
 * Slider pre-sized for a setting row with ± buttons (340 × 10px).
 *
 * Row pixel budget (panel w=700, pad_col=10, 4 gaps=40px):
 *   label(180) + [−](30) + slider(340) + [+](30) + value(80) + 40 = 700 px ✓
 */
static lv_obj_t *make_slider(lv_obj_t *row, int min, int max, int val, lv_event_cb_t cb)
{
    lv_obj_t *sld = lv_slider_create(row);
    lv_slider_set_range(sld, min, max);
    lv_slider_set_value(sld, val, LV_ANIM_OFF);
    lv_obj_set_size(sld, 340, 10);
    lv_obj_set_ext_click_area(sld, 14);
    lv_obj_add_event_cb(sld, cb, LV_EVENT_VALUE_CHANGED, NULL);
    return sld;
}

/** Small [−] or [+] button that adjusts a slider via pm_sld_t. */
static void pm_minus(lv_event_t *e)
{
    pm_sld_t *p = lv_event_get_user_data(e);
    int v = lv_slider_get_value(*p->sld) - p->step;
    if (v < p->min) v = p->min;
    lv_slider_set_value(*p->sld, v, LV_ANIM_OFF);
    lv_event_send(*p->sld, LV_EVENT_VALUE_CHANGED, NULL);
}

static void pm_plus(lv_event_t *e)
{
    pm_sld_t *p = lv_event_get_user_data(e);
    int v = lv_slider_get_value(*p->sld) + p->step;
    if (v > p->max) v = p->max;
    lv_slider_set_value(*p->sld, v, LV_ANIM_OFF);
    lv_event_send(*p->sld, LV_EVENT_VALUE_CHANGED, NULL);
}

static void make_pm_btn(lv_obj_t *row, const char *symbol,
                        lv_event_cb_t cb, pm_sld_t *ctx)
{
    lv_obj_t *btn = lv_btn_create(row);
    lv_obj_set_size(btn, 30, 30);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_radius(btn, 4, 0);
    lv_obj_set_style_bg_color(btn, lv_color_make(60, 80, 120), 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, ctx);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, symbol);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl);
}

// ---------------------------------------------------------------------------
// Event callbacks
// ---------------------------------------------------------------------------

static void cb_back(lv_event_t *e)
{
    ui_navigate_to(UI_SCREEN_MAIN);
}

static void cb_sensitivity(lv_event_t *e)
{
    int v = lv_slider_get_value(lv_event_get_target(e));
    g_drums[g_selected_drum].threshold = (uint16_t)v;
    lv_label_set_text_fmt(lbl_sens_val, "%d", v);
}

static void cb_retrigger(lv_event_t *e)
{
    int v = lv_slider_get_value(lv_event_get_target(e));
    g_drums[g_selected_drum].retrigger_ms = (uint16_t)v;
    lv_label_set_text_fmt(lbl_retrig_val, "%d ms", v);
}

static void apply_hit_curve(uint8_t idx)
{
    g_drums[g_selected_drum].hit_curve = idx;
    for (int i = 0; i < HIT_CURVE_COUNT; i++) {
        if (i == (int)idx) lv_obj_add_state(btn_curve[i], LV_STATE_CHECKED);
        else               lv_obj_clear_state(btn_curve[i], LV_STATE_CHECKED);
    }
}

static void cb_curve(lv_event_t *e)
{
    apply_hit_curve((uint8_t)(intptr_t)lv_event_get_user_data(e));
}

// Keyboard: hide on OK or Cancel
static void kb_event_cb(lv_event_t *e)
{
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(keyboard, NULL);
}

// Textarea: show keyboard when focused
static void ta_focused_cb(lv_event_t *e)
{
    lv_keyboard_set_textarea(keyboard, ta_name);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

static void save_hide_cb(lv_timer_t *t)
{
    save_hide_timer = NULL;
    lv_obj_add_flag(lbl_saved, LV_OBJ_FLAG_HIDDEN);
}

static void cb_save(lv_event_t *e)
{
    // Commit the name from the textarea
    const char *new_name = lv_textarea_get_text(ta_name);
    if (new_name && new_name[0] != '\0') {
        strncpy(g_drums[g_selected_drum].name, new_name,
                sizeof(g_drums[0].name) - 1);
        g_drums[g_selected_drum].name[sizeof(g_drums[0].name) - 1] = '\0';
    }

    // Hide keyboard if still open
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(keyboard, NULL);

    // TODO: transmit g_drums[g_selected_drum] to drum node via BLE/ESP-NOW

    // Show "✓ Saved" for 2 seconds
    lv_obj_clear_flag(lbl_saved, LV_OBJ_FLAG_HIDDEN);
    if (save_hide_timer) {
        lv_timer_del(save_hide_timer);
        save_hide_timer = NULL;
    }
    save_hide_timer = lv_timer_create(save_hide_cb, 2000, NULL);
    lv_timer_set_repeat_count(save_hide_timer, 1);
}

// ---------------------------------------------------------------------------
// Screen construction
// ---------------------------------------------------------------------------

void ui_drum_settings_init(const lv_img_dsc_t *bg_dsc)
{
    scr = lv_obj_create(NULL);
    lv_obj_set_style_pad_all(scr, 0, 0);

    // Wallpaper background (PSRAM-resident)
    if (bg_dsc) {
        lv_obj_t *bg = lv_img_create(scr);
        lv_img_set_src(bg, bg_dsc);
        lv_obj_set_pos(bg, 0, 0);
        lv_obj_set_size(bg, 1024, 600);
        lv_img_set_zoom(bg, 256);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    }

    // Semi-transparent dark overlay — improves text legibility over wallpaper
    lv_obj_t *overlay = lv_obj_create(scr);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_size(overlay, 1024, 600);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_60, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // ------------------------------------------------------------------
    // Back button — upper-left corner
    // ------------------------------------------------------------------
    lv_obj_t *btn_back = lv_btn_create(scr);
    lv_obj_set_size(btn_back, 140, 50);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 12, 12);
    lv_obj_add_event_cb(btn_back, cb_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(btn_back);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(back_lbl);

    // ------------------------------------------------------------------
    // Title — centred near top
    // ------------------------------------------------------------------
    lbl_title = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 18);

    // ------------------------------------------------------------------
    // Content panel — centred horizontally, starts below title.
    // Width 700px → x = (1024-700)/2 = 162.
    // All setting rows are children of this flex-column panel.
    // ------------------------------------------------------------------
    lv_obj_t *panel = make_panel(scr, 20);
    lv_obj_set_pos(panel, 162, 78);
    lv_obj_set_size(panel, 700, 430);

    // --- Drum Name -------------------------------------------------------
    {
        lv_obj_t *row = make_row(panel, 50, 16);
        make_row_label(row, "Drum Name", 200);

        ta_name = lv_textarea_create(row);
        lv_textarea_set_one_line(ta_name, true);
        lv_textarea_set_max_length(ta_name, sizeof(g_drums[0].name) - 1);
        lv_obj_set_width(ta_name, 460);
        lv_obj_set_style_text_font(ta_name, &lv_font_montserrat_20, 0);
        lv_obj_add_event_cb(ta_name, ta_focused_cb, LV_EVENT_FOCUSED, NULL);
    }

    // --- TRIGGER section header ------------------------------------------
    {
        lv_obj_t *hdr = lv_label_create(panel);
        lv_label_set_text(hdr, "TRIGGER");
        lv_obj_set_style_text_font(hdr, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(hdr, lv_color_make(140, 190, 255), 0);
        lv_obj_set_width(hdr, lv_pct(100));
        lv_obj_set_style_text_align(hdr, LV_TEXT_ALIGN_CENTER, 0);
    }

    // --- Sensitivity -----------------------------------------------------
    {
        pm_sens = (pm_sld_t){ &sld_sensitivity, 10, THRESH_MIN, THRESH_MAX };
        lv_obj_t *row = make_row(panel, 44, 10);
        make_row_label(row, "Sensitivity", 180);
        make_pm_btn(row, "-", pm_minus, &pm_sens);
        sld_sensitivity = make_slider(row, THRESH_MIN, THRESH_MAX, 0, cb_sensitivity);
        make_pm_btn(row, "+", pm_plus,  &pm_sens);
        lbl_sens_val    = make_value_label(row);
    }

    // --- Retrigger Guard -------------------------------------------------
    {
        pm_retr = (pm_sld_t){ &sld_retrigger, 5, RETRIG_MIN, RETRIG_MAX };
        lv_obj_t *row = make_row(panel, 44, 10);
        make_row_label(row, "Retrigger Guard", 180);
        make_pm_btn(row, "-", pm_minus, &pm_retr);
        sld_retrigger  = make_slider(row, RETRIG_MIN, RETRIG_MAX, 0, cb_retrigger);
        make_pm_btn(row, "+", pm_plus,  &pm_retr);
        lbl_retrig_val = make_value_label(row);
    }

    // --- Hit Curve — segmented button row --------------------------------
    //
    // Three LV_OBJ_FLAG_CHECKABLE buttons act as a mutually-exclusive
    // segmented control.  apply_hit_curve() enforces single-selection.
    {
        lv_obj_t *row = make_row(panel, 50, 16);
        make_row_label(row, "Hit Curve", 180);

        // Container for the three buttons (flex row, SPACE_BETWEEN)
        lv_obj_t *grp = lv_obj_create(row);
        lv_obj_set_size(grp, 460, 44);
        lv_obj_set_style_bg_opa(grp, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(grp, 0, 0);
        lv_obj_set_style_shadow_width(grp, 0, 0);
        lv_obj_set_style_pad_all(grp, 0, 0);
        lv_obj_clear_flag(grp, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_layout(grp, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(grp, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(grp, LV_FLEX_ALIGN_SPACE_BETWEEN,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        for (int i = 0; i < HIT_CURVE_COUNT; i++) {
            lv_obj_t *btn = lv_btn_create(grp);
            lv_obj_set_size(btn, 142, 40);
            lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_add_event_cb(btn, cb_curve, LV_EVENT_CLICKED, (void *)(intptr_t)i);
            lv_obj_t *lbl = lv_label_create(btn);
            lv_label_set_text(lbl, k_curve_lbl[i]);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
            lv_obj_center(lbl);
            btn_curve[i] = btn;
        }
    }

    // ------------------------------------------------------------------
    // Save button + confirmation label
    // ------------------------------------------------------------------
    lv_obj_t *btn_save = lv_btn_create(scr);
    lv_obj_set_size(btn_save, 200, 50);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_add_event_cb(btn_save, cb_save, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_lbl = lv_label_create(btn_save);
    lv_label_set_text(save_lbl, "SAVE");
    lv_obj_set_style_text_font(save_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(save_lbl);

    // "✓ Saved" — shown for 2 s after a successful save, hidden otherwise
    lbl_saved = lv_label_create(scr);
    lv_label_set_text(lbl_saved, LV_SYMBOL_OK "  Saved");
    lv_obj_set_style_text_font(lbl_saved, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_saved, lv_color_make(80, 220, 120), 0);
    lv_obj_align(lbl_saved, LV_ALIGN_BOTTOM_MID, 0, -74);
    lv_obj_add_flag(lbl_saved, LV_OBJ_FLAG_HIDDEN);

    // ------------------------------------------------------------------
    // On-screen keyboard — hidden until the name textarea is focused.
    // Positioned at the bottom of the screen.
    // ------------------------------------------------------------------
    keyboard = lv_keyboard_create(scr);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(keyboard, kb_event_cb, LV_EVENT_READY,  NULL);
    lv_obj_add_event_cb(keyboard, kb_event_cb, LV_EVENT_CANCEL, NULL);

    // Populate all widgets with the currently selected drum's values
    ui_drum_settings_refresh();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ui_drum_settings_refresh(void)
{
    if (!lbl_title || !sld_sensitivity) return;

    ui_drum_t *d = &g_drums[g_selected_drum];

    // Title
    lv_label_set_text_fmt(lbl_title, "Drum Settings: %s", d->name);

    // Name textarea
    lv_textarea_set_text(ta_name, d->name);

    // Sensitivity
    lv_slider_set_value(sld_sensitivity, d->threshold, LV_ANIM_OFF);
    lv_label_set_text_fmt(lbl_sens_val, "%u", d->threshold);

    // Retrigger Guard
    lv_slider_set_value(sld_retrigger, d->retrigger_ms, LV_ANIM_OFF);
    lv_label_set_text_fmt(lbl_retrig_val, "%u ms", d->retrigger_ms);

    // Hit Curve segmented buttons
    for (int i = 0; i < HIT_CURVE_COUNT; i++) {
        if (i == (int)d->hit_curve) lv_obj_add_state(btn_curve[i], LV_STATE_CHECKED);
        else                        lv_obj_clear_state(btn_curve[i], LV_STATE_CHECKED);
    }

    // Hide the saved confirmation when switching to a different drum
    if (lbl_saved) lv_obj_add_flag(lbl_saved, LV_OBJ_FLAG_HIDDEN);
    if (save_hide_timer) {
        lv_timer_del(save_hide_timer);
        save_hide_timer = NULL;
    }

    // Hide keyboard if it was left open from a previous visit
    if (keyboard) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(keyboard, NULL);
    }
}

lv_obj_t *ui_drum_settings_get_screen(void)
{
    return scr;
}
