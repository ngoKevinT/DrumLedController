/*****************************************************************************
 * File        : main.c
 * Project     : ADLS Drum Node — Hello World / Board Bring-Up
 * Target      : Seeed Studio XIAO ESP32-S3
 * Description : Minimal firmware to verify board functionality before the
 *               LED strip and piezo sensor are wired up.
 *
 *   What this verifies:
 *     1. Onboard LED blinks at 1 Hz → firmware boots, FreeRTOS is running
 *     2. Test button (D3) → immediate ESP-NOW ping sent; LED flashes fast
 *     3. Periodic heartbeat → Core Node sees this node in its serial log
 *     4. Receive path → onboard LED speeds up when any ESP-NOW packet arrives
 *
 *   Pin assignments: see pinout.h — source of truth is todo.md.
 *
 *   Upgrade path:
 *     Phase 2 — add ADC trigger task (A0 ch1, D1 ch2), LED strip RMT on D9,
 *               PAIR_REQ/ACK handshake, NVS config persistence
 *     Phase 3 — wire cb_save() config packets → apply lighting mode
 *****************************************************************************/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
// #include "espnow_transport.h"  // Phase 1 — not yet implemented
#include "pinout.h"

static const char *TAG = "drum_node";

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/** This node's ID broadcast in every outbound packet header.
 *  Will be loaded from NVS in Phase 2 to support multiple drum nodes. */
static const uint8_t DRUM_NODE_ID = 1;

/** How often to broadcast a heartbeat ping when idle. */
static const uint32_t HEARTBEAT_INTERVAL_MS = 5000;

/** Onboard LED blink period at idle — 1 Hz, readable at a glance. */
static const uint32_t BLINK_IDLE_MS = 1000;

// Phase 1: fast-blink rate and active-cycle counter re-enable with ESP-NOW
// static const uint32_t BLINK_ACTIVE_MS    = 80;
// static const uint32_t ACTIVE_BLINK_CYCLES = 10;
// static volatile uint32_t s_active_cycles  = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * PIN_LED_ONBOARD (GPIO21) is active LOW on the XIAO ESP32-S3.
 * These helpers make the intent clear at every call site.
 */
static inline void onboard_led_on(void)  { gpio_set_level(PIN_LED_ONBOARD, 0); }
static inline void onboard_led_off(void) { gpio_set_level(PIN_LED_ONBOARD, 1); }

// ---------------------------------------------------------------------------
// ESP-NOW receive callback
// ---------------------------------------------------------------------------

/**
 * Called from the ESP-NOW WiFi task when any packet arrives.
 * Keep fast — no blocking, no LVGL, no malloc.
 * Triggers a brief fast-blink burst for visual feedback without a scope.
 */
// Phase 1: restore on_recv when espnow_transport is implemented
// static void on_recv(const uint8_t mac[6], const uint8_t *data, size_t len)
// {
//     if (!data || len < sizeof(espnow_hdr_t)) { return; }
//     const espnow_hdr_t *hdr = (const espnow_hdr_t *)data;
//     ESP_LOGI(TAG, "RX [%02x:%02x:%02x:%02x:%02x:%02x]  type=0x%02x  node=%u  seq=%u",
//              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
//              hdr->type, hdr->node_id, hdr->seq);
//     s_active_cycles = ACTIVE_BLINK_CYCLES;
// }

// ---------------------------------------------------------------------------
// Tasks
// ---------------------------------------------------------------------------

/**
 * Blinks the onboard user LED (GPIO21, active LOW).
 * Rate: BLINK_ACTIVE_MS for ACTIVE_BLINK_CYCLES after any radio event,
 * then relaxes back to BLINK_IDLE_MS.
 */
static void blink_task(void *arg)
{
    gpio_reset_pin(PIN_LED_ONBOARD);
    gpio_set_direction(PIN_LED_ONBOARD, GPIO_MODE_OUTPUT);
    onboard_led_off();  // start with LED off (HIGH = off on active-LOW pin)

    bool lit = false;

    while (1) {
        lit = !lit;
        if (lit) onboard_led_on(); else onboard_led_off();
        // Phase 1: replace fixed delay with active/idle rate driven by ESP-NOW events
        vTaskDelay(pdMS_TO_TICKS(BLINK_IDLE_MS));
    }
}

/**
 * Polls D3 (test) and D4 (mode) buttons with 20 ms software debounce.
 * Also reads D2 (ring detect) and logs TRS cable type on change — useful
 * for wiring verification without a multimeter.
 *
 * D3 single press: broadcasts an immediate ESPNOW_PKT_PING.
 * D3 5-second hold: re-pairing mode (Phase 2 — NVS MAC clear + reboot).
 * D4 press: placeholder for Phase 2 mode cycling.
 */
