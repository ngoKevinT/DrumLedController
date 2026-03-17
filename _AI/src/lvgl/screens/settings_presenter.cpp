#include "settings_presenter.h"

#include <stdio.h>
#include <string.h>

void settings_presenter_init(settings_presenter_t *presenter,
                             const settings_screen_widget_refs_t *refs) {
    if (presenter == NULL) return;
    memset(presenter, 0, sizeof(*presenter));
    presenter->selected_node_idx = 0;
    if (refs != NULL) {
        presenter->refs = *refs;
    }
}

void settings_presenter_prev_node(settings_presenter_t *presenter, int32_t num_nodes) {
    if (presenter == NULL || num_nodes <= 0) return;
    presenter->selected_node_idx = (int8_t)((presenter->selected_node_idx - 1 + num_nodes) % num_nodes);
}

void settings_presenter_next_node(settings_presenter_t *presenter, int32_t num_nodes) {
    if (presenter == NULL || num_nodes <= 0) return;
    presenter->selected_node_idx = (int8_t)((presenter->selected_node_idx + 1) % num_nodes);
}

void settings_presenter_refresh(settings_presenter_t *presenter,
                                const drum_telemetry_t *telemetry,
                                const drum_config_t *cfg,
                                const char *const *drum_names,
                                const char *const *mode_names,
                                int32_t num_nodes) {
    if (presenter == NULL || telemetry == NULL || cfg == NULL || drum_names == NULL ||
        mode_names == NULL || num_nodes <= 0) {
        return;
    }

    if (presenter->selected_node_idx < 0 || presenter->selected_node_idx >= num_nodes) {
        presenter->selected_node_idx = 0;
    }

    int connected = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (telemetry[i].is_connected) connected++;
    }

    char buf[40];
    if (presenter->refs.connected_lbl != NULL) {
        snprintf(buf, sizeof(buf), "Connected nodes: %d / %d", connected, num_nodes);
        lv_label_set_text(presenter->refs.connected_lbl, buf);
    }

    drum_id_t id = (drum_id_t)presenter->selected_node_idx;
    if (presenter->refs.node_lbl != NULL) {
        snprintf(buf, sizeof(buf), "Node: %s", drum_names[id]);
        lv_label_set_text(presenter->refs.node_lbl, buf);
    }

    if (presenter->refs.mode_lbl != NULL) {
        snprintf(buf, sizeof(buf), "Mode: %s", mode_names[cfg[id].mode]);
        lv_label_set_text(presenter->refs.mode_lbl, buf);
    }

    if (presenter->refs.color_lbl != NULL) {
        snprintf(buf, sizeof(buf), "Color: #%06lX",
                 (unsigned long)(cfg[id].color_rgb & 0xFFFFFFUL));
        lv_label_set_text(presenter->refs.color_lbl, buf);
    }

    if (presenter->refs.sens_slider != NULL) {
        lv_slider_set_value(presenter->refs.sens_slider, cfg[id].sensitivity, LV_ANIM_OFF);
    }

    if (presenter->refs.retr_slider != NULL) {
        lv_slider_set_value(presenter->refs.retr_slider, cfg[id].retrigger_ms, LV_ANIM_OFF);
    }
}
