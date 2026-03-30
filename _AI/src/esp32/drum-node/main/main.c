/*****************************************************************************
 * File        : main.c
 * Project     : ADLS Drum Node — Phase 2 ESP-NOW Connection
 * Target      : Seeed Studio XIAO ESP32-S3
 * Description : Boots, initialises ESP-NOW, then attempts to pair with the
 *               Core Node for 30 seconds.  Falls back to standalone mode if
 *               no ACK is received.
 *
 *   Connection state machine:
 *     SEARCHING  — broadcasting PAIR_REQ every 1 s for up to 30 s
 *                  Blue LED: 500 ms blink
 *     PAIRED     — Core Node MAC saved to NVS; unicast peer registered
 *                  Blue LED: solid ON
 *     STANDALONE — 30 s elapsed, no response; node still works locally
 *                  Blue LED: slow 2 s blink
 *
 *   Node naming:
 *     Default name "Drum 1" is saved to NVS on first boot.
 *     Core Node renames the drum by sending ESPNOW_PKT_CONFIG with a
 *     non-empty name field.  The new name is saved to NVS immediately.
 *
 *   Upgrade path:
 *     Phase 3 — add ADC trigger task (A0 ch1, D1 ch2), LED strip RMT on D9
 *****************************************************************************/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "espnow_transport.h"
#include "pinout.h"

static const char *TAG = "drum_node";

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/** NVS namespace and keys for persistent drum node config. */
#define NVS_NS            "drum_cfg"
#define NVS_KEY_NAME      "node_name"
#define NVS_KEY_CORE_MAC  "core_mac"
#define NVS_KEY_CONFIG    "config"

/** Prefix used when generating a name from the chip MAC on first boot. */
#define NODE_NAME_PREFIX "Drum"

/** Maximum length of the node name string (including null terminator). */
#define NODE_NAME_MAX_LEN 16

/** Duration of the ESP-NOW search window before falling back to standalone. */
static const uint32_t CONNECT_TIMEOUT_S = 30;

/** Interval between PAIR_REQ broadcasts during the search window. */
static const uint32_t PAIR_REQ_INTERVAL_MS = 1000;

/** Onboard LED blink period while idle (1 Hz "alive" pulse). */
static const uint32_t BLINK_IDLE_MS = 1000;

/** Blue LED blink period while searching for Core Node. */
static const uint32_t BLINK_SEARCHING_MS = 500;

/** Blue LED blink period in standalone mode (slow, unobtrusive). */
static const uint32_t BLINK_STANDALONE_MS = 2000;

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------

typedef enum {
    CONN_STATE_SEARCHING,   /**< Broadcasting PAIR_REQ, waiting for ACK.   */
    CONN_STATE_PAIRED,      /**< Paired with Core Node — normal operation.  */
    CONN_STATE_STANDALONE,  /**< No Core Node found; operating autonomously. */
} conn_state_t;

/** Connection state — written by recv callback, read by connect_task and LEDs. */
static volatile conn_state_t s_conn_state = CONN_STATE_SEARCHING;

/** MAC of the Core Node — populated when PAIR_ACK is received. */
static uint8_t s_core_mac[6] = {0};

/** Human-readable node name — generated from chip MAC on first boot, then NVS-persisted. */
static char s_node_name[NODE_NAME_MAX_LEN] = "";

/** Node ID broadcast in every outbound packet header. */
static const uint8_t DRUM_NODE_ID = 1;

// ---------------------------------------------------------------------------
// Live config — mirrors what the Core Node last pushed via ESPNOW_PKT_CONFIG.
// Defaults match the UI's initial placeholder values so standalone mode
// produces sensible LED behaviour out of the box.
// Phase 3 wires these into the ADC trigger task and LED engine.
// ---------------------------------------------------------------------------

typedef struct {
    uint8_t  mode_idx;          /**< Lighting mode index (0-6)             */
    uint32_t color;             /**< Base LED color, 0x00RRGGBB            */
    uint8_t  idle_brightness;   /**< Ambient glow level (0-100 %)          */
    uint8_t  flash_brightness;  /**< Peak flash level   (0-100 %)          */
    uint16_t flash_time_ms;     /**< Static flash duration (10-2000 ms)    */
    uint16_t decay_time_ms;     /**< Fade decay time (50-3000 ms)          */
    uint16_t threshold;         /**< Trigger sensitivity (0-1023)          */
    uint16_t retrigger_ms;      /**< Retrigger guard window (0-500 ms)     */
    uint8_t  hit_curve;         /**< Velocity-to-brightness mapping        */
} DrumConfig;

