#pragma once

#include <lvgl.h>

#include "../ui_manager.h"

typedef struct {
    lv_event_cb_t on_node_click;
    lv_event_cb_t on_mode_up;
    lv_event_cb_t on_mode_dn;
    lv_event_cb_t on_color_up;
    lv_event_cb_t on_color_dn;
    lv_event_cb_t on_submit;
    lv_event_cb_t on_sync_all;
    lv_event_cb_t on_goto_settings;
} main_screen_ui_callbacks_t;

typedef struct {
    lv_obj_t *mode_label;
    lv_obj_t *mode_up_btn;
    lv_obj_t *mode_dn_btn;
    lv_obj_t *color_label;
    lv_obj_t *color_up_btn;
    lv_obj_t *color_dn_btn;
    lv_obj_t *submit_btn;
    lv_obj_t *sync_btn;
    lv_obj_t *to_settings_btn;

    lv_obj_t *node_btn[NUM_DRUM_NODES];
    lv_obj_t *node_label[NUM_DRUM_NODES];
    lv_obj_t *node_vbar[NUM_DRUM_NODES];
    lv_obj_t *node_thresh_line[NUM_DRUM_NODES];
    lv_obj_t *node_volt_lbl[NUM_DRUM_NODES];

    lv_obj_t *info_list;
    lv_obj_t *info_rows[NUM_DRUM_NODES];
} main_screen_ui_refs_t;

void main_screen_ui_init(main_screen_ui_refs_t *refs);

void main_screen_ui_build(main_screen_ui_refs_t *refs,
                          lv_obj_t *panel_ctrl,
                          lv_obj_t *panel_mon,
                          int32_t panel_w,
                          int32_t monitor_w,
                          int32_t screen_h,
                          const char *const *drum_names,
                          const main_screen_ui_callbacks_t *callbacks);

void main_screen_ui_refresh_control_labels(main_screen_ui_refs_t *refs,
                                           int8_t selected_node,
                                           const drum_staging_t *staging,
                                           const char *const *mode_names);

void main_screen_ui_refresh_node_widget(main_screen_ui_refs_t *refs,
                                        drum_id_t id,
                                        const drum_telemetry_t *tel,
                                        int32_t voltage_max_mv,
                                        lv_color_t online_color,
                                        lv_color_t offline_color);

void main_screen_ui_refresh_info_row(main_screen_ui_refs_t *refs,
                                     drum_id_t id,
                                     const drum_config_t *cfg,
                                     const char *const *drum_names,
                                     const char *const *mode_names);

void main_screen_ui_set_selected(main_screen_ui_refs_t *refs,
                                 int8_t prev_selected,
                                 int8_t new_selected,
                                 lv_color_t selected_color,
                                 lv_color_t deselected_color);
