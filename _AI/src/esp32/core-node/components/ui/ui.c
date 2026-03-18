/*****************************************************************************
 * File        : ui.c
 * Component   : ui
 * Description : Top-level screen manager for the ADLS Core Node.
 *
 *               Owns three screens:
 *                 UI_SCREEN_MAIN             — live dashboard (default)
 *                 UI_SCREEN_GLOBAL_SETTINGS  — brightness, power limits, pairing
 *                 UI_SCREEN_DRUM_SETTINGS    — per-node sensitivity / retrigger
 *
 *               Wallpaper BIN files are loaded from SPIFFS into PSRAM once at
 *               startup.  All subsequent rendering reads from PSRAM (~100 MB/s)
 *               rather than seeking through SPIFFS flash on every draw call,
 *               which would otherwise trigger the task watchdog and peg the CPU.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "ui.h"
#include "ui_main.h"
#include "ui_global_settings.h"
#include "ui_drum_settings.h"

static const char *TAG = "ui";

// ---------------------------------------------------------------------------
// Cross-task status messaging
// ---------------------------------------------------------------------------

#define STATUS_MSG_LEN  64
typedef char status_msg_t[STATUS_MSG_LEN];

// Queue holds up to 8 messages; written by any task, drained by LVGL timer.
static QueueHandle_t   s_status_queue  = NULL;
static ui_screen_t     g_active_screen = UI_SCREEN_MAIN;

void ui_post_status(const char *msg)
{
    if (!s_status_queue || !msg) return;
    status_msg_t buf = {0};
    strncpy(buf, msg, STATUS_MSG_LEN - 1);
    xQueueSend(s_status_queue, buf, 0);   // non-blocking; drop oldest if full
}

/** LVGL timer (250 ms) — drains the queue and updates the visible label. */
static void status_timer_cb(lv_timer_t *t)
{
    status_msg_t buf;
    while (xQueueReceive(s_status_queue, buf, 0) == pdTRUE) {
        switch (g_active_screen) {
        case UI_SCREEN_MAIN:
            ui_main_set_debug(buf);
            break;
        case UI_SCREEN_GLOBAL_SETTINGS:
            ui_global_settings_set_status(buf);
            break;
        default:
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Shared drum state — placeholder data for UI layout development.
// Replace with BLE/ESP-NOW telemetry once the comms layer is wired up.
// ---------------------------------------------------------------------------

ui_drum_t g_drums[UI_MAX_DRUMS] = {
    {
        .name="Kick",   .connected=true,
        .threshold=150, .reading=0, .retrigger_ms=30,  .hit_curve=HIT_CURVE_LINEAR,
        .color=0xFF2200, .mode_idx=1, .idle_brightness=20, .flash_brightness=100,
        .flash_time_ms=150, .decay_time_ms=300,
    },
    {
        .name="Snare",  .connected=true,
        .threshold=200, .reading=0, .retrigger_ms=25,  .hit_curve=HIT_CURVE_LINEAR,
        .color=0x0044FF, .mode_idx=0, .idle_brightness=15, .flash_brightness=100,
        .flash_time_ms=120, .decay_time_ms=250,
    },
    {
        .name="Tom 1",  .connected=false,
        .threshold=100, .reading=0, .retrigger_ms=40,  .hit_curve=HIT_CURVE_LOG,
        .color=0x00FF44, .mode_idx=2, .idle_brightness=10, .flash_brightness=80,
        .flash_time_ms=200, .decay_time_ms=400,
    },
    {
        .name="Tom 2",  .connected=false,
        .threshold=175, .reading=0, .retrigger_ms=40,  .hit_curve=HIT_CURVE_LOG,
        .color=0xFF8800, .mode_idx=3, .idle_brightness=10, .flash_brightness=80,
        .flash_time_ms=200, .decay_time_ms=400,
    },
    {
        .name="Tom 3",  .connected=false,
        .threshold=160, .reading=0, .retrigger_ms=40,  .hit_curve=HIT_CURVE_EXP,
        .color=0xFF00CC, .mode_idx=1, .idle_brightness=10, .flash_brightness=90,
        .flash_time_ms=180, .decay_time_ms=350,
    },
    {
        .name="Tom 4",  .connected=false,
        .threshold=120, .reading=0, .retrigger_ms=35,  .hit_curve=HIT_CURVE_LINEAR,
        .color=0x00CCFF, .mode_idx=4, .idle_brightness=10, .flash_brightness=85,
        .flash_time_ms=160, .decay_time_ms=300,
    },
};

int g_selected_drum = 0;

uint8_t g_master_brightness = 80;   // default 80 %
bool    g_power_limit_3a    = false;

const char *const ui_mode_names[UI_NUM_MODES] = {
    "SOLID", "PULSE", "STROBE", "RAINBOW", "CHASE", "BOUNCE", "CUSTOM"
};

// ---------------------------------------------------------------------------
// PSRAM wallpaper loader
// ---------------------------------------------------------------------------

/**
 * Read a BIN file from SPIFFS entirely into PSRAM and build an lv_img_dsc_t
 * that points into it.  The descriptor and pixel buffer are never freed;
 * they live for the lifetime of the application.
 *
 * LVGL BIN format: 4-byte lv_img_header_t  followed by raw pixel data.
 * Passing a pointer to lv_img_dsc_t to lv_img_set_src() makes LVGL treat it
 * as a RAM-resident C-array image — no FS driver, no seeks, no decoding.
 *
 * Returns NULL on failure (missing file or OOM); callers must handle this.
 */
static lv_img_dsc_t *load_wallpaper(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "load_wallpaper: cannot open %s", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    // Allocate the full file in PSRAM (wallpapers are ~1.2 MB each).
    uint8_t *buf = heap_caps_malloc((size_t)size, MALLOC_CAP_SPIRAM);
    if (!buf) {
        ESP_LOGE(TAG, "load_wallpaper: PSRAM alloc failed (%ld B) for %s", size, path);
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)size, f);
    fclose(f);
    if ((long)n != size) {
        ESP_LOGE(TAG, "load_wallpaper: short read %zu/%ld for %s", n, size, path);
        heap_caps_free(buf);
        return NULL;
    }

    // Build the descriptor.  The 4-byte header (lv_img_header_t) is the first
    // thing in every BIN file; pixel data follows immediately.
    lv_img_dsc_t *dsc = malloc(sizeof(lv_img_dsc_t));
    if (!dsc) {
        heap_caps_free(buf);
        return NULL;
    }
    memcpy(&dsc->header, buf, sizeof(lv_img_header_t));
    dsc->data_size = (uint32_t)(size - (long)sizeof(lv_img_header_t));
    dsc->data      = buf + sizeof(lv_img_header_t);

    ESP_LOGI(TAG, "Loaded %-30s  %ld B → PSRAM  w=%u h=%u",
             path, size, dsc->header.w, dsc->header.h);
    return dsc;
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void ui_navigate_to(ui_screen_t screen)
{
    lv_obj_t *target = NULL;
    switch (screen) {
    case UI_SCREEN_MAIN:            target = ui_main_get_screen();            break;
    case UI_SCREEN_GLOBAL_SETTINGS: target = ui_global_settings_get_screen(); break;
    case UI_SCREEN_DRUM_SETTINGS:   target = ui_drum_settings_get_screen();   break;
    default: return;
    }
    g_active_screen = screen;
    lv_scr_load(target);
}

// ---------------------------------------------------------------------------
// Startup
// ---------------------------------------------------------------------------

void ui_create(void)
{
    // Load wallpapers from SPIFFS into PSRAM before building any screens.
    // fopen/fread are plain C stdio — no LVGL API, so the LVGL mutex is not
    // required here even though we are called while it is held.
    lv_img_dsc_t *wp_main   = load_wallpaper("/spiffs/wallpaper2.bin");
    lv_img_dsc_t *wp_global = load_wallpaper("/spiffs/wallpaper1.bin");
    lv_img_dsc_t *wp_drum   = load_wallpaper("/spiffs/wallpaper4.bin");

    // Build all screens up-front so navigation between them is instant.
    ui_main_init(wp_main);
    ui_global_settings_init(wp_global);
    ui_drum_settings_init(wp_drum);

    // Status queue and timer — must be created after all screens are built.
    s_status_queue = xQueueCreate(8, STATUS_MSG_LEN);
    lv_timer_create(status_timer_cb, 250, NULL);

    ui_navigate_to(UI_SCREEN_MAIN);
}