static DrumConfig s_config = {
    .mode_idx         = 0,       // SOLID
    .color            = 0xFFFFFF, // White (RGB channels — W channel handled by LED engine)
    .idle_brightness  = 0,        // Off at idle until Core Node configures otherwise
    .flash_brightness = 100,
    .flash_time_ms    = 120,
    .decay_time_ms    = 250,
    .threshold        = 200,
    .retrigger_ms     = 25,
    .hit_curve        = 0,       // LINEAR
};

// ---------------------------------------------------------------------------
// NVS helpers
// ---------------------------------------------------------------------------

/**
 * Load the node name from NVS into @p out.
 * Returns ESP_ERR_NVS_NOT_FOUND if no name has been saved yet (first boot).
 * The caller is responsible for generating and persisting a default in that case.
 */
static esp_err_t nvs_load_name(char *out, size_t len)
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NVS_NOT_FOUND;  // namespace absent — first boot
    }
    ESP_ERROR_CHECK(ret);

    size_t sz = len;
    ret = nvs_get_str(h, NVS_KEY_NAME, out, &sz);
    nvs_close(h);
    return ret;  // caller handles ESP_ERR_NVS_NOT_FOUND if key also absent
}

/** Save the node name to NVS. */
static esp_err_t nvs_save_name(const char *name)
{
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &h));
    esp_err_t ret = nvs_set_str(h, NVS_KEY_NAME, name);
    if (ret == ESP_OK) { ret = nvs_commit(h); }
    nvs_close(h);
    return ret;
}

/**
 * Load the Core Node MAC from NVS.
 * Returns ESP_ERR_NVS_NOT_FOUND if no MAC has been saved yet
 * (i.e. this node has never been paired).
 */
static esp_err_t nvs_load_core_mac(uint8_t mac_out[6])
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (ret == ESP_ERR_NVS_NOT_FOUND) { return ESP_ERR_NVS_NOT_FOUND; }
    ESP_ERROR_CHECK(ret);

    size_t sz = 6;
    ret = nvs_get_blob(h, NVS_KEY_CORE_MAC, mac_out, &sz);
    nvs_close(h);
    return ret;
}

/** Save the Core Node MAC to NVS so pairing survives a reboot. */
static esp_err_t nvs_save_core_mac(const uint8_t mac[6])
{
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &h));
    esp_err_t ret = nvs_set_blob(h, NVS_KEY_CORE_MAC, mac, 6);
    if (ret == ESP_OK) { ret = nvs_commit(h); }
    nvs_close(h);
    return ret;
}

/**
 * Persist the current s_config blob to NVS.
 * Called from on_recv (WiFi task) only on CONFIG packets — infrequent
 * user-initiated events — so the occasional flash latency is acceptable.
 */
static esp_err_t nvs_save_config(void)
{
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &h));
    esp_err_t ret = nvs_set_blob(h, NVS_KEY_CONFIG, &s_config, sizeof(s_config));
    if (ret == ESP_OK) { ret = nvs_commit(h); }
    nvs_close(h);
    return ret;
}

/**
 * Load s_config from NVS.  If absent (first boot) the struct retains its
 * compile-time defaults — no error is returned.
 */
static void nvs_load_config(void)
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (ret == ESP_ERR_NVS_NOT_FOUND) { return; }
    if (ret != ESP_OK) { return; }

    size_t sz = sizeof(s_config);
    ret = nvs_get_blob(h, NVS_KEY_CONFIG, &s_config, &sz);
    nvs_close(h);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Config loaded from NVS: mode=%u  threshold=%u  retrigger=%ums",
                 s_config.mode_idx, s_config.threshold, s_config.retrigger_ms);
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * PIN_LED_ONBOARD (GPIO21) is active LOW on the XIAO ESP32-S3.
 */
static inline void onboard_led_on(void)  { gpio_set_level(PIN_LED_ONBOARD, 0); }
static inline void onboard_led_off(void) { gpio_set_level(PIN_LED_ONBOARD, 1); }

// ---------------------------------------------------------------------------
// ESP-NOW receive callback
// ---------------------------------------------------------------------------

/**
 * Called from the ESP-NOW WiFi task on every received frame.
 * Must be fast — no blocking, no NVS, no malloc.
 *
 * Handles:
 *   PAIR_ACK  — captures Core Node MAC, transitions to PAIRED state.
 *               Name override applied if ACK carries a non-empty name.
 *   CONFIG    — applies lighting settings (Phase 3); saves rename if present.
 */
