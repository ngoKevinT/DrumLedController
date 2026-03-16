/**
 * @file    ui_manager.cpp
 * @brief   Core Node UI – LVGL 8.3/9.x implementation.
 *
 * Screen: 320 x 170 (LilyGo T-Display-S3, landscape).
 *
 * Layout
 * ┌─────────────────┬─────────────────────────┐
 * │  Control Panel  │     Drum Monitor        │
 * │   (left 160px)  │     (right 160px)       │
 * │                 │  ┌──────┐  ┌──────┐    │
 * │  [Mode ▼][▲]   │  │SNARE │  │T.TOM │    │
 * │  [Colr ▼][▲]   │  └──────┘  └──────┘    │
 * │  [SELECT/SUBMIT]│  ┌──────┐  ┌──────┐    │
 * │  [SYNC ALL]     │  │F.TOM │  │ KICK │    │
 * │                 │  └──────┘  └──────┘    │
 * │                 │  ── node info list ───  │
 * └─────────────────┴─────────────────────────┘
 */

#include <string.h>
#include <stdio.h>

#include "ui_manager.h"

// ---------------------------------------------------------------------------
// Constants & helpers
// ---------------------------------------------------------------------------

static const char *const DRUM_NAMES[NUM_DRUM_NODES] = {
    "SNARE", "T.TOM", "F.TOM", "KICK"
};

static const char *const MODE_NAMES[NUM_LED_MODES] = {
    "Pure Reactive",
    "Heat Map",
    "90s Rave",
    "Ghost Note",
    "Spectrum",
    "Trailing Edge",
    "The Void"
};

/** Palette: default starting colours per drum node. */
static const uint32_t DEFAULT_COLORS[NUM_DRUM_NODES] = {
    0xFFFFFF,   // Snare  – white
    0x00AAFF,   // T.Tom  – cyan-blue
    0xFF6600,   // F.Tom  – orange
    0xFF0033    // Kick   – red
};

/** Visual colours for the node border highlight. */
#define COLOR_SELECTED    lv_color_hex(0xFFD700)   // gold
#define COLOR_DESELECTED  lv_color_hex(0x444444)   // dark grey

/** Voltage bar max (3.3 V expressed as millivolts). */
#define VOLTAGE_MAX_MV    3300

// ---------------------------------------------------------------------------
// Module-private state
// ---------------------------------------------------------------------------

/** Currently selected drum node index (-1 = none). */
static int8_t s_selected_node = -1;

/** Canonical configs – transmitted to the nodes. */
static drum_config_t s_drum_cfg[NUM_DRUM_NODES];

/** Staging area – mutated by Mode/Color buttons, flushed on SUBMIT. */
static drum_staging_t s_staging[NUM_DRUM_NODES];

/** Latest telemetry snapshot received from each node. */
static drum_telemetry_t s_telemetry[NUM_DRUM_NODES];

// ---------------------------------------------------------------------------
// LVGL object handles
// ---------------------------------------------------------------------------

/* Root containers */
static lv_obj_t *s_screen       = NULL;
static lv_obj_t *s_panel_ctrl   = NULL;   // left half
static lv_obj_t *s_panel_mon    = NULL;   // right half

/* Drum node widgets (right panel) */
static lv_obj_t *s_node_btn[NUM_DRUM_NODES];
static lv_obj_t *s_node_label[NUM_DRUM_NODES];
static lv_obj_t *s_node_vbar[NUM_DRUM_NODES];       // voltage progress bar
static lv_obj_t *s_node_thresh_line[NUM_DRUM_NODES]; // threshold marker (lv_line)
static lv_obj_t *s_node_volt_lbl[NUM_DRUM_NODES];   // voltage text label

/* Node info list (bottom of right panel) */
static lv_obj_t *s_info_list    = NULL;
static lv_obj_t *s_info_rows[NUM_DRUM_NODES];

