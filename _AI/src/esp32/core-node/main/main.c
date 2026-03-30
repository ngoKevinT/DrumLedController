/*****************************************************************************
 * File        : main.c
 * Project     : ADLS Core Node — Phase 2 ESP-NOW Pairing
 * Description : Hardware init entry point + drum node registry.
 *
 *   Phase 2 additions:
 *     - Drum node registry: up to MAX_DRUM_NODES entries (MAC + name) in NVS.
 *     - Passive pairing: Core Node listens for PAIR_REQ broadcasts, replies
 *       with PAIR_ACK, and marks the matching g_drums[] slot as connected.
 *     - core_pairing_task: decouples NVS writes and LVGL g_drums[] updates
 *       from the WiFi receive task.  Avoids calling LVGL API from the wrong task.
 *
 *   Thread safety:
 *     on_drum_recv (WiFi task) → s_pair_queue → core_pairing_task (own task)
 *     core_pairing_task acquires lvgl_port_lock before touching g_drums[].
 *     ui_post_status() is queue-backed — safe from any task.
 *
 *   Upgrade path:
 *     Phase 3 — wire TELEMETRY packets → ui_main_drum_flash() / live readings
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_spiffs.h"
#include "rgb_lcd_port.h"     // waveshare_esp32_s3_rgb_lcd_init(), wavesahre_rgb_lcd_bl_on()
#include "gt911.h"            // touch_gt911_init()
#include "lvgl_port.h"        // lvgl_port_init(), lvgl_port_lock(), lvgl_port_unlock()
#include "ui.h"               // ui_create(), ui_post_status(), g_drums[], UI_MAX_DRUMS
#include "espnow_transport.h" // espnow_transport_init(), espnow_transport_send(), etc.

static const char *TAG = "main";

// ---------------------------------------------------------------------------
// Drum node registry
// ---------------------------------------------------------------------------

/**
 * Maximum number of drum nodes this Core Node will track.
 * Must be <= UI_MAX_DRUMS (currently 6).  The indices are shared: slot 0
 * in s_drum_nodes[] corresponds to g_drums[0] on the UI.
 */
#define MAX_DRUM_NODES  UI_MAX_DRUMS

/** NVS namespace for Core Node persistent config. */
#define NVS_CORE_NS     "core_cfg"

/** Persisted record for one drum node. */
typedef struct {
    uint8_t mac[6];
    char    name[16];
    bool    valid;           /**< false = empty slot                       */
} DrumNodeEntry;

static DrumNodeEntry s_drum_nodes[MAX_DRUM_NODES];

// ---------------------------------------------------------------------------
// Pairing event queue
// ---------------------------------------------------------------------------

/**
 * Payload pushed onto s_pair_queue by on_drum_recv (WiFi task).
 * core_pairing_task processes these outside the WiFi context.
 */
typedef struct {
    uint8_t mac[6];
    char    name[16];  /**< Name from the PAIR_REQ packet                  */
} PairEvent;

/** Queue depth of 4 is plenty — pairing events arrive at most once per second. */
#define PAIR_QUEUE_DEPTH 4
static QueueHandle_t s_pair_queue;

// ---------------------------------------------------------------------------
// NVS helpers
// ---------------------------------------------------------------------------

/** Builds per-slot NVS keys like "mac_0", "name_3". */
static void slot_key(char *out, size_t len, const char *prefix, int idx)
{
    snprintf(out, len, "%s_%d", prefix, idx);
}

/**
 * Load all drum node entries from NVS into s_drum_nodes[].
 * Safe to call after espnow_transport_init() has initialised NVS.
 */
