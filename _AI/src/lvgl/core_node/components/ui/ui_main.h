#pragma once

#include "lvgl.h"

/**
 * @brief Build the main dashboard screen widget tree.
 * Call once from ui_create() while the LVGL mutex is held.
 */
void ui_main_init(void);

/**
 * @brief Return the main dashboard screen object.
 * Valid after ui_main_init() has been called.
 */
lv_obj_t *ui_main_get_screen(void);
