#include "main_screen.h"

main_screen_refs_t main_screen_create(int32_t screen_w,
                                      int32_t screen_h,
                                      int32_t panel_w,
                                      int32_t monitor_w) {
    main_screen_refs_t refs = {0};

    refs.screen = lv_obj_create(NULL);
    lv_obj_set_size(refs.screen, screen_w, screen_h);
    lv_obj_set_style_bg_color(refs.screen, lv_color_hex(0x111111), 0);
    lv_obj_set_style_pad_all(refs.screen, 0, 0);

    refs.panel_ctrl = lv_obj_create(refs.screen);
    lv_obj_set_size(refs.panel_ctrl, panel_w, screen_h);
    lv_obj_align(refs.panel_ctrl, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(refs.panel_ctrl, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(refs.panel_ctrl, 0, 0);
    lv_obj_set_style_pad_all(refs.panel_ctrl, 4, 0);
    lv_obj_set_style_radius(refs.panel_ctrl, 0, 0);

    refs.panel_mon = lv_obj_create(refs.screen);
    lv_obj_set_size(refs.panel_mon, monitor_w, screen_h);
    lv_obj_align(refs.panel_mon, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(refs.panel_mon, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_border_width(refs.panel_mon, 0, 0);
    lv_obj_set_style_pad_all(refs.panel_mon, 4, 0);
    lv_obj_set_style_radius(refs.panel_mon, 0, 0);

    return refs;
}
