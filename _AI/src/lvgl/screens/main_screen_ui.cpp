#include "main_screen_ui.h"

#include <stdio.h>
#include <string.h>

#define NODE_W 72
#define NODE_H 50

static const lv_coord_t NODE_X[NUM_DRUM_NODES] = {4, 84, 4, 84};
static const lv_coord_t NODE_Y[NUM_DRUM_NODES] = {6, 6, 60, 60};

static lv_point_t s_thresh_pts[NUM_DRUM_NODES][2];

static lv_obj_t *make_updown_row(lv_obj_t *parent,
                                 int32_t panel_w,
                                 const char *row_label,
                                 int y_offset,
                                 lv_obj_t **out_value_lbl,
                                 lv_obj_t **out_up,
                                 lv_obj_t **out_dn) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, panel_w - 8, 28);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, y_offset);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x222233), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x444466), 0);
    lv_obj_set_style_pad_all(row, 2, 0);
    lv_obj_set_style_radius(row, 4, 0);

    lv_obj_t *cat = lv_label_create(row);
    lv_label_set_text(cat, row_label);
    lv_obj_set_style_text_color(cat, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(cat, &lv_font_montserrat_10, 0);
    lv_obj_align(cat, LV_ALIGN_LEFT_MID, 2, 0);

    lv_obj_t *btn_dn = lv_btn_create(row);
    lv_obj_set_size(btn_dn, 20, 20);
    lv_obj_align(btn_dn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_dn, lv_color_hex(0x334455), 0);
    lv_obj_set_style_radius(btn_dn, 3, 0);
    lv_obj_t *lbl_dn = lv_label_create(btn_dn);
    lv_label_set_text(lbl_dn, LV_SYMBOL_DOWN);
    lv_obj_center(lbl_dn);
    *out_dn = btn_dn;

    lv_obj_t *btn_up = lv_btn_create(row);
    lv_obj_set_size(btn_up, 20, 20);
    lv_obj_align(btn_up, LV_ALIGN_RIGHT_MID, -24, 0);
    lv_obj_set_style_bg_color(btn_up, lv_color_hex(0x334455), 0);
    lv_obj_set_style_radius(btn_up, 3, 0);
    lv_obj_t *lbl_up = lv_label_create(btn_up);
    lv_label_set_text(lbl_up, LV_SYMBOL_UP);
    lv_obj_center(lbl_up);
    *out_up = btn_up;

    lv_obj_t *val = lv_label_create(row);
    lv_label_set_text(val, "-");
    lv_obj_set_style_text_color(val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_10, 0);
    lv_obj_align(val, LV_ALIGN_RIGHT_MID, -50, 0);
    lv_label_set_long_mode(val, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(val, 56);
    *out_value_lbl = val;

    return row;
}

void main_screen_ui_init(main_screen_ui_refs_t *refs) {
    if (refs == NULL) return;
    memset(refs, 0, sizeof(*refs));
}

void main_screen_ui_build(main_screen_ui_refs_t *refs,
                          lv_obj_t *panel_ctrl,
                          lv_obj_t *panel_mon,
                          int32_t panel_w,
                          int32_t monitor_w,
                          int32_t screen_h,
                          const char *const *drum_names,
                          const main_screen_ui_callbacks_t *callbacks) {
    if (refs == NULL || panel_ctrl == NULL || panel_mon == NULL ||
        drum_names == NULL || callbacks == NULL) {
        return;
    }

    lv_obj_t *title = lv_label_create(panel_ctrl);
    lv_label_set_text(title, "CONTROL");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8888CC), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    make_updown_row(panel_ctrl, panel_w, "Mode", 14,
                    &refs->mode_label, &refs->mode_up_btn, &refs->mode_dn_btn);
    lv_obj_add_event_cb(refs->mode_up_btn, callbacks->on_mode_up, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(refs->mode_dn_btn, callbacks->on_mode_dn, LV_EVENT_CLICKED, NULL);

    make_updown_row(panel_ctrl, panel_w, "Color", 46,
                    &refs->color_label, &refs->color_up_btn, &refs->color_dn_btn);
    lv_obj_add_event_cb(refs->color_up_btn, callbacks->on_color_up, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(refs->color_dn_btn, callbacks->on_color_dn, LV_EVENT_CLICKED, NULL);

    refs->submit_btn = lv_btn_create(panel_ctrl);
    lv_obj_set_size(refs->submit_btn, panel_w - 8, 26);
    lv_obj_align(refs->submit_btn, LV_ALIGN_TOP_LEFT, 0, 80);
    lv_obj_set_style_bg_color(refs->submit_btn, lv_color_hex(0x005599), 0);
    lv_obj_set_style_radius(refs->submit_btn, 4, 0);
    lv_obj_t *submit_lbl = lv_label_create(refs->submit_btn);
    lv_label_set_text(submit_lbl, "SELECT / SUBMIT");
    lv_obj_set_style_text_font(submit_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(submit_lbl);
    lv_obj_add_event_cb(refs->submit_btn, callbacks->on_submit, LV_EVENT_CLICKED, NULL);

    refs->sync_btn = lv_btn_create(panel_ctrl);
    lv_obj_set_size(refs->sync_btn, panel_w - 8, 26);
    lv_obj_align(refs->sync_btn, LV_ALIGN_TOP_LEFT, 0, 110);
    lv_obj_set_style_bg_color(refs->sync_btn, lv_color_hex(0x004422), 0);
    lv_obj_set_style_radius(refs->sync_btn, 4, 0);
    lv_obj_t *sync_lbl = lv_label_create(refs->sync_btn);
    lv_label_set_text(sync_lbl, "SYNC ALL");
    lv_obj_set_style_text_font(sync_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(sync_lbl);
    lv_obj_add_event_cb(refs->sync_btn, callbacks->on_sync_all, LV_EVENT_CLICKED, NULL);

    refs->to_settings_btn = lv_btn_create(panel_ctrl);
    lv_obj_set_size(refs->to_settings_btn, panel_w - 8, 24);
    lv_obj_align(refs->to_settings_btn, LV_ALIGN_TOP_LEFT, 0, 140);
    lv_obj_set_style_bg_color(refs->to_settings_btn, lv_color_hex(0x3B2F5A), 0);
    lv_obj_set_style_radius(refs->to_settings_btn, 4, 0);
    lv_obj_t *settings_lbl = lv_label_create(refs->to_settings_btn);
    lv_label_set_text(settings_lbl, "SETTINGS");
    lv_obj_set_style_text_font(settings_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(settings_lbl);
    lv_obj_add_event_cb(refs->to_settings_btn, callbacks->on_goto_settings, LV_EVENT_CLICKED, NULL);

    lv_obj_t *monitor_title = lv_label_create(panel_mon);
    lv_label_set_text(monitor_title, "DRUM MONITOR");
    lv_obj_set_style_text_color(monitor_title, lv_color_hex(0x88CCAA), 0);
    lv_obj_set_style_text_font(monitor_title, &lv_font_montserrat_10, 0);
    lv_obj_align(monitor_title, LV_ALIGN_TOP_MID, 0, 0);

    for (int i = 0; i < NUM_DRUM_NODES; i++) {
        lv_obj_t *btn = lv_obj_create(panel_mon);
        lv_obj_set_size(btn, NODE_W, NODE_H);
        lv_obj_set_pos(btn, NODE_X[i], NODE_Y[i]);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E1E3A), 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x444444), 0);
        lv_obj_set_style_radius(btn, 5, 0);
        lv_obj_set_style_pad_all(btn, 3, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, callbacks->on_node_click, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        refs->node_btn[i] = btn;

        lv_obj_t *name_lbl = lv_label_create(btn);
        lv_label_set_text(name_lbl, drum_names[i]);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(name_lbl, LV_ALIGN_TOP_MID, 0, 2);
        refs->node_label[i] = name_lbl;

        lv_obj_t *vbar = lv_bar_create(btn);
        lv_obj_set_size(vbar, NODE_W - 10, 7);
        lv_obj_align(vbar, LV_ALIGN_BOTTOM_MID, 0, -14);
        lv_bar_set_range(vbar, 0, 3300);
        lv_bar_set_value(vbar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(vbar, lv_color_hex(0x223344), 0);
        lv_obj_set_style_bg_color(lv_bar_get_indicator(vbar), lv_color_hex(0x00DDAA), 0);
        refs->node_vbar[i] = vbar;

        lv_obj_t *volt_lbl = lv_label_create(btn);
        lv_label_set_text(volt_lbl, "0mv");
        lv_obj_set_style_text_font(volt_lbl, &lv_font_montserrat_8, 0);
        lv_obj_set_style_text_color(volt_lbl, lv_color_hex(0x88CCBB), 0);
        lv_obj_align(volt_lbl, LV_ALIGN_BOTTOM_MID, 0, -2);
        refs->node_volt_lbl[i] = volt_lbl;

        lv_obj_t *thresh_line = lv_line_create(btn);
        s_thresh_pts[i][0].x = 0;
        s_thresh_pts[i][0].y = 0;
        s_thresh_pts[i][1].x = 6;
        s_thresh_pts[i][1].y = 0;
        lv_line_set_points(thresh_line, s_thresh_pts[i], 2);
        lv_obj_set_style_line_color(thresh_line, lv_color_hex(0xFF4444), 0);
        lv_obj_set_style_line_width(thresh_line, 2, 0);
        lv_obj_align(thresh_line, LV_ALIGN_BOTTOM_MID, 0, -14);
        refs->node_thresh_line[i] = thresh_line;
    }

    lv_obj_t *sep = lv_obj_create(panel_mon);
    lv_obj_set_size(sep, monitor_w - 8, 1);
    lv_obj_set_pos(sep, 4, 116);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    refs->info_list = lv_obj_create(panel_mon);
    lv_obj_set_size(refs->info_list, monitor_w - 8, screen_h - 120);
    lv_obj_set_pos(refs->info_list, 4, 118);
    lv_obj_set_style_bg_color(refs->info_list, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_border_width(refs->info_list, 0, 0);
    lv_obj_set_style_pad_all(refs->info_list, 1, 0);
    lv_obj_set_style_radius(refs->info_list, 0, 0);
    lv_obj_clear_flag(refs->info_list, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < NUM_DRUM_NODES; i++) {
        lv_obj_t *row = lv_label_create(refs->info_list);
        lv_obj_set_style_text_font(row, &lv_font_montserrat_8, 0);
        lv_obj_set_style_text_color(row, lv_color_hex(0xBBBBBB), 0);
        lv_obj_set_pos(row, 0, i * 12);
        lv_label_set_long_mode(row, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(row, monitor_w - 10);
        refs->info_rows[i] = row;
    }
}

void main_screen_ui_refresh_control_labels(main_screen_ui_refs_t *refs,
                                           int8_t selected_node,
                                           const drum_staging_t *staging,
                                           const char *const *mode_names) {
    if (refs == NULL || refs->mode_label == NULL || refs->color_label == NULL ||
        staging == NULL || mode_names == NULL) {
        return;
    }

    if (selected_node < 0 || selected_node >= NUM_DRUM_NODES) {
        lv_label_set_text(refs->mode_label, "-");
        lv_label_set_text(refs->color_label, "-");
        return;
    }

    drum_id_t id = (drum_id_t)selected_node;
    lv_label_set_text(refs->mode_label, mode_names[staging[id].mode]);

    char buf[12];
    snprintf(buf, sizeof(buf), "#%06lX",
             (unsigned long)(staging[id].color_rgb & 0xFFFFFFUL));
    lv_label_set_text(refs->color_label, buf);
}

void main_screen_ui_refresh_node_widget(main_screen_ui_refs_t *refs,
                                        drum_id_t id,
                                        const drum_telemetry_t *tel,
                                        int32_t voltage_max_mv,
                                        lv_color_t online_color,
                                        lv_color_t offline_color) {
    if (refs == NULL || tel == NULL) return;
    if ((uint8_t)id >= NUM_DRUM_NODES) return;

    if (refs->node_vbar[id] == NULL || refs->node_volt_lbl[id] == NULL ||
        refs->node_thresh_line[id] == NULL || refs->node_btn[id] == NULL) {
        return;
    }

    lv_bar_set_value(refs->node_vbar[id], (int32_t)tel->live_voltage_mv, LV_ANIM_OFF);

    char vbuf[10];
    snprintf(vbuf, sizeof(vbuf), "%umv", tel->live_voltage_mv);
    lv_label_set_text(refs->node_volt_lbl[id], vbuf);

    int bar_px = NODE_W - 10;
    int thr_px = (int)((long)tel->threshold_mv * bar_px / voltage_max_mv);
    if (thr_px < 0) thr_px = 0;
    if (thr_px > bar_px) thr_px = bar_px;

    int bar_left_offset = -(bar_px / 2) + thr_px;
    lv_obj_align(refs->node_thresh_line[id], LV_ALIGN_BOTTOM_MID,
                 bar_left_offset - (bar_px / 2) + 5, -14);

    lv_obj_set_style_bg_color(refs->node_btn[id],
                              tel->is_connected ? online_color : offline_color,
                              0);
}

void main_screen_ui_refresh_info_row(main_screen_ui_refs_t *refs,
                                     drum_id_t id,
                                     const drum_config_t *cfg,
                                     const char *const *drum_names,
                                     const char *const *mode_names) {
    if (refs == NULL || cfg == NULL || drum_names == NULL || mode_names == NULL) return;
    if ((uint8_t)id >= NUM_DRUM_NODES) return;
    if (refs->info_rows[id] == NULL) return;

    char buf[48];
    snprintf(buf, sizeof(buf), "%-5s #%06lX  %s",
             drum_names[id],
             (unsigned long)(cfg[id].color_rgb & 0xFFFFFFUL),
             mode_names[cfg[id].mode]);
    lv_label_set_text(refs->info_rows[id], buf);
}

void main_screen_ui_set_selected(main_screen_ui_refs_t *refs,
                                 int8_t prev_selected,
                                 int8_t new_selected,
                                 lv_color_t selected_color,
                                 lv_color_t deselected_color) {
    if (refs == NULL) return;

    if (prev_selected >= 0 && prev_selected < NUM_DRUM_NODES && refs->node_btn[prev_selected] != NULL) {
        lv_obj_set_style_border_color(refs->node_btn[prev_selected], deselected_color, 0);
        lv_obj_set_style_border_width(refs->node_btn[prev_selected], 2, 0);
    }

    if (new_selected >= 0 && new_selected < NUM_DRUM_NODES && refs->node_btn[new_selected] != NULL) {
        lv_obj_set_style_border_color(refs->node_btn[new_selected], selected_color, 0);
        lv_obj_set_style_border_width(refs->node_btn[new_selected], 3, 0);
    }
}
