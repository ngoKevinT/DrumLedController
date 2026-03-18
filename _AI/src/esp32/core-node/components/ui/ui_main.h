#pragma once

#include "lvgl.h"

/**
 * @brief Build the main dashboard screen widget tree.
 * @param bg  Wallpaper descriptor preloaded into PSRAM by ui_create().
 *            Pass NULL to skip the background image.
 * Call once from ui_create() while the LVGL mutex is held.
 */
void ui_main_init(const lv_img_dsc_t *bg);

/**
 * @brief Return the main dashboard screen object.
 * Valid after ui_main_init() has been called.
 */
lv_obj_t *ui_main_get_screen(void);

/**
 * @brief Refresh the visual state of one drum card from g_drums[idx].
 *
 * Call after writing new telemetry into g_drums[idx].
 * Must be called while the LVGL port mutex is held.
 */
void ui_main_drum_update(int idx);

/**
 * @brief Flash card[idx] to the drum's LED color for 300ms then restore.
 *
 * Safe to call on rapid successive hits — any in-progress flash is cancelled
 * and restarted.  Must be called while the LVGL port mutex is held.
 */
void ui_main_drum_flash(int idx);

/**
 * @brief Update the debug label at the top of the right panel.
 * Must be called while the LVGL port mutex is held (use ui_post_status()
 * from non-LVGL tasks instead).
 */
void ui_main_set_debug(const char *msg);
