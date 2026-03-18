#pragma once

#include "lvgl.h"

/**
 * @brief Build the Drum Settings screen widget tree.
 * @param bg  Wallpaper descriptor preloaded into PSRAM by ui_create().
 *            Pass NULL to skip the background image.
 * Call once from ui_create() while the LVGL mutex is held.
 */
void ui_drum_settings_init(const lv_img_dsc_t *bg);

/**
 * @brief Return the Drum Settings screen object.
 * Valid after ui_drum_settings_init() has been called.
 */
lv_obj_t *ui_drum_settings_get_screen(void);
