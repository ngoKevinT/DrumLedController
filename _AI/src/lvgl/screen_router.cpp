#include "screen_router.h"

void screen_router_init(screen_router_t *router,
                        lv_obj_t *loading_screen,
                        lv_obj_t *main_screen,
                        lv_obj_t *settings_screen) {
    if (router == NULL) return;
    router->loading_screen = loading_screen;
    router->main_screen = main_screen;
    router->settings_screen = settings_screen;
    router->active_screen = UI_SCREEN_LOADING;
}

bool screen_router_show(screen_router_t *router, ui_screen_t screen) {
    if (router == NULL) return false;

    lv_obj_t *target = NULL;
    switch (screen) {
        case UI_SCREEN_LOADING:
            target = router->loading_screen;
            break;
        case UI_SCREEN_MAIN:
            target = router->main_screen;
            break;
        case UI_SCREEN_SETTINGS:
            target = router->settings_screen;
            break;
        default:
            return false;
    }

    if (target == NULL) return false;

    lv_screen_load(target);
    router->active_screen = screen;
    return true;
}

ui_screen_t screen_router_get_active(const screen_router_t *router) {
    if (router == NULL) return UI_SCREEN_LOADING;
    return router->active_screen;
}
