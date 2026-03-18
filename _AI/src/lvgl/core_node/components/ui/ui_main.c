/*****************************************************************************
 * File        : ui_main.c
 * Component   : ui
 * Description : Main dashboard screen for the ADLS Core Node (1024×600).
 *
 *               The screen is split into two 512px vertical halves:
 *
 *               LEFT  — Control panel (implemented here)
 *               RIGHT — Drum monitor  (placeholder; implemented separately)
 *
 *   Left panel layout  (inner content area: 480×568 after 16px padding)
 *   All 5 items are direct flex children — SPACE_BETWEEN distributes the
 *   remaining 104px as four equal 26px gaps, filling the panel top-to-bottom.
 *
 *   ┌────────────────────────────────────────────┐  ↑
 *   │  [ MODE UP  ]        [ COLOR UP  ]         │  120px
 *   ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤  26px gap
 *   │  [ MODE DOWN]        [ COLOR DOWN]         │  120px
 *   ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤  26px gap
 *   │  [      SYNC COLOR & MODE        ]         │   80px
 *   ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤  26px gap
 *   │  ┌─────────── status label ──────────────┐ │   64px
 *   │  └────────────────────────────────────────┘ │
 *   ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤  26px gap
 *   │  [   ←   ]      [    SET    ]      [  →  ] │   80px
 *   └────────────────────────────────────────────┘  ↓
 *
 *   Pixel budget check (inner h = 568):
 *     Content : 120 + 120 + 80 + 64 + 80 = 464 px
 *     Gaps    : (568 − 464) / 4           = 26 px each  ✓
 *
 *   Bottom-right overlay (on top of wallpaper, right half):
 *     Two 64×64 icon buttons, 12px from bottom-right corner.
 *     Global Settings  → "S:/settings.png"
 *     Drum Settings    → "S:/settings_drum.png"
 *****************************************************************************/

#include "lvgl.h"
#include "ui_main.h"
#include "ui.h"

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------

static lv_obj_t *scr_main   = NULL;
static lv_obj_t *lbl_status = NULL;   // Shows the name of the last-pressed action

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

/**
 * Create a transparent, non-scrollable container suitable for use as a
 * flex layout wrapper. Clears the default LVGL background/border/shadow.
 */
