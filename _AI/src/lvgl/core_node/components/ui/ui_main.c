/*****************************************************************************
 * File        : ui_main.c
 * Component   : ui
 * Description : Main dashboard screen for the ADLS Core Node (1024×600).
 *
 *   LEFT panel  (512px) — Control buttons, status, navigation
 *   RIGHT panel (512px) — Debug label + 6 drum monitor cards
 *
 *   Left panel layout  (inner content area: 480×568 after 16px padding)
 *   ┌────────────────────────────────────────────┐  ↑
 *   │  [ MODE UP  ]        [ COLOR UP  ]         │  120px
 *   │  [ MODE DOWN]        [ COLOR DOWN]         │  120px
 *   │  [      SYNC COLOR & MODE        ]         │   80px
 *   │  ┌─────────── status label ──────────────┐ │   64px
 *   │  [   ←   ]      [    SET    ]      [  →  ] │   80px
 *   └────────────────────────────────────────────┘  ↓
 *   Gaps (SPACE_BETWEEN): (568−464)/4 = 26px each
 *
 *   Right panel layout (inner 496×584 after 8px padding)
 *   ┌────────────────────────────────────────────┐  ↑
 *   │  [debug label                            ] │  24px
 *   ├─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤  8px gap
 *   │  ┌─────────────────┐ ┌─────────────────┐  │  ↑
 *   │  │ [swatch] PULSE  │ │ [swatch] SOLID  │  │  │ row 0 (148px)
 *   │  └─────────────────┘ └─────────────────┘  │  ↓
 *   │  ┌─────────────────┐ ┌─────────────────┐  │  row 1 (148px)
 *   │  ┌─────────────────┐ ┌─────────────────┐  │  row 2 (148px)
 *   └────────────────────────────────────────────┘
 *   Grid h=476px; 3 rows×148=444; SPACE_BETWEEN: 2×16px gaps
 *   Grid w=496px; 2 cols×244=488; SPACE_BETWEEN: 1×8px gap
 *
 *   Each drum card (244×148px, 6px padding → inner 232×136px):
 *     Left:  80×80 color swatch — bg = drum LED color (80% opa for legibility)
 *            Inside swatch: name / "T:NNN" / "R:NNN" (Montserrat-12, white)
 *     Right: mode name (Montserrat-20) stacked above color hex (Montserrat-12)
 *
 *   Selection: 4px white LEFT border + slightly higher bg opacity.
 *   Flash:     Card bg jumps to drum color on trigger; one-shot 300ms timer
 *              restores the dark bg (no FS I/O, all PSRAM/SRAM ops).
 *
 *   Bottom-right overlay: two 64×64 icon buttons, 12px from corner.
 *     Drum Settings    → "S:/settings_drum.png"  (uses g_selected_drum)
 *     Global Settings  → "S:/settings.png"
 *****************************************************************************/

#include "lvgl.h"
#include "ui_main.h"
#include "ui_drum_settings.h"
#include "ui.h"

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------

static lv_obj_t *scr_main   = NULL;
static lv_obj_t *lbl_status = NULL;   // Last-pressed action text

// Right panel — debug line
static lv_obj_t *lbl_debug  = NULL;

// Right panel — one entry per drum card
typedef struct {
    lv_obj_t   *card;        // root card object (bg flash + selection border)
    lv_obj_t   *swatch;      // colored square (bg color update)
    lv_obj_t   *lbl_name;    // drum name inside swatch
    lv_obj_t   *lbl_thr;     // "T: NNN" inside swatch
    lv_obj_t   *lbl_read;    // "R: NNN" inside swatch
    lv_obj_t   *lbl_mode;    // mode name only: "PULSE"
    lv_obj_t   *lbl_color;   // color hex: "#FF2200"
    lv_timer_t *flash_timer; // non-NULL while a flash is in progress
} drum_card_t;

static drum_card_t s_cards[UI_MAX_DRUMS];

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
    // Clear both SCROLLABLE and CLICKABLE so layout containers never
    // intercept touch events that should reach interactive children.
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
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
static void cb_drum_settings(lv_event_t *e)
{
    // Refresh the drum settings screen with the currently selected drum's data
    // before navigating so the title and info labels are always in sync.
    ui_drum_settings_refresh();
    ui_navigate_to(UI_SCREEN_DRUM_SETTINGS);
}

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
// Drum card helpers
// ---------------------------------------------------------------------------

