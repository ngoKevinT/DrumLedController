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

/**
 * @brief Re-populate the Drum Settings screen with g_drums[g_selected_drum].
 *
 * Call this immediately before navigating to UI_SCREEN_DRUM_SETTINGS so the
 * title and info labels always reflect the currently selected drum card.
 * Must be called while the LVGL port mutex is held.
 */
void ui_drum_settings_refresh(void);
