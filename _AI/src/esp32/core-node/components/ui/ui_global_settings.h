#pragma once

#include "lvgl.h"

/**
 * @brief Build the Global Settings screen widget tree.
 * @param bg  Wallpaper descriptor preloaded into PSRAM by ui_create().
 *            Pass NULL to skip the background image.
 * Call once from ui_create() while the LVGL mutex is held.
 */
void ui_global_settings_init(const lv_img_dsc_t *bg);

/**
 * @brief Return the Global Settings screen object.
 * Valid after ui_global_settings_init() has been called.
 */
lv_obj_t *ui_global_settings_get_screen(void);

/**
 * @brief Sync all widgets to current g_drums[g_selected_drum] and global state.
 *
 * Call before navigating to this screen so the sliders and toggle reflect the
 * latest values.  Must be called while the LVGL port mutex is held.
 */
void ui_global_settings_refresh(void);

/**
 * @brief Update the CONNECTION status label with @p msg.
 * Must be called while the LVGL port mutex is held (use ui_post_status()
 * from non-LVGL tasks instead).
 */
void ui_global_settings_set_status(const char *msg);
