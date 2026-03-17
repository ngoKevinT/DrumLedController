#include "settings_screen.h"

lv_obj_t *settings_screen_create(int32_t width,
                                 int32_t height,
                                 const settings_screen_callbacks_t *callbacks,
                                 settings_screen_widget_refs_t *out_refs) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, width, height);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101827), 0);
    lv_obj_set_style_pad_all(screen, 8, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_color(title, lv_color_hex(0xD1FAE5), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *connected_lbl = lv_label_create(screen);
    lv_label_set_text(connected_lbl, "Connected nodes: 0 / 4");
    lv_obj_set_style_text_color(connected_lbl, lv_color_hex(0xA7F3D0), 0);
    lv_obj_set_style_text_font(connected_lbl, &lv_font_montserrat_10, 0);
    lv_obj_align(connected_lbl, LV_ALIGN_TOP_LEFT, 0, 20);

    lv_obj_t *global_title = lv_label_create(screen);
    lv_label_set_text(global_title, "Global Defaults");
    lv_obj_set_style_text_color(global_title, lv_color_hex(0xBFDBFE), 0);
    lv_obj_set_style_text_font(global_title, &lv_font_montserrat_12, 0);
    lv_obj_align(global_title, LV_ALIGN_TOP_LEFT, 0, 40);

    lv_obj_t *sens_lbl = lv_label_create(screen);
    lv_label_set_text(sens_lbl, "Sensitivity");
    lv_obj_set_style_text_font(sens_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(sens_lbl, lv_color_hex(0xE5E7EB), 0);
    lv_obj_align(sens_lbl, LV_ALIGN_TOP_LEFT, 0, 58);

    lv_obj_t *sens_slider = lv_slider_create(screen);
    lv_obj_set_size(sens_slider, 140, 10);
    lv_obj_align(sens_slider, LV_ALIGN_TOP_LEFT, 88, 62);
    lv_slider_set_range(sens_slider, 0, 255);
    lv_slider_set_value(sens_slider, 128, LV_ANIM_OFF);

    lv_obj_t *retr_lbl = lv_label_create(screen);
    lv_label_set_text(retr_lbl, "Retrigger ms");
    lv_obj_set_style_text_font(retr_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(retr_lbl, lv_color_hex(0xE5E7EB), 0);
    lv_obj_align(retr_lbl, LV_ALIGN_TOP_LEFT, 0, 80);

    lv_obj_t *retr_slider = lv_slider_create(screen);
    lv_obj_set_size(retr_slider, 140, 10);
    lv_obj_align(retr_slider, LV_ALIGN_TOP_LEFT, 88, 84);
    lv_slider_set_range(retr_slider, 1, 120);
    lv_slider_set_value(retr_slider, 30, LV_ANIM_OFF);

    lv_obj_t *node_title = lv_label_create(screen);
    lv_label_set_text(node_title, "Connected Drum Node Settings");
    lv_obj_set_style_text_color(node_title, lv_color_hex(0xFDE68A), 0);
    lv_obj_set_style_text_font(node_title, &lv_font_montserrat_12, 0);
    lv_obj_align(node_title, LV_ALIGN_TOP_LEFT, 0, 106);

    lv_obj_t *prev_btn = lv_btn_create(screen);
    lv_obj_set_size(prev_btn, 22, 18);
    lv_obj_align(prev_btn, LV_ALIGN_TOP_LEFT, 0, 124);
    lv_obj_t *prev_lbl = lv_label_create(prev_btn);
    lv_label_set_text(prev_lbl, "<");
    lv_obj_center(prev_lbl);

    lv_obj_t *next_btn = lv_btn_create(screen);
    lv_obj_set_size(next_btn, 22, 18);
    lv_obj_align(next_btn, LV_ALIGN_TOP_LEFT, 140, 124);
    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, ">");
    lv_obj_center(next_lbl);

    lv_obj_t *node_lbl = lv_label_create(screen);
    lv_label_set_text(node_lbl, "Node: SNARE");
    lv_obj_set_style_text_font(node_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(node_lbl, lv_color_hex(0xFEF3C7), 0);
    lv_obj_align(node_lbl, LV_ALIGN_TOP_LEFT, 28, 127);

    lv_obj_t *mode_lbl = lv_label_create(screen);
    lv_label_set_text(mode_lbl, "Mode: Pure Reactive");
    lv_obj_set_style_text_font(mode_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(mode_lbl, lv_color_hex(0xE5E7EB), 0);
    lv_obj_align(mode_lbl, LV_ALIGN_TOP_LEFT, 0, 146);

    lv_obj_t *color_lbl = lv_label_create(screen);
    lv_label_set_text(color_lbl, "Color: #FFFFFF");
    lv_obj_set_style_text_font(color_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(color_lbl, lv_color_hex(0xE5E7EB), 0);
    lv_obj_align(color_lbl, LV_ALIGN_TOP_LEFT, 0, 158);

    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 84, 22);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back to Main");
    lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(back_lbl);

    if (callbacks != NULL) {
        if (callbacks->on_back != NULL) {
            lv_obj_add_event_cb(back_btn, callbacks->on_back, LV_EVENT_CLICKED, NULL);
        }
        if (callbacks->on_prev_node != NULL) {
            lv_obj_add_event_cb(prev_btn, callbacks->on_prev_node, LV_EVENT_CLICKED, NULL);
        }
        if (callbacks->on_next_node != NULL) {
            lv_obj_add_event_cb(next_btn, callbacks->on_next_node, LV_EVENT_CLICKED, NULL);
        }
    }

    if (out_refs != NULL) {
        out_refs->connected_lbl = connected_lbl;
        out_refs->node_lbl = node_lbl;
        out_refs->mode_lbl = mode_lbl;
        out_refs->color_lbl = color_lbl;
        out_refs->sens_slider = sens_slider;
        out_refs->retr_slider = retr_slider;
    }

    return screen;
}
