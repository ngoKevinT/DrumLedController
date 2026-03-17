#include "loading_screen.h"

lv_obj_t *loading_screen_create(int32_t width, int32_t height) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, width, height);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "DRUM LED CONTROLLER");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -18);

    lv_obj_t *status = lv_label_create(screen);
    lv_label_set_text(status, "Initializing UI and nodes...");
    lv_obj_set_style_text_color(status, lv_color_hex(0x93C5FD), 0);
    lv_obj_set_style_text_font(status, &lv_font_montserrat_10, 0);
    lv_obj_align(status, LV_ALIGN_CENTER, 0, 8);

    lv_obj_t *spin = lv_spinner_create(screen);
    lv_obj_set_size(spin, 20, 20);
    lv_obj_align(spin, LV_ALIGN_CENTER, 0, 34);

    return screen;
}
