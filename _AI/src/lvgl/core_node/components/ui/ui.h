#pragma once

/**
 * Screen identifiers used by ui_navigate_to().
 */
typedef enum {
    UI_SCREEN_MAIN = 0,
    UI_SCREEN_GLOBAL_SETTINGS,
    UI_SCREEN_DRUM_SETTINGS,
} ui_screen_t;

/**
 * @brief Initialise all screens and display the main dashboard.
 *
 * Must be called while the LVGL port mutex is held (lvgl_port_lock).
 * Call once at startup — screens are retained in memory for instant navigation.
 */
void ui_create(void);

/**
 * @brief Switch the active display to the requested screen.
 *
 * Must be called while the LVGL port mutex is held.
 */
void ui_navigate_to(ui_screen_t screen);
