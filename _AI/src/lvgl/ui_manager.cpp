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
static void _cb_goto_settings(lv_event_t *e);
static void _cb_goto_main(lv_event_t *e);
static void _cb_settings_node_prev(lv_event_t *e);
static void _cb_settings_node_next(lv_event_t *e);

typedef struct {
    drum_id_t id;
    drum_telemetry_t tel;
} _tel_async_t;

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

static int _color_to_index(uint32_t rgb) {
    for (int i = 0; i < NUM_COLORS; i++) {
        if (COLOR_CYCLE[i] == rgb) return i;
    }
    return 0;
}

void ui_manager_init(void) {
    _init_state();
    main_screen_ui_init(&s_main_ui);

    _build_loading_screen();
    _build_main_screen();
    _build_settings_screen();

    screen_router_init(&s_screen_router, s_loading_screen, s_main_screen, s_settings_screen);

    ui_show_screen(UI_SCREEN_LOADING);
    lv_timer_t *boot_timer = lv_timer_create(_screen_load_done_cb, 900, NULL);
    if (boot_timer != NULL) lv_timer_set_repeat_count(boot_timer, 1);
}

void ui_manager_tick(void) {
    lv_timer_handler();
}

void ui_update_telemetry(drum_id_t drum_id, const drum_telemetry_t *tel) {
    if ((uint8_t)drum_id >= NUM_DRUM_NODES || tel == NULL) return;

    _tel_async_t *payload = (_tel_async_t *)lv_mem_alloc(sizeof(_tel_async_t));
    if (payload == NULL) return;

    payload->id = drum_id;
    payload->tel = *tel;
    lv_async_call(_async_telemetry_update, payload);
}

void ui_submit_staging(drum_id_t drum_id) {
    if ((uint8_t)drum_id >= NUM_DRUM_NODES) return;

    s_drum_cfg[drum_id].color_rgb = s_staging[drum_id].color_rgb;
    s_drum_cfg[drum_id].mode = s_staging[drum_id].mode;

    ui_transmit_config(drum_id, &s_drum_cfg[drum_id]);

    _refresh_info_row(drum_id);
    _refresh_settings_screen();
}

void ui_sync_all(void) {
    for (int i = 0; i < NUM_DRUM_NODES; i++) {
        ui_submit_staging((drum_id_t)i);
    }
}

void ui_show_screen(ui_screen_t screen) {
    if (screen == UI_SCREEN_SETTINGS) {
        _refresh_settings_screen();
    }

    (void)screen_router_show(&s_screen_router, screen);
}

ui_screen_t ui_get_active_screen(void) {
    return screen_router_get_active(&s_screen_router);
}

const drum_config_t *ui_get_drum_config(drum_id_t drum_id) {
    if ((uint8_t)drum_id >= NUM_DRUM_NODES) return NULL;
    return &s_drum_cfg[drum_id];
}

const drum_staging_t *ui_get_staging(drum_id_t drum_id) {
    if ((uint8_t)drum_id >= NUM_DRUM_NODES) return NULL;
    return &s_staging[drum_id];
}

void ui_transmit_config(drum_id_t drum_id, const drum_config_t *cfg) {
    (void)drum_id;
    (void)cfg;
}

static void _screen_load_done_cb(lv_timer_t *t) {
    (void)t;
    ui_show_screen(UI_SCREEN_MAIN);
    _set_selected_node(0);
}

static void _init_state(void) {
    for (int i = 0; i < NUM_DRUM_NODES; i++) {
        s_drum_cfg[i].color_rgb = DEFAULT_COLORS[i];
        s_drum_cfg[i].mode = MODE_PURE_REACTIVE;
        s_drum_cfg[i].sensitivity = 128;
        s_drum_cfg[i].retrigger_ms = 30;

        s_staging[i].color_rgb = DEFAULT_COLORS[i];
        s_staging[i].mode = MODE_PURE_REACTIVE;

        s_telemetry[i].live_voltage_mv = 0;
        s_telemetry[i].threshold_mv = 500;
        s_telemetry[i].is_connected = false;
    }
}

static void _build_loading_screen(void) {
    s_loading_screen = loading_screen_create(SCREEN_W, SCREEN_H);
}

