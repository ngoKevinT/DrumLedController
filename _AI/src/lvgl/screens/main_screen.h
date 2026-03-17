#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *panel_ctrl;
    lv_obj_t *panel_mon;
} main_screen_refs_t;

main_screen_refs_t main_screen_create(int32_t screen_w,
                                      int32_t screen_h,
                                      int32_t panel_w,
                                      int32_t monitor_w);

#ifdef __cplusplus
}
#endif
