#pragma once

#include <stdbool.h>

#include <lvgl.h>

#include "ui_manager.h"

typedef struct {
    lv_obj_t *loading_screen;
    lv_obj_t *main_screen;
    lv_obj_t *settings_screen;
    ui_screen_t active_screen;
} screen_router_t;

void screen_router_init(screen_router_t *router,
                        lv_obj_t *loading_screen,
                        lv_obj_t *main_screen,
                        lv_obj_t *settings_screen);

bool screen_router_show(screen_router_t *router, ui_screen_t screen);

ui_screen_t screen_router_get_active(const screen_router_t *router);
