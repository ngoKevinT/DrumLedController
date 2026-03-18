#pragma once

#include <stdbool.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Screen identifiers
// ---------------------------------------------------------------------------

typedef enum {
    UI_SCREEN_MAIN = 0,
    UI_SCREEN_GLOBAL_SETTINGS,
    UI_SCREEN_DRUM_SETTINGS,
} ui_screen_t;

// ---------------------------------------------------------------------------
// Shared drum data model
// ---------------------------------------------------------------------------

#define UI_MAX_DRUMS  6
#define UI_NUM_MODES  7

/**
 * Live state for one connected (or placeholder unconnected) drum node.
 * Defined in ui.c; all screen modules read and write through these globals.
 * All writes must happen while the LVGL port mutex is held.
 */
typedef struct {
    char     name[24];   /**< Display name, e.g. "Kick"  */
    uint32_t color;      /**< LED color, 0x00RRGGBB       */
    uint8_t  mode_idx;   /**< Index into ui_mode_names[]  */
    uint16_t threshold;  /**< Target hit threshold        */
    uint16_t reading;    /**< Current trigger reading     */
    bool     connected;  /**< Paired and responding       */
} ui_drum_t;

extern ui_drum_t         g_drums[UI_MAX_DRUMS];
extern int               g_selected_drum;   /**< 0-based index of selected card */
extern const char *const ui_mode_names[UI_NUM_MODES];

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