static void _build_main_screen(void) {
    main_screen_refs_t refs = main_screen_create(SCREEN_W, SCREEN_H, PANEL_W, MONITOR_W);
    s_main_screen = refs.screen;
    s_panel_ctrl = refs.panel_ctrl;
    s_panel_mon = refs.panel_mon;

    main_screen_ui_callbacks_t callbacks;
    callbacks.on_node_click = _cb_node_click;
    callbacks.on_mode_up = _cb_mode_up;
    callbacks.on_mode_dn = _cb_mode_dn;
    callbacks.on_color_up = _cb_color_up;
    callbacks.on_color_dn = _cb_color_dn;
    callbacks.on_submit = _cb_submit;
    callbacks.on_sync_all = _cb_sync_all;
    callbacks.on_goto_settings = _cb_goto_settings;

    main_screen_ui_build(&s_main_ui,
                         s_panel_ctrl,
                         s_panel_mon,
                         PANEL_W,
                         MONITOR_W,
                         SCREEN_H,
                         DRUM_NAMES,
                         &callbacks);

    for (int i = 0; i < NUM_DRUM_NODES; i++) {
        _refresh_info_row((drum_id_t)i);
    }
}

static void _build_settings_screen(void) {
    settings_screen_callbacks_t callbacks;
    callbacks.on_back = _cb_goto_main;
    callbacks.on_prev_node = _cb_settings_node_prev;
    callbacks.on_next_node = _cb_settings_node_next;

    settings_screen_widget_refs_t refs;
    s_settings_screen = settings_screen_create(SCREEN_W, SCREEN_H, &callbacks, &refs);
    settings_presenter_init(&s_settings_presenter, &refs);
    _refresh_settings_screen();
}

static void _refresh_settings_screen(void) {
    settings_presenter_refresh(&s_settings_presenter,
                               s_telemetry,
                               s_drum_cfg,
                               DRUM_NAMES,
                               MODE_NAMES,
                               NUM_DRUM_NODES);
}

static void _refresh_control_panel_labels(void) {
    main_screen_ui_refresh_control_labels(&s_main_ui,
                                          s_selected_node,
                                          s_staging,
                                          MODE_NAMES);
}

static void _refresh_node_widget(drum_id_t id) {
    main_screen_ui_refresh_node_widget(&s_main_ui,
                                       id,
                                       &s_telemetry[id],
                                       VOLTAGE_MAX_MV,
                                       lv_color_hex(0x1E1E3A),
                                       lv_color_hex(0x2A1A1A));
}

static void _refresh_info_row(drum_id_t id) {
    main_screen_ui_refresh_info_row(&s_main_ui,
                                    id,
                                    s_drum_cfg,
                                    DRUM_NAMES,
                                    MODE_NAMES);
}

static void _set_selected_node(int8_t idx) {
    int8_t prev = s_selected_node;
    s_selected_node = idx;

    main_screen_ui_set_selected(&s_main_ui,
                                prev,
                                idx,
                                COLOR_SELECTED,
                                COLOR_DESELECTED);

    _refresh_control_panel_labels();
}

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

    drum_id_t id = (drum_id_t)s_selected_node;
    int cur_idx = _color_to_index(s_staging[id].color_rgb);
    int next_idx = (cur_idx + 1) % NUM_COLORS;
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
    int cur_idx = _color_to_index(s_staging[id].color_rgb);
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

static void _cb_goto_settings(lv_event_t *e) {
    (void)e;
    printf("[UI] SETTINGS clicked\n");
    ui_show_screen(UI_SCREEN_SETTINGS);
}

static void _cb_goto_main(lv_event_t *e) {
    (void)e;
    printf("[UI] Back to Main clicked\n");
    ui_show_screen(UI_SCREEN_MAIN);
}

static void _cb_settings_node_prev(lv_event_t *e) {
    (void)e;
    settings_presenter_prev_node(&s_settings_presenter, NUM_DRUM_NODES);
    _refresh_settings_screen();
}

static void _cb_settings_node_next(lv_event_t *e) {
    (void)e;
    settings_presenter_next_node(&s_settings_presenter, NUM_DRUM_NODES);
    _refresh_settings_screen();
}

static void _async_telemetry_update(void *param) {
    _tel_async_t *p = (_tel_async_t *)param;
    if (p == NULL) return;

    drum_id_t id = p->id;
    if ((uint8_t)id < NUM_DRUM_NODES) {
        s_telemetry[id] = p->tel;
        _refresh_node_widget(id);
        _refresh_info_row(id);
        _refresh_settings_screen();
    }

    lv_mem_free(p);
}
