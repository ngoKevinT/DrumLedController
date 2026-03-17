#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *connected_lbl;
    lv_obj_t *node_lbl;
    lv_obj_t *mode_lbl;
    lv_obj_t *color_lbl;
    lv_obj_t *sens_slider;
    lv_obj_t *retr_slider;
} settings_screen_widget_refs_t;

typedef struct {
    lv_event_cb_t on_back;
    lv_event_cb_t on_prev_node;
    lv_event_cb_t on_next_node;
} settings_screen_callbacks_t;

lv_obj_t *settings_screen_create(int32_t width,
                                 int32_t height,
                                 const settings_screen_callbacks_t *callbacks,
                                 settings_screen_widget_refs_t *out_refs);

#ifdef __cplusplus
}
#endif