static void core_nvs_load_nodes(void)
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_CORE_NS, NVS_READONLY, &h);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No drum node registry in NVS — first boot");
        return;
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
        return;
    }

    for (int i = 0; i < MAX_DRUM_NODES; i++) {
        char mac_key[10], name_key[12];
        slot_key(mac_key,  sizeof(mac_key),  "mac",  i);
        slot_key(name_key, sizeof(name_key), "name", i);

        uint8_t mac[6];
        size_t mac_sz = sizeof(mac);
        if (nvs_get_blob(h, mac_key, mac, &mac_sz) != ESP_OK) {
            continue;  // slot is empty
        }

        char name[16] = {0};
        size_t name_sz = sizeof(name);
        nvs_get_str(h, name_key, name, &name_sz);  // ignore error — name may be absent

        memcpy(s_drum_nodes[i].mac,  mac,  6);
        strncpy(s_drum_nodes[i].name, name[0] ? name : "Drum ?", sizeof(s_drum_nodes[i].name) - 1);
        s_drum_nodes[i].valid = true;

        ESP_LOGI(TAG, "Loaded slot %d: \"%s\"  [%02x:%02x:%02x:%02x:%02x:%02x]",
                 i, s_drum_nodes[i].name,
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    nvs_close(h);
}

/** Persist a single drum node slot to NVS. */
static esp_err_t core_nvs_save_node(int idx)
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_CORE_NS, NVS_READWRITE, &h);
    if (ret != ESP_OK) { return ret; }

    char mac_key[10], name_key[12];
    slot_key(mac_key,  sizeof(mac_key),  "mac",  idx);
    slot_key(name_key, sizeof(name_key), "name", idx);

    ret = nvs_set_blob(h, mac_key,  s_drum_nodes[idx].mac,  6);
    if (ret == ESP_OK) {
        ret = nvs_set_str(h, name_key, s_drum_nodes[idx].name);
    }
    if (ret == ESP_OK) {
        ret = nvs_commit(h);
    }
    nvs_close(h);
    return ret;
}

// ---------------------------------------------------------------------------
// Registry lookups
// ---------------------------------------------------------------------------

/** Returns the slot index whose MAC matches, or -1 if not found. */
static int find_node_by_mac(const uint8_t mac[6])
{
    for (int i = 0; i < MAX_DRUM_NODES; i++) {
        if (s_drum_nodes[i].valid && memcmp(s_drum_nodes[i].mac, mac, 6) == 0) {
            return i;
        }
    }
    return -1;
}

