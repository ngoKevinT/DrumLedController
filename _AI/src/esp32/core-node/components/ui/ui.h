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

/** Velocity-to-brightness mapping curve for a drum trigger. */
typedef enum {
    HIT_CURVE_LINEAR = 0,
    HIT_CURVE_LOG,
    HIT_CURVE_EXP,
    HIT_CURVE_COUNT,
} hit_curve_t;

/**
 * Live state for one connected (or placeholder unconnected) drum node.
 * Defined in ui.c; all screen modules read and write through these globals.
 * All writes must happen while the LVGL port mutex is held.
 *
 * Trigger fields are edited on the Drum Settings screen.
 * LED fields are edited on the Global Settings screen.
 */
typedef struct {
    /* Identity */
    char     name[24];           /**< Display name, e.g. "Kick"          */
    bool     connected;          /**< Paired and responding               */

    /* Trigger / sensing */
    uint16_t threshold;          /**< Sensitivity threshold (0–1023)      */
    uint16_t reading;            /**< Live trigger reading (read-only)    */
    uint16_t retrigger_ms;       /**< Retrigger guard window (0–500 ms)   */
    uint8_t  hit_curve;          /**< hit_curve_t — velocity mapping      */

    /* LED behaviour */
    uint32_t color;              /**< Base LED color, 0x00RRGGBB          */
    uint8_t  mode_idx;           /**< Index into ui_mode_names[]          */
    uint8_t  idle_brightness;    /**< Ambient glow level (0–100 %)        */
    uint8_t  flash_brightness;   /**< Peak flash level (0–100 %)          */
    uint16_t flash_time_ms;      /**< Static flash duration (10–2000 ms)  */
    uint16_t decay_time_ms;      /**< Fade decay time (50–3000 ms)        */
} ui_drum_t;

extern ui_drum_t         g_drums[UI_MAX_DRUMS];
extern int               g_selected_drum;   /**< 0-based index of selected card */
extern const char *const ui_mode_names[UI_NUM_MODES];

/** Global LED controls — apply to all drum nodes collectively. */
extern uint8_t g_master_brightness;  /**< 0-100 %: scales every node's output  */
extern bool    g_power_limit_3a;     /**< When true, cap draw to 3A protocol    */

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

/**
 * @brief Post a status string to the active screen's debug/status label.
 *
 * Safe to call from ANY FreeRTOS task (including the ESP-NOW/WiFi task).
 * The message is queued and applied by an LVGL timer on the next tick.
 * Messages longer than 63 characters are truncated.
 */
void ui_post_status(const char *msg);
