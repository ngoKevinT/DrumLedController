#pragma once

/**
 * @brief Build and display the root UI screen.
 *
 * Must be called while the LVGL port mutex is held (lvgl_port_lock).
 * Owns the entire widget tree for the active screen — call once at startup.
 */
void ui_create(void);
