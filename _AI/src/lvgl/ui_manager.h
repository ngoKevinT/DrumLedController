#pragma once

#include <lvgl.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define UI_WEAK __attribute__((weak))
#else
#define UI_WEAK
#endif

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

#define NUM_DRUM_NODES   4
#define NUM_LED_MODES    7

// LilyGo T-Display-S3: 320 x 170 landscape
#define SCREEN_W         320
#define SCREEN_H         170

#define PANEL_W          (SCREEN_W / 2)   // 160 – Control Panel (left)
#define MONITOR_W        (SCREEN_W / 2)   // 160 – Drum Monitor  (right)

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------

/** Logical index for each drum in the kit. */
typedef enum {
    DRUM_SNARE  = 0,
    DRUM_T_TOM  = 1,
    DRUM_F_TOM  = 2,
    DRUM_KICK   = 3
} drum_id_t;

/** Artistic LED modes (mirrors modes_logic.md). */
typedef enum {
    MODE_PURE_REACTIVE  = 0,
    MODE_HEAT_MAP       = 1,
    MODE_90S_RAVE       = 2,
    MODE_GHOST_NOTE     = 3,
    MODE_SPECTRUM       = 4,
    MODE_TRAILING_EDGE  = 5,
    MODE_VOID           = 6
} led_mode_t;

/** App-level screens managed by the UI manager. */
typedef enum {
    UI_SCREEN_LOADING = 0,
    UI_SCREEN_MAIN    = 1,
    UI_SCREEN_SETTINGS = 2
} ui_screen_t;

// ---------------------------------------------------------------------------
// Data Structures
// ---------------------------------------------------------------------------

/**
 * @brief Per-node live telemetry received from a Drum Node via ESP-NOW.
 *        Populated externally (e.g., esp_now_receive callback).
 */
typedef struct {
    uint16_t live_voltage_mv;   ///< ADC reading in millivolts  (0–3300)
    uint16_t threshold_mv;      ///< Current trigger threshold  (0–3300)
    bool     is_connected;      ///< True if a recent heartbeat was received
} drum_telemetry_t;

/**
 * @brief Canonical per-node configuration stored on the Core Node and
 *        transmitted to a Drum Node only when SELECT/SUBMIT is pressed.
 */
typedef struct {
    uint32_t   color_rgb;       ///< 0x00RRGGBB packed color
    led_mode_t mode;            ///< Active LED mode
    uint8_t    sensitivity;     ///< 0–255, maps to detection threshold
    uint8_t    retrigger_ms;    ///< Refractory / mask period in ms
} drum_config_t;

/**
 * @brief Staging area: edits made via the UI live here until submitted.
 *        Mirrors drum_config_t but is node-local and never transmitted
 *        until ui_submit_staging() is called.
 */
typedef struct {
    uint32_t   color_rgb;
    led_mode_t mode;
} drum_staging_t;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialise LVGL objects and register all event callbacks.
 *         Must be called once after lv_init() and display driver setup.
 */
void ui_manager_init(void);

/**
 * @brief  Must be called from the main loop (or a FreeRTOS task) to pump
 *         LVGL's internal timer and re-draw dirty widgets.
 *         Call at least every 5 ms.
 */
void ui_manager_tick(void);

/**
 * @brief  Push updated telemetry from an ESP-NOW receive callback into the
 *         UI.  Thread-safe: uses lv_async_call internally.
 *
 * @param  drum_id   Which drum node sent the update.
 * @param  tel       Pointer to the new telemetry snapshot (copied internally).
 */
void ui_update_telemetry(drum_id_t drum_id, const drum_telemetry_t *tel);

/**
 * @brief  Commit the staging variables for the currently selected drum node
 *         to its canonical DrumState and queue an ESP-NOW transmission.
 *         Called automatically by the SELECT/SUBMIT button callback.
 *
 * @param  drum_id   Node to commit.
 */
void ui_submit_staging(drum_id_t drum_id);

/**
 * @brief  Trigger the "Sync All" operation: submits staging for every node
 *         and broadcasts a global sync packet.
 *         Called automatically by the SYNC button callback.
 */
void ui_sync_all(void);

/**
 * @brief Show one of the app screens.
 *
 * @param screen  Target screen to display.
 */
void ui_show_screen(ui_screen_t screen);

/**
 * @brief Get the currently active app screen.
 */
ui_screen_t ui_get_active_screen(void);

/**
 * @brief  Return a pointer to the canonical config for a given node.
 *         Read-only; do not write directly – use the staging workflow.
 */
const drum_config_t *ui_get_drum_config(drum_id_t drum_id);

/**
 * @brief  Return a pointer to the live staging data for a given node.
 *         Read-only externally; the UI mutates this internally.
 */
const drum_staging_t *ui_get_staging(drum_id_t drum_id);

// ---------------------------------------------------------------------------
// Weak / override-able hooks
// ---------------------------------------------------------------------------

/**
 * @brief  Called when a config packet should be sent to a Drum Node.
 *         Override this in your ESP-NOW layer.
 *
 * @param  drum_id   Target node.
 * @param  cfg       Config to transmit.
 */
UI_WEAK void ui_transmit_config(drum_id_t drum_id, const drum_config_t *cfg);

#ifdef __cplusplus
}
#endif