/* Control panel widgets (left panel) */
static lv_obj_t *s_mode_label   = NULL;
static lv_obj_t *s_mode_up_btn  = NULL;
static lv_obj_t *s_mode_dn_btn  = NULL;
static lv_obj_t *s_color_label  = NULL;
static lv_obj_t *s_color_up_btn = NULL;
static lv_obj_t *s_color_dn_btn = NULL;
static lv_obj_t *s_submit_btn   = NULL;
static lv_obj_t *s_sync_btn     = NULL;

// ---------------------------------------------------------------------------
// Forward declarations of internal helpers
// ---------------------------------------------------------------------------

static void _init_state(void);
static void _build_layout(void);
static void _build_control_panel(void);
static void _build_drum_monitor(void);
static void _build_node_info_list(void);

static void _refresh_control_panel_labels(void);
static void _refresh_node_widget(drum_id_t id);
static void _refresh_info_row(drum_id_t id);
static void _set_selected_node(int8_t idx);

/* Event callbacks */
static void _cb_node_click(lv_event_t *e);
static void _cb_mode_up(lv_event_t *e);
static void _cb_mode_dn(lv_event_t *e);
static void _cb_color_up(lv_event_t *e);
static void _cb_color_dn(lv_event_t *e);
static void _cb_submit(lv_event_t *e);
static void _cb_sync_all(lv_event_t *e);

/* Async trampoline for telemetry updates (called from lv_async_call). */
typedef struct { drum_id_t id; drum_telemetry_t tel; } _tel_async_t;
static void _async_telemetry_update(void *param);

// ---------------------------------------------------------------------------
// Colour cycling table (10 distinct, evenly-spaced hues in HSV).
// Packed as 0x00RRGGBB for simplicity.
// ---------------------------------------------------------------------------
static const uint32_t COLOR_CYCLE[] = {
    0xFFFFFF,   // 0 – White
    0xFF0000,   // 1 – Red
    0xFF6600,   // 2 – Orange
    0xFFFF00,   // 3 – Yellow
    0x00FF00,   // 4 – Green
    0x00FFAA,   // 5 – Spring
    0x00AAFF,   // 6 – Sky Blue
    0x0000FF,   // 7 – Blue
    0xAA00FF,   // 8 – Violet
    0xFF00AA    // 9 – Hot Pink
};
#define NUM_COLORS ((int)(sizeof(COLOR_CYCLE) / sizeof(COLOR_CYCLE[0])))

/** Find the closest index in COLOR_CYCLE for a packed RGB value. */
static int _color_to_index(uint32_t rgb) {
    for (int i = 0; i < NUM_COLORS; i++) {
        if (COLOR_CYCLE[i] == rgb) return i;
    }
    return 0;
}

// ===========================================================================
// Public API implementation
// ===========================================================================

void ui_manager_init(void) {
    _init_state();
    _build_layout();
    lv_scr_load(s_screen);
    /* Select the first node by default so the UI is never in a blank state. */
    _set_selected_node(0);
}

void ui_manager_tick(void) {
    lv_timer_handler();
}

void ui_update_telemetry(drum_id_t drum_id, const drum_telemetry_t *tel) {
    if ((uint8_t)drum_id >= NUM_DRUM_NODES || tel == NULL) return;

    /* Allocate a small struct on the heap; freed inside the async callback. */
    _tel_async_t *payload = (_tel_async_t *)lv_mem_alloc(sizeof(_tel_async_t));
    if (payload == NULL) return;
    payload->id  = drum_id;
    payload->tel = *tel;
    lv_async_call(_async_telemetry_update, payload);
}

void ui_submit_staging(drum_id_t drum_id) {
    if ((uint8_t)drum_id >= NUM_DRUM_NODES) return;

    /* Commit staging → canonical config. */
    s_drum_cfg[drum_id].color_rgb = s_staging[drum_id].color_rgb;
    s_drum_cfg[drum_id].mode      = s_staging[drum_id].mode;

    /* Transmit – implemented by the application layer. */
    ui_transmit_config(drum_id, &s_drum_cfg[drum_id]);

    /* Refresh the info list row to show the committed values. */
    _refresh_info_row(drum_id);
}

void ui_sync_all(void) {
    for (int i = 0; i < NUM_DRUM_NODES; i++) {
        ui_submit_staging((drum_id_t)i);
    }
}