static void on_recv(const uint8_t mac[6], const uint8_t *data, size_t len)
{
    if (!data || len < sizeof(espnow_hdr_t)) { return; }
    const espnow_hdr_t *hdr = (const espnow_hdr_t *)data;

    ESP_LOGI(TAG, "RX [%02x:%02x:%02x:%02x:%02x:%02x]  type=0x%02x  seq=%u",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             hdr->type, hdr->seq);

    if (hdr->type == ESPNOW_PKT_PAIR_ACK) {
        if (len < sizeof(espnow_pair_ack_pkt_t)) { return; }

        // Only accept the first ACK — ignore if we're already paired
        if (s_conn_state != CONN_STATE_SEARCHING) { return; }

        memcpy(s_core_mac, mac, 6);

        // Apply Core Node's name suggestion if provided
        const espnow_pair_ack_pkt_t *ack = (const espnow_pair_ack_pkt_t *)data;
        if (ack->name[0] != '\0') {
            strncpy(s_node_name, ack->name, NODE_NAME_MAX_LEN - 1);
            s_node_name[NODE_NAME_MAX_LEN - 1] = '\0';
            // NVS write deferred to connect_task (not safe here in WiFi task)
        }

        // Signal connect_task — write must be atomic-enough for a state flag
        s_conn_state = CONN_STATE_PAIRED;

    } else if (hdr->type == ESPNOW_PKT_CONFIG) {
        if (len < sizeof(espnow_config_pkt_t)) { return; }
        const espnow_config_pkt_t *cfg = (const espnow_config_pkt_t *)data;

        // Apply all config fields to live state
        s_config.mode_idx         = cfg->mode_idx;
        s_config.color            = cfg->color;
        s_config.idle_brightness  = cfg->idle_brightness;
        s_config.flash_brightness = cfg->flash_brightness;
        s_config.flash_time_ms    = cfg->flash_time_ms;
        s_config.decay_time_ms    = cfg->decay_time_ms;
        s_config.threshold        = cfg->threshold;
        s_config.retrigger_ms     = cfg->retrigger_ms;
        s_config.hit_curve        = cfg->hit_curve;

        // Handle rename — NVS write is acceptable here: CONFIG packets arrive
        // only on user-initiated Save, not on every hit, so flash latency is fine.
        bool name_changed = false;
        if (cfg->name[0] != '\0' &&
            strncmp(cfg->name, s_node_name, NODE_NAME_MAX_LEN) != 0) {
            strncpy(s_node_name, cfg->name, NODE_NAME_MAX_LEN - 1);
            s_node_name[NODE_NAME_MAX_LEN - 1] = '\0';
            name_changed = true;
        }

        // Persist the full config blob (includes updated fields from Core Node)
        esp_err_t save_ret = nvs_save_config();
        if (name_changed) {
            esp_err_t name_ret = nvs_save_name(s_node_name);
            ESP_LOGI(TAG, "Renamed to \"%s\"  NVS: %s",
                     s_node_name, esp_err_to_name(name_ret));
        }

        ESP_LOGI(TAG, "CONFIG applied: mode=%u  color=0x%06lx  thresh=%u  "
                      "retrig=%ums  idle=%u%%  flash=%u%%  NVS: %s",
                 s_config.mode_idx, (unsigned long)s_config.color,
                 s_config.threshold, s_config.retrigger_ms,
                 s_config.idle_brightness, s_config.flash_brightness,
                 esp_err_to_name(save_ret));

        // Phase 3: notify LED engine task of updated config via FreeRTOS queue
    }
}

// ---------------------------------------------------------------------------
// Tasks
// ---------------------------------------------------------------------------

/**
 * Blinks the onboard user LED (GPIO21, active LOW) at 1 Hz.
 * Confirms the logic rail is live at a glance — always running.
 */
static void blink_task(void *arg)
{
    gpio_reset_pin(PIN_LED_ONBOARD);
    gpio_set_direction(PIN_LED_ONBOARD, GPIO_MODE_OUTPUT);
    onboard_led_off();

    bool lit = false;
    while (1) {
        lit = !lit;
        if (lit) onboard_led_on(); else onboard_led_off();
        vTaskDelay(pdMS_TO_TICKS(BLINK_IDLE_MS));
    }
}

/**
 * Drives the blue status LED (D5/GPIO6) to reflect the connection state:
 *   SEARCHING   — 500 ms blink
 *   PAIRED      — solid ON
 *   STANDALONE  — 2 s slow blink
 */