static void btn_poll_task(void *arg)
{
    gpio_reset_pin(PIN_BTN_TEST);
    gpio_set_direction(PIN_BTN_TEST, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN_TEST, GPIO_PULLUP_ONLY);

    gpio_reset_pin(PIN_BTN_MODE);
    gpio_set_direction(PIN_BTN_MODE, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN_MODE, GPIO_PULLUP_ONLY);

    gpio_reset_pin(PIN_RING_DETECT);
    gpio_set_direction(PIN_RING_DETECT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_RING_DETECT, GPIO_PULLUP_ONLY);

    // Track previous levels for falling-edge detection (PULLUP = HIGH when open)
    bool prev_test   = true;
    bool prev_mode   = true;
    bool prev_ring   = true;

    // static uint16_t btn_seq = 0;  // Phase 1: re-enable with espnow ping
    uint32_t test_held_ms   = 0;  // accumulates hold time for 5s re-pairing

    while (1) {
        bool cur_test  = gpio_get_level(PIN_BTN_TEST);
        bool cur_mode  = gpio_get_level(PIN_BTN_MODE);
        bool cur_ring  = gpio_get_level(PIN_RING_DETECT);

        // --- Test button ---
        if (!cur_test) {
            // Button is held — accumulate time
            test_held_ms += 20;
            if (test_held_ms == 5000) {
                // Phase 2: clear Core Node MAC from NVS and reboot
                ESP_LOGW(TAG, "Test button held 5s — re-pairing mode (Phase 2 TODO)");
            }
        } else {
            if (!prev_test) {
                // Rising edge: button released after a short press
                if (test_held_ms < 5000) {
                    // Phase 1: send ESP-NOW ping when espnow_transport is implemented
                    ESP_LOGI(TAG, "Test button pressed (Phase 1: ping TODO)");
                    // espnow_ping_pkt_t pkt = { .hdr = { .type = ESPNOW_PKT_PING,
                    //     .node_id = DRUM_NODE_ID, .seq = ++btn_seq } };
                    // esp_err_t err = espnow_transport_broadcast(&pkt, sizeof(pkt));
                    // if (err == ESP_OK) { s_active_cycles = ACTIVE_BLINK_CYCLES; }
                    // else { ESP_LOGW(TAG, "Ping failed: %s", esp_err_to_name(err)); }
                }
            }
            test_held_ms = 0;
        }

        // --- Mode button ---
        if (prev_mode && !cur_mode) {
            // Phase 2: cycle g_current_mode and apply to LED strip
            ESP_LOGI(TAG, "Mode button pressed (Phase 2: lighting mode cycle)");
        }

        // --- Ring detect — log cable type on change for wiring verification ---
        if (cur_ring != prev_ring) {
            ESP_LOGI(TAG, "TRS cable type: %s",
                     cur_ring ? "mono TS (ring shorted)" : "stereo TRS");
        }

        prev_test = cur_test;
        prev_mode = cur_mode;
        prev_ring = cur_ring;

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * Sends a periodic ESPNOW_PKT_PING heartbeat so the Core Node can detect
 * this drum node in its serial log before full PAIR_REQ/ACK is implemented.
 */
static void heartbeat_task(void *arg)
{
    static uint16_t hb_seq = 0;

    while (1) {
        ++hb_seq;
        // Unconditional alive-pulse visible on serial monitor regardless of radio state
        ESP_LOGI(TAG, "--- heartbeat #%u  node_id=%u ---", hb_seq, DRUM_NODE_ID);

        // Phase 1: broadcast heartbeat over ESP-NOW when espnow_transport is implemented
        // espnow_ping_pkt_t pkt = { .hdr = { .type = ESPNOW_PKT_PING,
        //     .node_id = DRUM_NODE_ID, .seq = hb_seq } };
        // esp_err_t err = espnow_transport_broadcast(&pkt, sizeof(pkt));
        // if (err == ESP_OK) { ESP_LOGI(TAG, "Heartbeat #%u broadcast OK", hb_seq); }
        // else { ESP_LOGW(TAG, "Heartbeat #%u broadcast failed: %s", hb_seq, esp_err_to_name(err)); }

        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS));
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  ADLS Drum Node  |  node_id=%u  |  booting...", DRUM_NODE_ID);
    ESP_LOGI(TAG, "=================================================");

    // Phase 1: ESP-NOW init — uncomment when espnow_transport is implemented
    // ESP_ERROR_CHECK(espnow_transport_init());
    // espnow_transport_set_recv_cb(on_recv);
    // ESP_LOGI(TAG, "ESP-NOW ready");

    xTaskCreate(blink_task,     "blink",     2048, NULL, 3, NULL);
    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL, 4, NULL);
    // Phase 1: re-enable btn_poll_task when ESP-NOW ping is available
    // xTaskCreate(btn_poll_task,  "btns",      2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "Tasks started — LED blinking at 1 Hz, heartbeat every 5s.");
}
