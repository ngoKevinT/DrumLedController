/*****************************************************************************
 * File        : main.c
 * Project     : ADLS Core Node — Boilerplate
 * Description : Hardware initialisation entry point.
 *               Brings up the RGB LCD and GT911 touch controller, hands the
 *               panel handles to the LVGL port, then delegates all UI
 *               construction to the `ui` component.
 *
 *               Nothing UI-related lives here — this file's only job is to
 *               wire hardware to the LVGL runtime and call ui_create().
 *****************************************************************************/

#include "esp_log.h"
#include "esp_spiffs.h"
#include "rgb_lcd_port.h"        // waveshare_esp32_s3_rgb_lcd_init(), wavesahre_rgb_lcd_bl_on()
#include "gt911.h"               // touch_gt911_init()
#include "lvgl_port.h"           // lvgl_port_init(), lvgl_port_lock(), lvgl_port_unlock()
#include "ui.h"                  // ui_create()
#include "espnow_transport.h"    // espnow_transport_init(), espnow_transport_start_pairing()

static const char *TAG = "main";

static void spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path              = "/spiffs",
        .partition_label        = NULL,   // uses first SPIFFS partition found
        .max_files              = 8,
        .format_if_mount_failed = false,
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS mounted at /spiffs");
    }
}

void app_main(void)
{
    static esp_lcd_panel_handle_t panel_handle = NULL;
    static esp_lcd_touch_handle_t tp_handle    = NULL;

    // ESP-NOW: init radio first, then register the UI status callback.
    // The pairing session starts AFTER ui_create() so the queue exists
    // and all messages are visible on screen.
    ESP_ERROR_CHECK(espnow_transport_init());
    espnow_transport_set_status_cb(ui_post_status);

    // Mount SPIFFS before LVGL so image assets are available when ui_create()
    // calls lv_img_set_src() with "S:/..." paths.
    spiffs_init();

    // Touch must be initialised before the LCD so the GT911 INT pin level
    // sets the correct I2C address (0x5D) before the panel comes up.
    tp_handle    = touch_gt911_init();
    panel_handle = waveshare_esp32_s3_rgb_lcd_init();

    // Backlight intentionally held OFF until after the first frame is rendered.
    // This eliminates the blank-white flash that would otherwise be visible
    // during LVGL init (~20-50ms). The display goes dark → fully drawn in one cut.

    // Start the LVGL FreeRTOS task, tick timer, display driver, and indev.
    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

    ESP_LOGI(TAG, "Building UI");

    // All LVGL API calls must be made while holding the port mutex.
    if (lvgl_port_lock(-1)) {
        lv_fs_stdio_init();   // register drive 'S' → /spiffs
        ui_create();
        lvgl_port_unlock();
    }

    // First frame is now queued. Turn the backlight on — the user sees the
    // finished screen with no intermediate blank or partial state.
    wavesahre_rgb_lcd_bl_on();

    // Auto-start a 30 s pairing window on boot so nearby drum nodes can
    // announce themselves as soon as the UI is visible.
    espnow_transport_start_pairing(30);
}