static void blue_led_task(void *arg)
{
    gpio_reset_pin(PIN_LED_BLUE);
    gpio_set_direction(PIN_LED_BLUE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED_BLUE, 0);  // off at start

    bool lit = false;
    while (1) {
        switch (s_conn_state) {
            case CONN_STATE_PAIRED:
                gpio_set_level(PIN_LED_BLUE, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case CONN_STATE_SEARCHING:
                lit = !lit;
                gpio_set_level(PIN_LED_BLUE, lit ? 1 : 0);
                vTaskDelay(pdMS_TO_TICKS(BLINK_SEARCHING_MS));
                break;

            case CONN_STATE_STANDALONE:
                lit = !lit;
                gpio_set_level(PIN_LED_BLUE, lit ? 1 : 0);
                vTaskDelay(pdMS_TO_TICKS(BLINK_STANDALONE_MS));
                break;
        }
    }
}

/**
 * Sends a periodic heartbeat every 5 s so the Core Node can see this
 * node's serial log output and confirm it is alive.
 */
static void heartbeat_task(void *arg)
{
    static uint16_t hb_seq = 0;
    while (1) {
        ++hb_seq;
        const char *state_str =
            (s_conn_state == CONN_STATE_PAIRED)     ? "PAIRED" :
            (s_conn_state == CONN_STATE_STANDALONE)  ? "STANDALONE" : "SEARCHING";
        ESP_LOGI(TAG, "--- heartbeat #%u  node=\"%s\"  state=%s ---",
                 hb_seq, s_node_name, state_str);

        // Phase 3: send ESPNOW_PKT_TELEMETRY to Core Node if paired
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * Polls the test (A2/D2) and mode (D3) buttons with 20 ms software debounce.
 *
 * A2/D2 short press : placeholder (Phase 3 — manual trigger test)
 * A2/D2 5 s hold    : clears Core Node MAC from NVS and reboots to re-pair
 * D3 press          : placeholder (Phase 3 — lighting mode cycle)
 *
 * Ring detect is handled in hardware: 100kΩ pull-down on ch2 keeps the ring
 * line low when no TRS plug is present.  No dedicated detect pin required.
 */
static void btn_poll_task(void *arg)
{
    gpio_reset_pin(PIN_BTN_TEST);
    gpio_set_direction(PIN_BTN_TEST, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN_TEST, GPIO_PULLUP_ONLY);

    gpio_reset_pin(PIN_BTN_MODE);
    gpio_set_direction(PIN_BTN_MODE, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN_MODE, GPIO_PULLUP_ONLY);

    bool prev_test = true;
    bool prev_mode = true;
    uint32_t test_held_ms = 0;

    while (1) {
        bool cur_test = gpio_get_level(PIN_BTN_TEST);
        bool cur_mode = gpio_get_level(PIN_BTN_MODE);

        // --- Test button ---
        if (!cur_test) {
            test_held_ms += 20;
            if (test_held_ms == 5000) {
                // 5-second hold: wipe Core Node MAC so next boot triggers pairing
                ESP_LOGW(TAG, "Test button held 5s — clearing Core MAC from NVS, rebooting to re-pair");
                nvs_handle_t h;
                if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
                    nvs_erase_key(h, NVS_KEY_CORE_MAC);
                    nvs_commit(h);
                    nvs_close(h);
                }
                esp_restart();
            }
        } else {
            if (!prev_test && test_held_ms < 5000) {
                // Short press — Phase 3: fire manual trigger test
                ESP_LOGI(TAG, "Test button pressed (Phase 3: manual trigger test)");
            }
            test_held_ms = 0;
        }

        // --- Mode button ---
        if (prev_mode && !cur_mode) {
            ESP_LOGI(TAG, "Mode button pressed (Phase 3: lighting mode cycle)");
        }

        prev_test = cur_test;
        prev_mode = cur_mode;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * Runs once after boot.  Attempts to pair with the Core Node for
 * CONNECT_TIMEOUT_S seconds by broadcasting PAIR_REQ every second.
 *
 * Exits with one of three outcomes:
 *   1. PAIRED     — PAIR_ACK received; Core MAC saved to NVS.
 *   2. RESTORED   — Core MAC was already in NVS from a previous session.
 *   3. STANDALONE — Timeout elapsed; node continues without Core Node.
 */
static void connect_task(void *arg)
{
    // Load persisted name — generate from chip MAC on first boot.
    // WiFi is already started by espnow_transport_init(), so the STA MAC is ready.
    esp_err_t name_ret = nvs_load_name(s_node_name, sizeof(s_node_name));
    if (name_ret == ESP_ERR_NVS_NOT_FOUND) {
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        // Use last 3 bytes as hex suffix: "Drum_A1B2C3"
        snprintf(s_node_name, sizeof(s_node_name),
                 "%s_%02X%02X%02X", NODE_NAME_PREFIX, mac[3], mac[4], mac[5]);
        nvs_save_name(s_node_name);
        ESP_LOGI(TAG, "First boot — generated node name: \"%s\"", s_node_name);
    }

    // Load persisted config (compile-time defaults used on first boot)
    nvs_load_config();
    ESP_LOGI(TAG, "Node name: \"%s\"  mode=%u  threshold=%u",
             s_node_name, s_config.mode_idx, s_config.threshold);

    // Check if we are already paired from a previous boot
    uint8_t stored_mac[6];
    if (nvs_load_core_mac(stored_mac) == ESP_OK) {
        memcpy(s_core_mac, stored_mac, 6);
        espnow_transport_add_peer(s_core_mac);
        s_conn_state = CONN_STATE_PAIRED;
        ESP_LOGI(TAG, "=== Pairing restored — Core Node [%02x:%02x:%02x:%02x:%02x:%02x] ===",
                 s_core_mac[0], s_core_mac[1], s_core_mac[2],
                 s_core_mac[3], s_core_mac[4], s_core_mac[5]);
        vTaskDelete(NULL);
        return;
    }

    // No stored MAC — start the 30-second search window
    ESP_LOGI(TAG, "=== ESP-NOW: no saved pairing — searching for Core Node (%lus window) ===",
             (unsigned long)CONNECT_TIMEOUT_S);

    espnow_pair_req_pkt_t req = {};
    req.hdr.type    = ESPNOW_PKT_PAIR_REQ;
    req.hdr.node_id = DRUM_NODE_ID;
    strncpy(req.name, s_node_name, sizeof(req.name) - 1);

    for (uint32_t t = 0; t < CONNECT_TIMEOUT_S; t++) {
        if (s_conn_state != CONN_STATE_SEARCHING) {
            break;  // PAIR_ACK arrived — exit early
        }

        req.hdr.seq++;
        esp_err_t err = espnow_transport_broadcast(&req, sizeof(req));

        ESP_LOGI(TAG, "PAIR_REQ #%u  name=\"%s\"  %lus / %lus  [%s]",
                 req.hdr.seq, req.name,
                 (unsigned long)(t + 1), (unsigned long)CONNECT_TIMEOUT_S,
                 err == ESP_OK ? "sent" : esp_err_to_name(err));

        vTaskDelay(pdMS_TO_TICKS(PAIR_REQ_INTERVAL_MS));
    }

    if (s_conn_state == CONN_STATE_PAIRED) {
        // Save Core MAC and name (name may have been updated by ACK packet)
        ESP_ERROR_CHECK(nvs_save_core_mac(s_core_mac));
        ESP_ERROR_CHECK(nvs_save_name(s_node_name));
        espnow_transport_add_peer(s_core_mac);
        ESP_LOGI(TAG, "=== PAIRED — Core Node [%02x:%02x:%02x:%02x:%02x:%02x]  name=\"%s\" ===",
                 s_core_mac[0], s_core_mac[1], s_core_mac[2],
                 s_core_mac[3], s_core_mac[4], s_core_mac[5],
                 s_node_name);
    } else {
        s_conn_state = CONN_STATE_STANDALONE;
        ESP_LOGW(TAG, "=== STANDALONE — no Core Node found after %lus; running autonomously ===",
                 (unsigned long)CONNECT_TIMEOUT_S);
    }

    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  ADLS Drum Node  |  node_id=%u  |  booting...", DRUM_NODE_ID);
    ESP_LOGI(TAG, "=================================================");

    // NVS is initialised inside espnow_transport_init — do not call nvs_flash_init here
    ESP_ERROR_CHECK(espnow_transport_init());
    espnow_transport_set_recv_cb(on_recv);
    ESP_LOGI(TAG, "ESP-NOW ready");

    // Launch tasks — connect_task runs the 30-second pairing window then exits
    xTaskCreate(blink_task,    "blink",    2048, NULL, 3, NULL);
    xTaskCreate(blue_led_task, "blue_led", 2048, NULL, 3, NULL);
    xTaskCreate(heartbeat_task,"heartbeat",2048, NULL, 4, NULL);
    xTaskCreate(btn_poll_task, "btns",     2048, NULL, 3, NULL);
    xTaskCreate(connect_task,  "connect",  4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Tasks started — searching for Core Node...");
}
