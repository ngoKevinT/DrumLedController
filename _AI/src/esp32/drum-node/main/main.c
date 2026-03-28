/*****************************************************************************
 * File        : main.c
 * Project     : ADLS Drum Node — Hello World / Board Bring-Up
 * Target      : Seeed Studio XIAO ESP32-S3
 * Description : Minimal firmware to verify board functionality before the
 *               LED strip and piezo sensor are wired up.
 *
 *   What this verifies:
 *     1. Onboard LED blinks at 1 Hz → firmware boots, FreeRTOS is running
 *     2. Test button (D4) → immediate ESP-NOW ping sent; LED flashes fast
 *     3. Periodic heartbeat → Core Node sees this node in its serial log
 *     4. Receive path → LED speeds up when any ESP-NOW packet arrives
 *
 *   Upgrade path:
 *     Phase 2 — add piezo ADC task, LED strip RMT driver, PAIR_REQ/ACK
 *     Phase 3 — wire cb_save() config packets → apply lighting mode
 *****************************************************************************/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "espnow_transport.h"
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

/** LED blink period when idle — slow enough to read at a glance. */
static const uint32_t BLINK_IDLE_MS = 1000;

/** LED blink period after any radio activity — visually distinct from idle. */
static const uint32_t BLINK_ACTIVE_MS = 80;

/** How many blink cycles to stay in active rate before relaxing to idle. */
static const uint32_t ACTIVE_BLINK_CYCLES = 10;

// ---------------------------------------------------------------------------
// Shared state between tasks (updated from recv callback)
// ---------------------------------------------------------------------------

/** Remaining fast-blink cycles before returning to idle rate.
 *  Written by recv callback; read by blink task. */
static volatile uint32_t s_active_cycles = 0;

// ---------------------------------------------------------------------------
// ESP-NOW receive callback
// ---------------------------------------------------------------------------

/**
 * Called from the ESP-NOW WiFi task when any packet arrives.
 * Keep this fast — no blocking, no LVGL, no malloc.
 * Triggers a brief fast-blink burst to give visual feedback.
 */
static void on_recv(const uint8_t mac[6], const uint8_t *data, size_t len)
{
    if (!data || len < sizeof(espnow_hdr_t)) {
        return;
    }
    const espnow_hdr_t *hdr = (const espnow_hdr_t *)data;
    ESP_LOGI(TAG, "RX [%02x:%02x:%02x:%02x:%02x:%02x]  type=0x%02x  node=%u  seq=%u",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             hdr->type, hdr->node_id, hdr->seq);

    // Signal the blink task that activity just happened
    s_active_cycles = ACTIVE_BLINK_CYCLES;
}

// ---------------------------------------------------------------------------
// Tasks
// ---------------------------------------------------------------------------

/**
 * Blinks PIN_LED_ONBOARD.
 * Rate: BLINK_ACTIVE_MS for ACTIVE_BLINK_CYCLES after any radio event,
 * then relaxes back to BLINK_IDLE_MS.
 * This gives a clear "I received something" flash without needing a scope.
 */
static void blink_task(void *arg)
{
    gpio_reset_pin(PIN_LED_ONBOARD);
    gpio_set_direction(PIN_LED_ONBOARD, GPIO_MODE_OUTPUT);
    bool state = false;

    while (1) {
        state = !state;
        gpio_set_level(PIN_LED_ONBOARD, state ? 1 : 0);

        uint32_t period;
        if (s_active_cycles > 0) {
            s_active_cycles--;
            period = BLINK_ACTIVE_MS;
        } else {
            period = BLINK_IDLE_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(period));
    }
}

/**
 * Polls the test and mode buttons with 20 ms software debounce.
 *
 * D4 press: broadcasts an immediate ESPNOW_PKT_PING and triggers the
 *           fast-blink visual so you can confirm the send without serial.
 * D5 press: placeholder — Phase 2 will cycle through lighting modes.
 */
static void btn_poll_task(void *arg)
{
    gpio_reset_pin(PIN_BTN_TEST);
    gpio_set_direction(PIN_BTN_TEST, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN_TEST, GPIO_PULLUP_ONLY);

    gpio_reset_pin(PIN_BTN_MODE);
    gpio_set_direction(PIN_BTN_MODE, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN_MODE, GPIO_PULLUP_ONLY);

    // Track previous level for falling-edge detection (PULLUP = HIGH when open)
    bool prev_test = true;
    bool prev_mode = true;

    static uint16_t btn_seq = 0;

    while (1) {
        bool cur_test = gpio_get_level(PIN_BTN_TEST);
        bool cur_mode = gpio_get_level(PIN_BTN_MODE);

        // Falling edge → button just pressed
        if (prev_test && !cur_test) {
            ESP_LOGI(TAG, "Test button pressed — sending ping");
            espnow_ping_pkt_t pkt = {
                .hdr = {
                    .type    = ESPNOW_PKT_PING,
                    .node_id = DRUM_NODE_ID,
                    .seq     = ++btn_seq,
                },
            };
            esp_err_t err = espnow_transport_broadcast(&pkt, sizeof(pkt));
            if (err == ESP_OK) {
                s_active_cycles = ACTIVE_BLINK_CYCLES;  // visual confirmation
            } else {
                ESP_LOGW(TAG, "Button ping failed: %s", esp_err_to_name(err));
            }
        }

        if (prev_mode && !cur_mode) {
            // Phase 2: cycle g_current_mode and apply to LED strip
            ESP_LOGI(TAG, "Mode button pressed (Phase 2: lighting mode cycle)");
        }

        prev_test = cur_test;
        prev_mode = cur_mode;

        // 20 ms poll gives clean debounce without needing a hardware RC filter
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * Sends a periodic ESPNOW_PKT_PING so the Core Node sees this drum node
 * in its serial log before full PAIR_REQ / PAIR_ACK is implemented.
 * Priority 4 — higher than UI-bound tasks, lower than a future ISR trigger.
 */
static void heartbeat_task(void *arg)
{
    static uint16_t hb_seq = 0;

    while (1) {
        espnow_ping_pkt_t pkt = {
            .hdr = {
                .type    = ESPNOW_PKT_PING,
                .node_id = DRUM_NODE_ID,
                .seq     = ++hb_seq,
            },
        };
        esp_err_t err = espnow_transport_broadcast(&pkt, sizeof(pkt));
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Heartbeat #%u sent", hb_seq);
        } else {
            ESP_LOGW(TAG, "Heartbeat #%u failed: %s", hb_seq, esp_err_to_name(err));
        }

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

    // Init transport before any task tries to send
    ESP_ERROR_CHECK(espnow_transport_init());
    espnow_transport_set_recv_cb(on_recv);

    ESP_LOGI(TAG, "ESP-NOW ready");

    // blink_task     — LED heartbeat, visual activity indicator
    // heartbeat_task — periodic broadcast so Core Node detects this node
    // btn_poll_task  — D4 manual ping, D5 mode cycle placeholder
    xTaskCreate(blink_task,     "blink",     2048, NULL, 3, NULL);
    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL, 4, NULL);
    xTaskCreate(btn_poll_task,  "btns",      2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "All tasks started.");
    ESP_LOGI(TAG, "  Onboard LED blinks 1 Hz at idle, fast on radio activity.");
    ESP_LOGI(TAG, "  Press D4 to send a manual ping to the Core Node.");
}
