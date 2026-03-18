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
#include "rgb_lcd_port.h"   // waveshare_esp32_s3_rgb_lcd_init(), wavesahre_rgb_lcd_bl_on()
#include "gt911.h"          // touch_gt911_init()
#include "lvgl_port.h"      // lvgl_port_init(), lvgl_port_lock(), lvgl_port_unlock()
#include "ui.h"             // ui_create()

static const char *TAG = "main";

void app_main(void)
{
    static esp_lcd_panel_handle_t panel_handle = NULL;
    static esp_lcd_touch_handle_t tp_handle    = NULL;

    // Touch must be initialised before the LCD so the GT911 INT pin level
    // sets the correct I2C address (0x5D) before the panel comes up.
    tp_handle    = touch_gt911_init();
    panel_handle = waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();

    // Start the LVGL FreeRTOS task, tick timer, display driver, and indev.
    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

    ESP_LOGI(TAG, "Building UI");

    // All LVGL API calls must be made while holding the port mutex.
    if (lvgl_port_lock(-1)) {
        ui_create();
        lvgl_port_unlock();
    }
}