/** Returns the first empty slot index, or -1 if the table is full. */
static int find_empty_slot(void)
{
    for (int i = 0; i < MAX_DRUM_NODES; i++) {
        if (!s_drum_nodes[i].valid) { return i; }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// ESP-NOW receive callback
// ---------------------------------------------------------------------------

/**
 * Called from the ESP-NOW / WiFi task — must be fast and non-blocking.
 *
 * Handles:
 *   PAIR_REQ  — enqueues a PairEvent for core_pairing_task to process.
 *   TELEMETRY — Phase 3: route to UI drum-flash.  Placeholder log for now.
 *   CONFIG    — not expected inbound on Core Node.  Logged and dropped.
 */
static void on_drum_recv(const uint8_t mac[6], const uint8_t *data, size_t len)
{
    if (!data || len < sizeof(espnow_hdr_t)) { return; }
    const espnow_hdr_t *hdr = (const espnow_hdr_t *)data;

    ESP_LOGI(TAG, "RX node %u [%02x:%02x:%02x:%02x:%02x:%02x]  type=0x%02x  seq=%u",
             hdr->node_id,
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             hdr->type, hdr->seq);

    if (hdr->type == ESPNOW_PKT_PAIR_REQ) {
        if (len < sizeof(espnow_pair_req_pkt_t)) { return; }
        const espnow_pair_req_pkt_t *req = (const espnow_pair_req_pkt_t *)data;

        PairEvent ev;
        memcpy(ev.mac, mac, 6);
        strncpy(ev.name, req->name[0] ? req->name : "Drum ?", sizeof(ev.name) - 1);
        ev.name[sizeof(ev.name) - 1] = '\0';

        // Non-blocking send — if the queue is full the PAIR_REQ will simply be
        // retried by the drum node on its next 1-second interval.
        if (xQueueSendFromISR(s_pair_queue, &ev, NULL) == errQUEUE_FULL) {
            ESP_LOGW(TAG, "Pair queue full — PAIR_REQ from [%02x:%02x:...] dropped",
                     mac[0], mac[1]);
        }

    } else if (hdr->type == ESPNOW_PKT_TELEMETRY) {
        // Phase 3: call ui_main_drum_flash(slot_idx, velocity)
        char msg[48];
        snprintf(msg, sizeof(msg), "Hit: node %u  seq=%u", hdr->node_id, hdr->seq);
        ui_post_status(msg);

    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "Node %u seen (type=0x%02x  seq=%u)",
                 hdr->node_id, hdr->type, hdr->seq);
        ui_post_status(msg);
    }
}

// ---------------------------------------------------------------------------
// Pairing task
// ---------------------------------------------------------------------------

/**
 * Blocks on s_pair_queue and processes each PAIR_REQ event.
 *
 * For each event:
 *   1. Look up the MAC in s_drum_nodes[].
 *   2. If new, claim an empty slot, persist to NVS, register ESP-NOW peer.
 *   3. Send PAIR_ACK back to the drum node.
 *   4. Acquire the LVGL lock and update g_drums[slot].connected / .name.
 *   5. Post a human-readable status message via ui_post_status().
 */
static void core_pairing_task(void *arg)
{
    PairEvent ev;

    while (1) {
        if (xQueueReceive(s_pair_queue, &ev, portMAX_DELAY) != pdTRUE) { continue; }

        // --- Find or claim a slot ---
        int slot = find_node_by_mac(ev.mac);
        bool is_new = (slot < 0);

        if (is_new) {
            slot = find_empty_slot();
            if (slot < 0) {
                ESP_LOGW(TAG, "Registry full — rejecting PAIR_REQ from [%02x:%02x:...]",
                         ev.mac[0], ev.mac[1]);
                ui_post_status("Pairing rejected: registry full (max 6 nodes)");
                continue;
            }

            // Populate and persist the new slot
            memcpy(s_drum_nodes[slot].mac,  ev.mac,  6);
            strncpy(s_drum_nodes[slot].name, ev.name, sizeof(s_drum_nodes[slot].name) - 1);
            s_drum_nodes[slot].name[sizeof(s_drum_nodes[slot].name) - 1] = '\0';
            s_drum_nodes[slot].valid = true;

            esp_err_t nvs_ret = core_nvs_save_node(slot);
            if (nvs_ret != ESP_OK) {
                ESP_LOGE(TAG, "NVS save failed for slot %d: %s",
                         slot, esp_err_to_name(nvs_ret));
            }

            // Register the drum node as a unicast ESP-NOW peer
            espnow_transport_add_peer(ev.mac);

            ESP_LOGI(TAG, "New node registered — slot %d  name=\"%s\"  [%02x:%02x:%02x:%02x:%02x:%02x]",
                     slot, s_drum_nodes[slot].name,
                     ev.mac[0], ev.mac[1], ev.mac[2],
                     ev.mac[3], ev.mac[4], ev.mac[5]);
        } else {
            ESP_LOGI(TAG, "Re-pair — slot %d  name=\"%s\"  [%02x:%02x:...]",
                     slot, s_drum_nodes[slot].name, ev.mac[0], ev.mac[1]);
        }

        // --- Send PAIR_ACK — includes the name the Core Node has on record ---
        espnow_pair_ack_pkt_t ack = {};
        ack.hdr.type    = ESPNOW_PKT_PAIR_ACK;
        ack.hdr.node_id = 0;     // 0 = Core Node
        ack.hdr.seq     = 0;     // not tracking Core→drum sequence in Phase 2
        strncpy(ack.name, s_drum_nodes[slot].name, sizeof(ack.name) - 1);

        esp_err_t tx_ret = espnow_transport_send(ev.mac, &ack, sizeof(ack));
        if (tx_ret != ESP_OK) {
            ESP_LOGE(TAG, "PAIR_ACK send failed: %s", esp_err_to_name(tx_ret));
        } else {
            ESP_LOGI(TAG, "PAIR_ACK sent to slot %d (\"%s\")", slot, ack.name);
        }

        // --- Update g_drums[] under the LVGL lock ---
        // g_drums is owned by the LVGL task. Always hold the port mutex when writing.
        if (lvgl_port_lock(-1)) {
            strncpy(g_drums[slot].name, s_drum_nodes[slot].name,
                    sizeof(g_drums[slot].name) - 1);
            memcpy(g_drums[slot].mac, s_drum_nodes[slot].mac, 6);
            g_drums[slot].connected = true;
            lvgl_port_unlock();
        }

        // --- Post status to the active screen ---
        char msg[64];
        if (is_new) {
            snprintf(msg, sizeof(msg), "Paired: \"%s\" [%02x:%02x:%02x...]",
                     s_drum_nodes[slot].name,
                     ev.mac[0], ev.mac[1], ev.mac[2]);
        } else {
            snprintf(msg, sizeof(msg), "Re-paired: \"%s\" [%02x:%02x:...]",
                     s_drum_nodes[slot].name, ev.mac[0], ev.mac[1]);
        }
        ui_post_status(msg);
    }
}

// ---------------------------------------------------------------------------
// SPIFFS init
// ---------------------------------------------------------------------------

static void spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path              = "/spiffs",
        .partition_label        = NULL,
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

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void app_main(void)
{
    static esp_lcd_panel_handle_t panel_handle = NULL;
    static esp_lcd_touch_handle_t tp_handle    = NULL;

    // ESP-NOW + NVS init — must run before any NVS or radio call.
    // espnow_transport_init() handles nvs_flash_init() internally.
    ESP_ERROR_CHECK(espnow_transport_init());

    // Load drum node registry from NVS and pre-register each stored MAC as an
    // ESP-NOW peer so the Core Node can immediately unicast to a re-pairing node.
    core_nvs_load_nodes();
    for (int i = 0; i < MAX_DRUM_NODES; i++) {
        if (s_drum_nodes[i].valid) {
            espnow_transport_add_peer(s_drum_nodes[i].mac);
            ESP_LOGI(TAG, "Pre-registered peer slot %d: \"%s\"", i, s_drum_nodes[i].name);
        }
    }

    // Create the pairing queue before registering the receive callback.
    // The callback may fire as soon as it is registered.
    s_pair_queue = xQueueCreate(PAIR_QUEUE_DEPTH, sizeof(PairEvent));
    configASSERT(s_pair_queue);

    espnow_transport_set_recv_cb(on_drum_recv);
    ESP_LOGI(TAG, "ESP-NOW ready — listening for drum node PAIR_REQ broadcasts");

    // SPIFFS must be mounted before ui_create() calls lv_img_set_src()
    spiffs_init();

    // Touch before LCD: GT911 INT level sets the I2C address before the panel comes up
    tp_handle    = touch_gt911_init();
    panel_handle = waveshare_esp32_s3_rgb_lcd_init();

    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

    if (lvgl_port_lock(-1)) {
        lv_fs_stdio_init();

        // Mark any already-known drum nodes as connected in the UI data model
        // so their cards render with names even before the first telemetry arrives.
        for (int i = 0; i < MAX_DRUM_NODES; i++) {
            if (s_drum_nodes[i].valid) {
                strncpy(g_drums[i].name, s_drum_nodes[i].name,
                        sizeof(g_drums[i].name) - 1);
                memcpy(g_drums[i].mac, s_drum_nodes[i].mac, 6);
                g_drums[i].connected = true;
            }
        }

        ui_create();
        lvgl_port_unlock();
    }

    wavesahre_rgb_lcd_bl_on();

    // Start the pairing task now that the queue and LVGL context are ready
    xTaskCreate(core_pairing_task, "pairing", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Boot complete — pairing task running");
}
