/*****************************************************************************
 * File        : espnow_transport.c
 * Component   : espnow_transport
 * Description : Thin ESP-NOW transport layer for the ADLS Core Node.
 *
 *   Phase 1 scope (implemented here):
 *     - NVS, WiFi (STA, power-save OFF), ESP-NOW init
 *     - Broadcast send
 *     - on_send: logs MAC + seq + OK/FAIL for every transmitted packet
 *     - on_recv: forwards to user callback if registered (stub for Phase 2)
 *     - Ping task: broadcasts ESPNOW_PKT_PING every second (radio self-test)
 *
 *   See espnow_transport.h for Phase 2 / Phase 3 extension points.
 *****************************************************************************/

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_now.h"
#include "esp_log.h"

#include "espnow_transport.h"

static const char *TAG = "espnow";

// Broadcast address — received by all ESP-NOW nodes on the same channel
static const uint8_t k_broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Rolling sequence counter shared by all outbound packets
static uint16_t s_seq = 0;

// User-supplied receive callback (Phase 2); NULL until set
static espnow_recv_cb_t  s_recv_cb      = NULL;

// Status callback — called with human-readable strings from the pairing task
static espnow_status_cb_t s_status_cb   = NULL;

// Pairing task handle; NULL when no session is running
static TaskHandle_t       s_pairing_task = NULL;

// ---------------------------------------------------------------------------
// Internal ESP-NOW callbacks
// ---------------------------------------------------------------------------

/**
 * Called by the ESP-NOW stack after each transmitted frame.
 * ESP-IDF v5.4+ changed the first argument from (const uint8_t *mac) to
 * (const wifi_tx_info_t *tx_info) — peer_addr lives inside that struct.
 * Runs in a WiFi task — keep it short, no LVGL calls.
 */
static void on_send(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
{
    const uint8_t *mac = tx_info->des_addr;
    ESP_LOGI(TAG, "tx [%02x:%02x:%02x:%02x:%02x:%02x] seq=%-4u %s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             s_seq,
             status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

/**
 * Called by the ESP-NOW stack when a frame is received.
 * ESP-IDF 5.x signature uses esp_now_recv_info_t.
 * Runs in a WiFi task — forward to queue, do not call LVGL API here.
 */
static void on_recv(const esp_now_recv_info_t *info,
                    const uint8_t *data, int len)
{
    if (s_recv_cb && data && len > 0) {
        s_recv_cb(info->src_addr, data, (size_t)len);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

esp_err_t espnow_transport_init(void)
{
    // -- NVS (required by WiFi driver) --------------------------------------
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition corrupt; erasing and re-initialising");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // -- Network interface + event loop (WiFi prerequisites) ----------------
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // -- WiFi — STA mode, power-save OFF ------------------------------------
    // Power-save must be disabled: WIFI_PS_NONE cuts receive latency from
    // ~100 ms (DTIM sleep) to ~1 ms, which is essential for the <10 ms
    // hit-to-LED latency budget.
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    // -- ESP-NOW ------------------------------------------------------------
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_send));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));

    // Pre-register the broadcast peer once; required before any
    // esp_now_send() to FF:FF:FF:FF:FF:FF.
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, k_broadcast_mac, 6);
    peer.channel = 0;     // 0 = follow the current WiFi channel
    peer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    ESP_LOGI(TAG, "ESP-NOW ready (WiFi STA, power-save OFF)");
    return ESP_OK;
}

esp_err_t espnow_transport_broadcast(const void *data, size_t len)
{
    return esp_now_send(k_broadcast_mac, (const uint8_t *)data, len);
}

void espnow_transport_set_recv_cb(espnow_recv_cb_t cb)
{
    s_recv_cb = cb;
}

esp_err_t espnow_transport_add_peer(const uint8_t mac[6])
{
    if (esp_now_is_peer_exist(mac)) {
        return ESP_OK;
    }
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;
    peer.encrypt = false;
    return esp_now_add_peer(&peer);
}

esp_err_t espnow_transport_send(const uint8_t mac[6],
                                const void *data, size_t len)
{
    return esp_now_send(mac, (const uint8_t *)data, len);
}

// ---------------------------------------------------------------------------
// Status callback setter
// ---------------------------------------------------------------------------

void espnow_transport_set_status_cb(espnow_status_cb_t cb)
{
    s_status_cb = cb;
}

// ---------------------------------------------------------------------------
// Pairing task + controls
// ---------------------------------------------------------------------------

static void pairing_task(void *arg)
{
    uint32_t duration_s = (uint32_t)(uintptr_t)arg;
    char msg[48];
    espnow_ping_pkt_t pkt;
    pkt.hdr.type    = ESPNOW_PKT_PING;
    pkt.hdr.node_id = 0;

    // duration_s == 0 means run indefinitely; use UINT32_MAX as the counter
    uint32_t remaining = (duration_s == 0) ? UINT32_MAX : duration_s;

    for (uint32_t t = remaining; t > 0; t--) {
        // Post status at the very start and every 5 s thereafter
        if (t == remaining || t % 5 == 0) {
            if (duration_s == 0) {
                snprintf(msg, sizeof(msg), "Pairing: active");
            } else {
                snprintf(msg, sizeof(msg), "Pairing: %lus", (unsigned long)t);
            }
            if (s_status_cb) s_status_cb(msg);
        }

        pkt.hdr.seq = ++s_seq;
        esp_err_t err = espnow_transport_broadcast(&pkt, sizeof(pkt));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "broadcast failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (s_status_cb) s_status_cb("Pairing: done");
    s_pairing_task = NULL;
    vTaskDelete(NULL);
}

void espnow_transport_start_pairing(uint32_t duration_s)
{
    // Stop any currently running session
    if (s_pairing_task) {
        vTaskDelete(s_pairing_task);
        s_pairing_task = NULL;
    }
    if (xTaskCreate(pairing_task, "espnow_pair", 2048,
                    (void *)(uintptr_t)duration_s, 5, &s_pairing_task) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create pairing task");
    } else {
        ESP_LOGI(TAG, "Pairing started (%lus)", (unsigned long)duration_s);
    }
}

void espnow_transport_stop_pairing(void)
{
    if (s_pairing_task) {
        vTaskDelete(s_pairing_task);
        s_pairing_task = NULL;
        if (s_status_cb) s_status_cb("Pairing: stopped");
        ESP_LOGI(TAG, "Pairing stopped");
    }
}