/** Unpack 0x00RRGGBB into an lv_color_t. */
static lv_color_t unpack_color(uint32_t rgb)
{
    return lv_color_make((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

/**
 * Apply or remove the selection highlight on card[idx].
 *
 * Selected   : 4px white LEFT border + slightly higher bg opacity.
 *              Left-edge accent is instantly recognisable without boxing in
 *              the content or fighting the color swatch for attention.
 * Unselected : 1px light-gray FULL border at low opacity.
 * Disconnected unselected: same but bg opacity is lower (20% vs 50%).
 */
static void set_card_selected(int idx, bool selected)
{
    lv_obj_t *card = s_cards[idx].card;
    if (selected) {
        lv_obj_set_style_border_width(card, 4, 0);
        lv_obj_set_style_border_color(card, lv_color_white(), 0);
        lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_side(card, LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_70, 0);
    } else {
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, lv_color_make(180, 180, 180), 0);
        lv_obj_set_style_border_opa(card, LV_OPA_30, 0);
        lv_obj_set_style_border_side(card, LV_BORDER_SIDE_FULL, 0);
        lv_obj_set_style_bg_opa(card, g_drums[idx].connected ? LV_OPA_50 : LV_OPA_20, 0);
    }
}

/** Move selection to idx; deselects the previous card and updates g_selected_drum. */
static void select_drum(int idx)
{
    set_card_selected(g_selected_drum, false);
    g_selected_drum = idx;
    set_card_selected(g_selected_drum, true);
}

/** Card click callback — idx is stored as user_data. */
static void cb_card_click(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    select_drum(idx);
}

/**
 * One-shot timer callback: restores the card background after a trigger flash.
 * user_data points to the drum_card_t entry so we can derive the index and
 * apply the correct opacity for selected vs unselected state.
 */
static void flash_expire_cb(lv_timer_t *t)
{
    drum_card_t *dc  = (drum_card_t *)t->user_data;
    int          idx = (int)(dc - s_cards);   // pointer arithmetic → index

    dc->flash_timer = NULL;
    lv_obj_set_style_bg_color(dc->card, lv_color_make(20, 20, 20), 0);
    lv_obj_set_style_bg_opa(dc->card,
        (idx == g_selected_drum) ? LV_OPA_70 : (g_drums[idx].connected ? LV_OPA_50 : LV_OPA_20),
        0);
}

/**
 * Build one drum card and store references in s_cards[idx].
 *
 * Card layout (244×148px, inner 232×136px after 6px padding):
 *
 *   ┌──────────────────────────────────────────────────┐
 *   │  ┌────────┐   PULSE            (Montserrat-20)   │
 *   │  │ Kick   │                                      │
 *   │  │ T: 150 │   #FF2200          (Montserrat-12)   │
 *   │  │ R:   0 │                                      │
 *   │  └────────┘                                      │
 *   └──────────────────────────────────────────────────┘
 *    ←80px swatch→←8px→←────── 144px info ───────────→
 */
static void build_drum_card(lv_obj_t *parent, int idx)
{
    ui_drum_t   *drum = &g_drums[idx];
    drum_card_t *dc   = &s_cards[idx];

    // ------------------------------------------------------------------
    // Card container — 244×148px, 6px padding → inner 232×136px
    // ------------------------------------------------------------------
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 244, 148);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_bg_color(card, lv_color_make(20, 20, 20), 0);
    // Disconnected slots are dimmed to 20% so active drums stand out.
    lv_obj_set_style_bg_opa(card, drum->connected ? LV_OPA_50 : LV_OPA_20, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_make(180, 180, 180), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_30, 0);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    // LV_EVENT_PRESSED fires on finger-down for instant visual feedback,
    // vs LV_EVENT_CLICKED which only fires after finger-up.
    lv_obj_add_event_cb(card, cb_card_click, LV_EVENT_PRESSED, (void *)(intptr_t)idx);
    dc->card        = card;
    dc->flash_timer = NULL;

    // ------------------------------------------------------------------
    // Color swatch — 80×80px, left-aligned, vertically centred.
    //
    // 80% opacity lets the dark card bleed through so white text stays
    // legible on any drum LED color without requiring text shadows.
    // ------------------------------------------------------------------
    lv_color_t swatch_col = drum->connected ? unpack_color(drum->color)
                                            : lv_color_make(80, 80, 80);

    lv_obj_t *swatch = lv_obj_create(card);
    lv_obj_set_size(swatch, 80, 80);
    lv_obj_align(swatch, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(swatch, 6, 0);
    lv_obj_set_style_bg_color(swatch, swatch_col, 0);
    lv_obj_set_style_bg_opa(swatch, LV_OPA_80, 0);
    lv_obj_set_style_border_width(swatch, 0, 0);
    lv_obj_set_style_pad_all(swatch, 4, 0);
    // lv_obj_create() sets CLICKABLE by default in LVGL v8.  Clear it so
    // touches on the swatch fall through to the card, which owns the handler.
    lv_obj_clear_flag(swatch, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(swatch, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(swatch, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(swatch,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    dc->swatch = swatch;

    // Swatch labels: name / threshold / reading
    dc->lbl_name = lv_label_create(swatch);
    lv_label_set_text(dc->lbl_name, drum->name);
    lv_obj_set_style_text_font(dc->lbl_name, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(dc->lbl_name, lv_color_white(), 0);
    lv_label_set_long_mode(dc->lbl_name, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(dc->lbl_name, lv_pct(100));

    dc->lbl_thr = lv_label_create(swatch);
    lv_label_set_text_fmt(dc->lbl_thr, "T:%3u", drum->threshold);
    lv_obj_set_style_text_font(dc->lbl_thr, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(dc->lbl_thr, lv_color_white(), 0);

    dc->lbl_read = lv_label_create(swatch);
    lv_label_set_text_fmt(dc->lbl_read, "R:%3u", drum->reading);
    lv_obj_set_style_text_font(dc->lbl_read, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(dc->lbl_read, lv_color_white(), 0);

    // ------------------------------------------------------------------
    // Right-side info panel — mode name (large) above color hex (small).
    //
    // Transparent flex container centred in the remaining 144px width.
    // x offset = 80 (swatch) + 8 (gap) = 88 from inner-left.
    // Width = 232 (inner) − 80 (swatch) − 8 (gap) = 144px.
    // ------------------------------------------------------------------
    lv_obj_t *info = make_container(card);
    lv_obj_set_size(info, 144, 136);
    lv_obj_align(info, LV_ALIGN_LEFT_MID, 88, 0);
    lv_obj_set_layout(info, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info,
        LV_FLEX_ALIGN_CENTER,   // centre items vertically
        LV_FLEX_ALIGN_CENTER,   // centre items horizontally
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(info, 6, 0);

    dc->lbl_mode = lv_label_create(info);
    lv_obj_set_style_text_font(dc->lbl_mode, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(dc->lbl_mode, lv_color_white(), 0);

    dc->lbl_color = lv_label_create(info);
    lv_obj_set_style_text_font(dc->lbl_color, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(dc->lbl_color, lv_color_make(200, 200, 200), 0);

    if (drum->connected) {
        lv_label_set_text(dc->lbl_mode, ui_mode_names[drum->mode_idx]);
        lv_label_set_text_fmt(dc->lbl_color, "#%06X", (unsigned int)drum->color);
    } else {
        lv_label_set_text(dc->lbl_mode, "--");
        lv_label_set_text(dc->lbl_color, "Disconnected");
    }
}

// ---------------------------------------------------------------------------
// Screen construction — right panel
// ---------------------------------------------------------------------------

static void build_right_panel(lv_obj_t *screen)
{
    // ------------------------------------------------------------------
    // Root container — right 512px, transparent, 8px padding.
    // Inner content area: 496×584.
    // ------------------------------------------------------------------
    lv_obj_t *panel = lv_obj_create(screen);
    lv_obj_set_pos(panel, 512, 0);
    lv_obj_set_size(panel, 512, 600);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    // ------------------------------------------------------------------
    // Debug label — top of panel, 24px tall.
    // ------------------------------------------------------------------
    lbl_debug = lv_label_create(panel);
    lv_label_set_text(lbl_debug, "Debug: --");
    lv_obj_set_style_text_font(lbl_debug, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_debug, lv_color_make(180, 180, 180), 0);
    lv_obj_set_size(lbl_debug, lv_pct(100), 24);
    lv_obj_align(lbl_debug, LV_ALIGN_TOP_LEFT, 0, 0);

    // ------------------------------------------------------------------
    // 2×3 drum grid — fills the space between the debug label and the
    // nav icon buttons (which sit at the bottom-right of the screen).
    //
    // Pixel budget (inner panel h = 584):
    //   debug top  = 0,  debug bottom = 24
    //   gap        = 8px  →  grid starts at y = 32
    //   nav bar top on screen = 600 − 12 − 64 = 524
    //   nav bar top in content = 524 − 8 (panel pad) = 516
    //   gap above nav = 8px  →  grid ends at y = 508
    //   grid h = 508 − 32 = 476px
    //
    //   3 rows × 148px = 444px; SPACE_BETWEEN: 2 gaps of 16px each  ✓
    //   2 cols × 244px = 488px; SPACE_BETWEEN: 1 gap of  8px        ✓
    // ------------------------------------------------------------------
    lv_obj_t *grid = make_container(panel);
    lv_obj_set_size(grid, lv_pct(100), 476);
    lv_obj_align(grid, LV_ALIGN_TOP_LEFT, 0, 32);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(grid,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_START);

    for (int row = 0; row < 3; row++) {
        lv_obj_t *drum_row = make_container(grid);
        lv_obj_set_size(drum_row, lv_pct(100), 148);
        lv_obj_set_layout(drum_row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(drum_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(drum_row,
            LV_FLEX_ALIGN_SPACE_BETWEEN,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_START);

        build_drum_card(drum_row, row * 2 + 0);
        build_drum_card(drum_row, row * 2 + 1);
    }

    // Apply the default selection highlight (drum 0).
    set_card_selected(g_selected_drum, true);
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
// Public API — drum card updates
// (must be called while holding the LVGL port mutex)
// ---------------------------------------------------------------------------

/**
 * Refresh the visual state of card[idx] from g_drums[idx].
 * Call this after updating g_drums[idx] with fresh telemetry.
 */
void ui_main_drum_update(int idx)
{
    if (idx < 0 || idx >= UI_MAX_DRUMS) return;

    ui_drum_t   *drum = &g_drums[idx];
    drum_card_t *dc   = &s_cards[idx];

    // Swatch color
    lv_color_t col = drum->connected ? unpack_color(drum->color)
                                     : lv_color_make(80, 80, 80);
    lv_obj_set_style_bg_color(dc->swatch, col, 0);

    // Swatch labels
    lv_label_set_text(dc->lbl_name, drum->name);
    lv_label_set_text_fmt(dc->lbl_thr,  "T:%3u", drum->threshold);
    lv_label_set_text_fmt(dc->lbl_read, "R:%3u", drum->reading);

    // Right-side info labels
    if (drum->connected) {
        lv_label_set_text(dc->lbl_mode, ui_mode_names[drum->mode_idx]);
        lv_label_set_text_fmt(dc->lbl_color, "#%06X", (unsigned int)drum->color);
    } else {
        lv_label_set_text(dc->lbl_mode, "--");
        lv_label_set_text(dc->lbl_color, "Disconnected");
    }

    // Card bg opacity (in case connected state changed)
    if (idx != g_selected_drum) {
        lv_obj_set_style_bg_opa(dc->card, drum->connected ? LV_OPA_50 : LV_OPA_20, 0);
    }
}

/**
 * Flash card[idx]'s background to the drum's LED color for 300ms, then fade
 * back to the dark bg.  Safe to call on rapid successive hits — any in-progress
 * flash timer is cancelled and restarted so the card always shows the latest hit.
 */
void ui_main_drum_flash(int idx)
{
    if (idx < 0 || idx >= UI_MAX_DRUMS) return;

    drum_card_t *dc = &s_cards[idx];

    // Cancel any in-progress flash timer so we don't double-restore.
    if (dc->flash_timer) {
        lv_timer_del(dc->flash_timer);
        dc->flash_timer = NULL;
    }

    // Set card bg to the drum's current LED color at high opacity.
    lv_obj_set_style_bg_color(dc->card, unpack_color(g_drums[idx].color), 0);
    lv_obj_set_style_bg_opa(dc->card, LV_OPA_90, 0);

    // One-shot timer: restore bg after 300ms.
    dc->flash_timer = lv_timer_create(flash_expire_cb, 300, dc);
    lv_timer_set_repeat_count(dc->flash_timer, 1);
}

// ---------------------------------------------------------------------------
// Public API — screen construction
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
