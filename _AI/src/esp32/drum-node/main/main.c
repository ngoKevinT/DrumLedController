/*****************************************************************************
 * File        : main.c
 * Project     : ADLS Drum Node
 * Target      : Seeed Studio XIAO ESP32-S3
 * Description : Entry point — stub until hardware is available.
 *
 *   Responsibilities (to be implemented):
 *     - Piezo ADC sampling on ADC1_CH0 (GPIO 1) — high-priority task / ISR
 *     - Hit detection: threshold comparison, retrigger guard, velocity calc
 *     - LED control via FastLED / WS2812B on a single data pin
 *     - ESP-NOW uplink: send espnow_telemetry_pkt_t to the Core Node on each hit
 *     - ESP-NOW downlink: receive espnow_config_pkt_t from Core Node, apply settings
 *****************************************************************************/

#include "esp_log.h"
#include "espnow_transport.h"

static const char *TAG = "drum_node";

void app_main(void)
{
    ESP_LOGI(TAG, "Drum Node starting...");

    // TODO: init LED strip (FastLED / RMT)
    // TODO: init piezo ADC task
    // TODO: register espnow_transport_set_recv_cb() to handle config packets

    ESP_ERROR_CHECK(espnow_transport_init());

    ESP_LOGI(TAG, "Drum Node ready — awaiting pairing");

    // Main loop placeholder — real work happens in FreeRTOS tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
