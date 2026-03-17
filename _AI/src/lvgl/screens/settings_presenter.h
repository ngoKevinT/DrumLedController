#pragma once

#include <lvgl.h>

#include "settings_screen.h"
#include "../ui_manager.h"

typedef struct {
    int8_t selected_node_idx;
    settings_screen_widget_refs_t refs;
} settings_presenter_t;

void settings_presenter_init(settings_presenter_t *presenter,
                             const settings_screen_widget_refs_t *refs);

void settings_presenter_prev_node(settings_presenter_t *presenter, int32_t num_nodes);

void settings_presenter_next_node(settings_presenter_t *presenter, int32_t num_nodes);

void settings_presenter_refresh(settings_presenter_t *presenter,
                                const drum_telemetry_t *telemetry,
                                const drum_config_t *cfg,
                                const char *const *drum_names,
                                const char *const *mode_names,
                                int32_t num_nodes);