const drum_config_t *ui_get_drum_config(drum_id_t drum_id) {
    if ((uint8_t)drum_id >= NUM_DRUM_NODES) return NULL;
    return &s_drum_cfg[drum_id];
}

const drum_staging_t *ui_get_staging(drum_id_t drum_id) {
    if ((uint8_t)drum_id >= NUM_DRUM_NODES) return NULL;
    return &s_staging[drum_id];
}

/** Default (weak) transmit hook – override in your ESP-NOW module. */
void ui_transmit_config(drum_id_t drum_id, const drum_config_t *cfg) {
    (void)drum_id;
    (void)cfg;
    /* No-op: the application layer must override this function. */
}

// ===========================================================================
// Internal: state initialisation
// ===========================================================================

static void _init_state(void) {
    for (int i = 0; i < NUM_DRUM_NODES; i++) {
        s_drum_cfg[i].color_rgb    = DEFAULT_COLORS[i];
        s_drum_cfg[i].mode         = MODE_PURE_REACTIVE;
        s_drum_cfg[i].sensitivity  = 128;
        s_drum_cfg[i].retrigger_ms = 30;

        s_staging[i].color_rgb = DEFAULT_COLORS[i];
        s_staging[i].mode      = MODE_PURE_REACTIVE;

        s_telemetry[i].live_voltage_mv = 0;
        s_telemetry[i].threshold_mv    = 500;
        s_telemetry[i].is_connected    = false;
    }
}

// ===========================================================================
// Internal: layout builders
// ===========================================================================

