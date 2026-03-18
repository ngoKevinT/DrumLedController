#pragma once

#include "lvgl.h"

/**
 * @brief Build the Global Settings screen widget tree.
 * Call once from ui_create() while the LVGL mutex is held.
 */
void ui_global_settings_init(void);

/**
 * @brief Return the Global Settings screen object.
 * Valid after ui_global_settings_init() has been called.
 */
lv_obj_t *ui_global_settings_get_screen(void);