static lv_obj_t *make_container(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(obj,     LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0,           0);
    lv_obj_set_style_shadow_width(obj, 0,           0);
    lv_obj_set_style_pad_all(obj,    0,             0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

/**
 * Create a button with a centred Montserrat-20 label and register a callback.
 * Size must be set by the caller after this returns.
 */
static lv_obj_t *make_btn(lv_obj_t *parent, const char *text, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl);

    return btn;
}

// ---------------------------------------------------------------------------
// Button callbacks
// ---------------------------------------------------------------------------

static void cb_mode_up    (lv_event_t *e) { lv_label_set_text(lbl_status, "MODE UP");    }
static void cb_mode_down  (lv_event_t *e) { lv_label_set_text(lbl_status, "MODE DOWN");  }
static void cb_color_up   (lv_event_t *e) { lv_label_set_text(lbl_status, "COLOR UP");   }
static void cb_color_down (lv_event_t *e) { lv_label_set_text(lbl_status, "COLOR DOWN"); }
static void cb_sync       (lv_event_t *e) { lv_label_set_text(lbl_status, "SYNC");       }
static void cb_prev       (lv_event_t *e) { lv_label_set_text(lbl_status, LV_SYMBOL_LEFT " PREV");  }
static void cb_set        (lv_event_t *e) { lv_label_set_text(lbl_status, "SET");         }
static void cb_next       (lv_event_t *e) { lv_label_set_text(lbl_status, "NEXT " LV_SYMBOL_RIGHT); }
static void cb_global_settings(lv_event_t *e) { ui_navigate_to(UI_SCREEN_GLOBAL_SETTINGS); }
static void cb_drum_settings  (lv_event_t *e) { ui_navigate_to(UI_SCREEN_DRUM_SETTINGS);   }

// ---------------------------------------------------------------------------
// Screen construction — wallpaper background
// ---------------------------------------------------------------------------

static void build_background(lv_obj_t *screen, const lv_img_dsc_t *bg_dsc)
{
    if (!bg_dsc) return;   // nothing to draw if preload failed

    lv_obj_t *bg = lv_img_create(screen);
    // Passing a pointer (not a "S:/..." string) makes LVGL treat the image as a
    // RAM-resident C-array descriptor — no FS seek, no decode overhead.
    lv_img_set_src(bg, bg_dsc);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_size(bg, 1024, 600);
    lv_img_set_zoom(bg, 256);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
}

// ---------------------------------------------------------------------------
// Screen construction — left panel
// ---------------------------------------------------------------------------

static void build_left_panel(lv_obj_t *screen)
{
    // ------------------------------------------------------------------
    // Root container — left 512px of the 1024×600 screen.
    //
    // All 5 items are direct children of this flex column.
    // SPACE_BETWEEN auto-distributes the 104px remainder as four 26px gaps,
    // so the panel fills top-to-bottom without any hard-coded margin values.
    // ------------------------------------------------------------------
    lv_obj_t *left = lv_obj_create(screen);
    lv_obj_set_pos(left, 0, 0);
    lv_obj_set_size(left, 512, 600);
    lv_obj_set_style_radius(left, 0, 0);
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(left, 16, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left,
        LV_FLEX_ALIGN_SPACE_BETWEEN,   // main axis: equal gaps between all 5 items
        LV_FLEX_ALIGN_CENTER,          // cross axis: centre each item horizontally
        LV_FLEX_ALIGN_START);

    // ------------------------------------------------------------------
    // Item 1 — Row: MODE UP  |  COLOR UP   (120px tall)
    //
    // Two 234px buttons; SPACE_BETWEEN distributes the leftover 12px
    // (480 − 234×2 = 12) as a single 12px gap between them.
    // ------------------------------------------------------------------
    lv_obj_t *row1 = make_container(left);
    lv_obj_set_size(row1, lv_pct(100), 120);
    lv_obj_set_layout(row1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row1,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    lv_obj_set_size(make_btn(row1, "MODE UP",  cb_mode_up),  234, 120);
    lv_obj_set_size(make_btn(row1, "COLOR UP", cb_color_up), 234, 120);

    // ------------------------------------------------------------------
    // Item 2 — Row: MODE DOWN  |  COLOR DOWN   (120px tall)
    // ------------------------------------------------------------------
    lv_obj_t *row2 = make_container(left);
    lv_obj_set_size(row2, lv_pct(100), 120);
    lv_obj_set_layout(row2, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row2,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    lv_obj_set_size(make_btn(row2, "MODE DOWN",  cb_mode_down),  234, 120);
    lv_obj_set_size(make_btn(row2, "COLOR DOWN", cb_color_down), 234, 120);

    // ------------------------------------------------------------------
    // Item 3 — SYNC COLOR & MODE button   (80px tall, full width)
    // ------------------------------------------------------------------
    lv_obj_set_size(make_btn(left, "SYNC COLOR & MODE", cb_sync), lv_pct(100), 80);

    // ------------------------------------------------------------------
    // Item 4 — Status display box   (64px tall)
    //
    // Bordered container with a centred label that shows the last action.
    // Only this label's dirty rect is invalidated on update — no full repaint.
    // ------------------------------------------------------------------
    lv_obj_t *status_box = lv_obj_create(left);
    lv_obj_set_size(status_box, lv_pct(100), 64);
    lv_obj_set_style_radius(status_box, 4, 0);
    lv_obj_set_style_pad_all(status_box, 4, 0);
    lv_obj_clear_flag(status_box, LV_OBJ_FLAG_SCROLLABLE);

    lbl_status = lv_label_create(status_box);
    lv_label_set_text(lbl_status, "---");
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_status);

    // ------------------------------------------------------------------
    // Item 5 — Navigation row: ←  |  SET  |  →   (80px tall)
    //
    // Button widths: prev=120, set=220, next=120 → total 460px.
    // SPACE_BETWEEN distributes the leftover 20px as two 10px gaps.
    // ------------------------------------------------------------------
    lv_obj_t *nav_row = make_container(left);
    lv_obj_set_size(nav_row, lv_pct(100), 80);
    lv_obj_set_layout(nav_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    lv_obj_set_size(make_btn(nav_row, LV_SYMBOL_LEFT, cb_prev), 120, 80);
    lv_obj_set_size(make_btn(nav_row, "SET",           cb_set),  220, 80);
    lv_obj_set_size(make_btn(nav_row, LV_SYMBOL_RIGHT, cb_next), 120, 80);
}

// ---------------------------------------------------------------------------
// Screen construction — right panel (placeholder, transparent)
// ---------------------------------------------------------------------------

static void build_right_panel(lv_obj_t *screen)
{
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_set_pos(right, 512, 0);
    lv_obj_set_size(right, 512, 600);
    lv_obj_set_style_radius(right, 0, 0);
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(right);
    lv_label_set_text(lbl, "Drum Monitor\n(WIP)");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);
}

// ---------------------------------------------------------------------------
// Screen construction — nav overlay buttons (bottom-right)
//
// Two 64×64 icon buttons anchored 12px from the bottom-right corner.
// They sit above all other content (added last) and navigate to settings.
// ---------------------------------------------------------------------------

static lv_obj_t *make_icon_btn(lv_obj_t *parent, const char *img_src, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 64, 64);
    lv_obj_set_style_pad_all(btn, 4, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *img = lv_img_create(btn);
    lv_img_set_src(img, img_src);
    lv_obj_center(img);

    return btn;
}

static void build_nav_overlay(lv_obj_t *screen)
{
    // Container anchored to bottom-right, holding the two icon buttons side by side.
    lv_obj_t *bar = make_container(screen);
    lv_obj_set_size(bar, 148, 64);   // 64 + 20 gap + 64 = 148
    lv_obj_align(bar, LV_ALIGN_BOTTOM_RIGHT, -12, -12);
    lv_obj_set_layout(bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    make_icon_btn(bar, "S:/settings_drum.png", cb_drum_settings);
    make_icon_btn(bar, "S:/settings.png",      cb_global_settings);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ui_main_init(const lv_img_dsc_t *bg)
{
    scr_main = lv_obj_create(NULL);
    lv_obj_set_style_pad_all(scr_main, 0, 0);

    build_background(scr_main, bg);
    build_left_panel(scr_main);
    build_right_panel(scr_main);
    build_nav_overlay(scr_main);
}

lv_obj_t *ui_main_get_screen(void)
{
    return scr_main;
}