static void _build_layout(void) {
    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x111111), 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);

    /* ---- Left panel: Control ---- */
    s_panel_ctrl = lv_obj_create(s_screen);
    lv_obj_set_size(s_panel_ctrl, PANEL_W, SCREEN_H);
    lv_obj_align(s_panel_ctrl, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(s_panel_ctrl, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(s_panel_ctrl, 0, 0);
    lv_obj_set_style_pad_all(s_panel_ctrl, 4, 0);
    lv_obj_set_style_radius(s_panel_ctrl, 0, 0);

    /* ---- Right panel: Monitor ---- */
    s_panel_mon = lv_obj_create(s_screen);
    lv_obj_set_size(s_panel_mon, MONITOR_W, SCREEN_H);
    lv_obj_align(s_panel_mon, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(s_panel_mon, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_border_width(s_panel_mon, 0, 0);
    lv_obj_set_style_pad_all(s_panel_mon, 4, 0);
    lv_obj_set_style_radius(s_panel_mon, 0, 0);

    _build_control_panel();
    _build_drum_monitor();
    _build_node_info_list();
}

// ---------------------------------------------------------------------------
// Control Panel (left)
// ---------------------------------------------------------------------------

/** Helper – create a compact labelled Up/Down row with a value label. */
static lv_obj_t *_make_updown_row(lv_obj_t *parent,
                                   const char *row_label,
                                   int y_offset,
                                   lv_obj_t **out_value_lbl,
                                   lv_obj_t **out_up,
                                   lv_obj_t **out_dn)
{
    /* Row container */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, PANEL_W - 8, 28);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, y_offset);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x222233), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x444466), 0);
    lv_obj_set_style_pad_all(row, 2, 0);
    lv_obj_set_style_radius(row, 4, 0);

    /* Row category label (e.g. "Mode") */
    lv_obj_t *cat = lv_label_create(row);
    lv_label_set_text(cat, row_label);
    lv_obj_set_style_text_color(cat, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(cat, &lv_font_montserrat_10, 0);
    lv_obj_align(cat, LV_ALIGN_LEFT_MID, 2, 0);

    /* Down button */
    lv_obj_t *btn_dn = lv_btn_create(row);
    lv_obj_set_size(btn_dn, 20, 20);
    lv_obj_align(btn_dn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_dn, lv_color_hex(0x334455), 0);
    lv_obj_set_style_radius(btn_dn, 3, 0);
    lv_obj_t *lbl_dn = lv_label_create(btn_dn);
    lv_label_set_text(lbl_dn, LV_SYMBOL_DOWN);
    lv_obj_center(lbl_dn);
    *out_dn = btn_dn;

    /* Up button */
    lv_obj_t *btn_up = lv_btn_create(row);
    lv_obj_set_size(btn_up, 20, 20);
    lv_obj_align(btn_up, LV_ALIGN_RIGHT_MID, -24, 0);
    lv_obj_set_style_bg_color(btn_up, lv_color_hex(0x334455), 0);
    lv_obj_set_style_radius(btn_up, 3, 0);
    lv_obj_t *lbl_up = lv_label_create(btn_up);
    lv_label_set_text(lbl_up, LV_SYMBOL_UP);
    lv_obj_center(lbl_up);
    *out_up = btn_up;

    /* Value label (between category and buttons) */
    lv_obj_t *val = lv_label_create(row);
    lv_label_set_text(val, "—");
    lv_obj_set_style_text_color(val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_10, 0);
    lv_obj_align(val, LV_ALIGN_RIGHT_MID, -50, 0);
    lv_label_set_long_mode(val, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(val, 56);
    *out_value_lbl = val;

    return row;
}

static void _build_control_panel(void) {
    /* Title */
    lv_obj_t *title = lv_label_create(s_panel_ctrl);
    lv_label_set_text(title, "CONTROL");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8888CC), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    /* Mode row */
    _make_updown_row(s_panel_ctrl, "Mode", 14,
                     &s_mode_label, &s_mode_up_btn, &s_mode_dn_btn);
    lv_obj_add_event_cb(s_mode_up_btn, _cb_mode_up, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_mode_dn_btn, _cb_mode_dn, LV_EVENT_CLICKED, NULL);

    /* Color row */
    _make_updown_row(s_panel_ctrl, "Color", 46,
                     &s_color_label, &s_color_up_btn, &s_color_dn_btn);
    lv_obj_add_event_cb(s_color_up_btn, _cb_color_up, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_color_dn_btn, _cb_color_dn, LV_EVENT_CLICKED, NULL);

    /* SELECT / SUBMIT button */
    s_submit_btn = lv_btn_create(s_panel_ctrl);
    lv_obj_set_size(s_submit_btn, PANEL_W - 8, 26);
    lv_obj_align(s_submit_btn, LV_ALIGN_TOP_LEFT, 0, 80);
    lv_obj_set_style_bg_color(s_submit_btn, lv_color_hex(0x005599), 0);
    lv_obj_set_style_radius(s_submit_btn, 4, 0);
    lv_obj_t *submit_lbl = lv_label_create(s_submit_btn);
    lv_label_set_text(submit_lbl, "SELECT / SUBMIT");
    lv_obj_set_style_text_font(submit_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(submit_lbl);
    lv_obj_add_event_cb(s_submit_btn, _cb_submit, LV_EVENT_CLICKED, NULL);

    /* SYNC ALL button */
    s_sync_btn = lv_btn_create(s_panel_ctrl);
    lv_obj_set_size(s_sync_btn, PANEL_W - 8, 26);
    lv_obj_align(s_sync_btn, LV_ALIGN_TOP_LEFT, 0, 110);
    lv_obj_set_style_bg_color(s_sync_btn, lv_color_hex(0x004422), 0);
    lv_obj_set_style_radius(s_sync_btn, 4, 0);
    lv_obj_t *sync_lbl = lv_label_create(s_sync_btn);
    lv_label_set_text(sync_lbl, "SYNC ALL");
    lv_obj_set_style_text_font(sync_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(sync_lbl);
    lv_obj_add_event_cb(s_sync_btn, _cb_sync_all, LV_EVENT_CLICKED, NULL);
}

// ---------------------------------------------------------------------------
// Drum Monitor (right)
// ---------------------------------------------------------------------------

/**
 * Kit layout (pixel coordinates relative to s_panel_mon):
 *
 *   ( 4, 6)  SNARE   |  (84, 6)  T.TOM
 *   ( 4,60)  F.TOM   |  (84,60)  KICK
 *
 * Each node widget is 72 x 50 px.
 */
#define NODE_W   72
#define NODE_H   50

static const lv_coord_t NODE_X[NUM_DRUM_NODES] = { 4, 84, 4, 84 };
static const lv_coord_t NODE_Y[NUM_DRUM_NODES] = { 6, 6, 60, 60 };

static void _build_drum_monitor(void) {
    /* Title */
    lv_obj_t *title = lv_label_create(s_panel_mon);
    lv_label_set_text(title, "DRUM MONITOR");
    lv_obj_set_style_text_color(title, lv_color_hex(0x88CCAA), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    for (int i = 0; i < NUM_DRUM_NODES; i++) {
        /* ---- Clickable node button ---- */
        lv_obj_t *btn = lv_obj_create(s_panel_mon);
        lv_obj_set_size(btn, NODE_W, NODE_H);
        lv_obj_set_pos(btn, NODE_X[i], NODE_Y[i]);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E1E3A), 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_color(btn, COLOR_DESELECTED, 0);
        lv_obj_set_style_radius(btn, 5, 0);
        lv_obj_set_style_pad_all(btn, 3, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, _cb_node_click, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
        s_node_btn[i] = btn;

        /* Drum name label */
        lv_obj_t *name_lbl = lv_label_create(btn);
        lv_label_set_text(name_lbl, DRUM_NAMES[i]);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(name_lbl, LV_ALIGN_TOP_MID, 0, 2);
        s_node_label[i] = name_lbl;

        /* Live voltage progress bar */
        lv_obj_t *vbar = lv_bar_create(btn);
        lv_obj_set_size(vbar, NODE_W - 10, 7);
        lv_obj_align(vbar, LV_ALIGN_BOTTOM_MID, 0, -14);
        lv_bar_set_range(vbar, 0, VOLTAGE_MAX_MV);
        lv_bar_set_value(vbar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(vbar, lv_color_hex(0x223344), 0);
        lv_obj_set_style_bg_color(
            lv_bar_get_indicator(vbar),
            lv_color_hex(0x00DDAA), 0);
        s_node_vbar[i] = vbar;

        /* Voltage millivolt label */
        lv_obj_t *volt_lbl = lv_label_create(btn);
        lv_label_set_text(volt_lbl, "0mv");
        lv_obj_set_style_text_font(volt_lbl, &lv_font_montserrat_8, 0);
        lv_obj_set_style_text_color(volt_lbl, lv_color_hex(0x88CCBB), 0);
        lv_obj_align(volt_lbl, LV_ALIGN_BOTTOM_MID, 0, -2);
        s_node_volt_lbl[i] = volt_lbl;

        /* Threshold marker – a short horizontal lv_line inside the bar area.
           We create the line object here; its points are updated per telemetry. */
        lv_obj_t *thresh_line = lv_line_create(btn);
        static lv_point_t pts[NUM_DRUM_NODES][2]; /* static storage for points */
        pts[i][0].x = 0;  pts[i][0].y = 0;
        pts[i][1].x = 6;  pts[i][1].y = 0;
        lv_line_set_points(thresh_line, pts[i], 2);
        lv_obj_set_style_line_color(thresh_line, lv_color_hex(0xFF4444), 0);
        lv_obj_set_style_line_width(thresh_line, 2, 0);
        /* Position is refreshed in _refresh_node_widget(). */
        lv_obj_align(thresh_line, LV_ALIGN_BOTTOM_MID, 0, -14);
        s_node_thresh_line[i] = thresh_line;
    }
}

// ---------------------------------------------------------------------------
// Node Info List (bottom strip of right panel)
// ---------------------------------------------------------------------------

static void _build_node_info_list(void) {
    /* Thin separator */
    lv_obj_t *sep = lv_obj_create(s_panel_mon);
    lv_obj_set_size(sep, MONITOR_W - 8, 1);
    lv_obj_set_pos(sep, 4, 116);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    s_info_list = lv_obj_create(s_panel_mon);
    lv_obj_set_size(s_info_list, MONITOR_W - 8, SCREEN_H - 120);
    lv_obj_set_pos(s_info_list, 4, 118);
    lv_obj_set_style_bg_color(s_info_list, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_border_width(s_info_list, 0, 0);
    lv_obj_set_style_pad_all(s_info_list, 1, 0);
    lv_obj_set_style_radius(s_info_list, 0, 0);
    lv_obj_clear_flag(s_info_list, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < NUM_DRUM_NODES; i++) {
        lv_obj_t *row = lv_label_create(s_info_list);
        lv_obj_set_style_text_font(row, &lv_font_montserrat_8, 0);
        lv_obj_set_style_text_color(row, lv_color_hex(0xBBBBBB), 0);
        lv_obj_set_pos(row, 0, i * 12);
        lv_label_set_long_mode(row, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(row, MONITOR_W - 10);
        s_info_rows[i] = row;
        _refresh_info_row((drum_id_t)i);
    }
}

// ===========================================================================
// Internal: refresh helpers
// ===========================================================================

static void _refresh_control_panel_labels(void) {
    if (s_selected_node < 0) {
        lv_label_set_text(s_mode_label,  "—");
        lv_label_set_text(s_color_label, "—");
        return;
    }

    drum_id_t id = (drum_id_t)s_selected_node;
    lv_label_set_text(s_mode_label, MODE_NAMES[s_staging[id].mode]);

    char buf[12];
    snprintf(buf, sizeof(buf), "#%06lX",
             (unsigned long)(s_staging[id].color_rgb & 0xFFFFFFUL));
    lv_label_set_text(s_color_label, buf);
}

static void _refresh_node_widget(drum_id_t id) {
    /* Voltage bar */
    lv_bar_set_value(s_node_vbar[id],
                     (int32_t)s_telemetry[id].live_voltage_mv,
                     LV_ANIM_OFF);

    /* Voltage text */
    char vbuf[10];
    snprintf(vbuf, sizeof(vbuf), "%umv", s_telemetry[id].live_voltage_mv);
    lv_label_set_text(s_node_volt_lbl[id], vbuf);

    /* Threshold marker: map threshold_mv to bar pixel width. */
    int bar_px = NODE_W - 10;   /* same as bar width */
    int thr_px = (int)((long)s_telemetry[id].threshold_mv * bar_px
                       / VOLTAGE_MAX_MV);
    /* Clamp */
    if (thr_px < 0)       thr_px = 0;
    if (thr_px > bar_px)  thr_px = bar_px;

    /* Re-position the threshold line at the appropriate x within the bar. */
    int bar_left_offset = -(bar_px / 2) + thr_px;
    lv_obj_align(s_node_thresh_line[id], LV_ALIGN_BOTTOM_MID,
                 bar_left_offset - (bar_px / 2) + 5, -14);

    /* Connectivity tint */
    if (!s_telemetry[id].is_connected) {
        lv_obj_set_style_bg_color(s_node_btn[id],
                                  lv_color_hex(0x2A1A1A), 0);
    } else {
        lv_obj_set_style_bg_color(s_node_btn[id],
                                  lv_color_hex(0x1E1E3A), 0);
    }
}

static void _refresh_info_row(drum_id_t id) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%-5s #%06lX  %s",
             DRUM_NAMES[id],
             (unsigned long)(s_drum_cfg[id].color_rgb & 0xFFFFFFUL),
             MODE_NAMES[s_drum_cfg[id].mode]);
    lv_label_set_text(s_info_rows[id], buf);
}

static void _set_selected_node(int8_t idx) {
    /* Clear previous highlight */
    if (s_selected_node >= 0) {
        lv_obj_set_style_border_color(s_node_btn[s_selected_node],
                                      COLOR_DESELECTED, 0);
        lv_obj_set_style_border_width(s_node_btn[s_selected_node], 2, 0);
    }

    s_selected_node = idx;

    /* Apply new highlight */
    if (idx >= 0) {
        lv_obj_set_style_border_color(s_node_btn[idx],
                                      COLOR_SELECTED, 0);
        lv_obj_set_style_border_width(s_node_btn[idx], 3, 0);
    }

    _refresh_control_panel_labels();
}

// ===========================================================================
// Event callbacks
// ===========================================================================

static void _cb_node_click(lv_event_t *e) {
    int8_t idx = (int8_t)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < NUM_DRUM_NODES) {
        printf("[UI] Node button clicked: %s (id=%d)\n", DRUM_NAMES[idx], idx);
    } else {
        printf("[UI] Node button clicked: invalid id=%d\n", idx);
    }
    _set_selected_node(idx);
}

static void _cb_mode_up(lv_event_t *e) {
    (void)e;
    if (s_selected_node < 0) {
        printf("[UI] Mode Up clicked: no node selected\n");
        return;
    }
    drum_id_t id = (drum_id_t)s_selected_node;
    int next = ((int)s_staging[id].mode + 1) % NUM_LED_MODES;
    s_staging[id].mode = (led_mode_t)next;
    printf("[UI] Mode Up clicked: %s -> %s\n", DRUM_NAMES[id], MODE_NAMES[s_staging[id].mode]);
    _refresh_control_panel_labels();
}

static void _cb_mode_dn(lv_event_t *e) {
    (void)e;
    if (s_selected_node < 0) {
        printf("[UI] Mode Down clicked: no node selected\n");
        return;
    }
    drum_id_t id = (drum_id_t)s_selected_node;
    int prev = ((int)s_staging[id].mode - 1 + NUM_LED_MODES) % NUM_LED_MODES;
    s_staging[id].mode = (led_mode_t)prev;
    printf("[UI] Mode Down clicked: %s -> %s\n", DRUM_NAMES[id], MODE_NAMES[s_staging[id].mode]);
    _refresh_control_panel_labels();
}

static void _cb_color_up(lv_event_t *e) {
    (void)e;
    if (s_selected_node < 0) {
        printf("[UI] Color Up clicked: no node selected\n");
        return;
    }
    drum_id_t id    = (drum_id_t)s_selected_node;
    int cur_idx     = _color_to_index(s_staging[id].color_rgb);
    int next_idx    = (cur_idx + 1) % NUM_COLORS;
    s_staging[id].color_rgb = COLOR_CYCLE[next_idx];
    printf("[UI] Color Up clicked: %s -> #%06lX\n",
           DRUM_NAMES[id],
           (unsigned long)(s_staging[id].color_rgb & 0xFFFFFFUL));
    _refresh_control_panel_labels();
}

static void _cb_color_dn(lv_event_t *e) {
    (void)e;
    if (s_selected_node < 0) {
        printf("[UI] Color Down clicked: no node selected\n");
        return;
    }
    drum_id_t id = (drum_id_t)s_selected_node;
    int cur_idx  = _color_to_index(s_staging[id].color_rgb);
    int prev_idx = (cur_idx - 1 + NUM_COLORS) % NUM_COLORS;
    s_staging[id].color_rgb = COLOR_CYCLE[prev_idx];
    printf("[UI] Color Down clicked: %s -> #%06lX\n",
           DRUM_NAMES[id],
           (unsigned long)(s_staging[id].color_rgb & 0xFFFFFFUL));
    _refresh_control_panel_labels();
}

static void _cb_submit(lv_event_t *e) {
    (void)e;
    if (s_selected_node < 0) {
        printf("[UI] SELECT/SUBMIT clicked: no node selected\n");
        return;
    }
    printf("[UI] SELECT/SUBMIT clicked: %s (id=%d)\n",
           DRUM_NAMES[s_selected_node], s_selected_node);
    ui_submit_staging((drum_id_t)s_selected_node);
}

static void _cb_sync_all(lv_event_t *e) {
    (void)e;
    printf("[UI] SYNC ALL clicked\n");
    ui_sync_all();
}

// ===========================================================================
// Async telemetry trampoline
// ===========================================================================

static void _async_telemetry_update(void *param) {
    _tel_async_t *p = (_tel_async_t *)param;
    if (p == NULL) return;

    drum_id_t id = p->id;
    if ((uint8_t)id < NUM_DRUM_NODES) {
        s_telemetry[id] = p->tel;
        _refresh_node_widget(id);
        /* Also refresh info row so connectivity text stays current. */
        _refresh_info_row(id);
    }

    lv_mem_free(p);
}
