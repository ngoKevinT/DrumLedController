#pragma once

/*****************************************************************************
 * File        : espnow_transport.h
 * Component   : espnow_transport
 * Description : Thin ESP-NOW transport layer for the ADLS Core Node.
 *
 *   Grow this component in phases:
 *
 *   Phase 1 (Core only, no drum nodes yet)
 *     espnow_transport_init()
 *     espnow_transport_start_ping_task()   <- proves radio is alive
 *
 *   Phase 2 (drum nodes available)
 *     espnow_transport_set_recv_cb()       <- receive telemetry
 *     espnow_transport_add_peer()          <- register each drum node
 *     espnow_transport_send()              <- unicast config packets
 *
 *   Phase 3 (production)
 *     Remove espnow_transport_start_ping_task()
 *     Wire cb_save() → espnow_transport_send(espnow_config_pkt_t)
 *     Wire recv_cb  → ui_main_drum_flash() / ui_main_drum_update()
 *****************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

// ---------------------------------------------------------------------------
// Packet types
// ---------------------------------------------------------------------------

typedef enum {
    ESPNOW_PKT_PING      = 0x01, /**< Phase 1: heartbeat / radio self-test     */
    ESPNOW_PKT_TELEMETRY = 0x02, /**< Phase 2: drum hit data  (drum → core)    */
    ESPNOW_PKT_CONFIG    = 0x03, /**< Phase 3: LED config push (core → drum)   */
    ESPNOW_PKT_PAIR_REQ  = 0x10, /**< Phase 2: pairing broadcast (drum → all)  */
    ESPNOW_PKT_PAIR_ACK  = 0x11, /**< Phase 2: pairing reply   (core → drum)   */
} espnow_pkt_type_t;

// ---------------------------------------------------------------------------
// Wire-format packet structs — packed (no padding between fields)
// ---------------------------------------------------------------------------

#pragma pack(push, 1)

/** Common header — first 4 bytes of every packet. */
typedef struct {
    uint8_t  type;     /**< espnow_pkt_type_t                               */
    uint8_t  node_id;  /**< 0 = Core Node, 1-6 = Drum Nodes                */
    uint16_t seq;      /**< Rolling sequence counter (per sender)           */
} espnow_hdr_t;

/** Phase 1 — ping (header only). */
typedef struct {
    espnow_hdr_t hdr;
} espnow_ping_pkt_t;

/** Phase 2 — drum hit telemetry sent by a drum node on each trigger. */
typedef struct {
    espnow_hdr_t hdr;
    uint16_t     raw_reading; /**< Raw ADC value (0-1023)                   */
    uint8_t      velocity;    /**< Normalised hit velocity (0-255)          */
} espnow_telemetry_pkt_t;

/** Phase 2 — pairing broadcast sent by drum node on boot until ACK received. */
typedef struct {
    espnow_hdr_t hdr;
    char         name[16]; /**< Null-terminated node name, e.g. "Snare"    */
} espnow_pair_req_pkt_t;

/** Phase 2 — pairing reply sent by Core Node to the requesting drum node. */
typedef struct {
    espnow_hdr_t hdr;
    char         name[16]; /**< Optional name override from Core Node; zero = keep drum's default */
} espnow_pair_ack_pkt_t;

/** Phase 3 — LED + trigger config pushed to a drum node on Save. */
typedef struct {
    espnow_hdr_t hdr;
    uint8_t      mode_idx;
    uint32_t     color;            /**< 0x00RRGGBB                          */
    uint8_t      idle_brightness;  /**< 0-100 %                             */
    uint8_t      flash_brightness; /**< 0-100 %                             */
    uint16_t     flash_time_ms;
    uint16_t     decay_time_ms;
    uint16_t     threshold;        /**< Sensitivity (0-1023)                */
    uint16_t     retrigger_ms;     /**< Retrigger guard (0-500 ms)          */
    uint8_t      hit_curve;        /**< hit_curve_t                         */
    char         name[16];         /**< Non-empty = rename this node; zero = no rename */
} espnow_config_pkt_t;

#pragma pack(pop)

// ---------------------------------------------------------------------------
// Receive callback (Phase 2)
// ---------------------------------------------------------------------------

/**
 * Called from the ESP-NOW receive task when a packet arrives.
 *
 * @param mac  Sender's MAC address (6 bytes).
 * @param data Raw packet bytes — inspect hdr.type, then cast to the
 *             appropriate struct (espnow_telemetry_pkt_t, etc.).
 * @param len  Byte length of data.
 *
 * IMPORTANT: Do NOT call LVGL API directly here.  Post to a FreeRTOS
 * queue and process it from the LVGL task (while lvgl_port_lock is held).
 */
typedef void (*espnow_recv_cb_t)(const uint8_t mac[6],
                                 const uint8_t *data, size_t len);

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
 * @brief Initialise NVS, WiFi (STA mode, power-save OFF), and ESP-NOW.
 *
 * Call once from app_main, BEFORE lvgl_port_init().
 * Registers the internal send-status callback (logs OK/FAIL per packet).
 * Registers the internal receive callback (forwards to s_recv_cb if set).
 */
esp_err_t espnow_transport_init(void);

/**
 * @brief Broadcast raw bytes to all nodes (FF:FF:FF:FF:FF:FF).
 *
 * Safe to call from any FreeRTOS task.
 * ESP-NOW hard limit: len <= 250 bytes.
 */
esp_err_t espnow_transport_broadcast(const void *data, size_t len);

/**
 * @brief Register a callback for received packets.  Pass NULL to deregister.
 *        Call after espnow_transport_init().  (Phase 2)
 */
void espnow_transport_set_recv_cb(espnow_recv_cb_t cb);

/**
 * @brief Register a unicast peer by MAC address. (Phase 2)
 *        No-op if the peer is already registered.
 */
esp_err_t espnow_transport_add_peer(const uint8_t mac[6]);

/**
 * @brief Send a packet directly to a specific peer MAC. (Phase 2)
 *        Peer must have been added with espnow_transport_add_peer() first.
 */
esp_err_t espnow_transport_send(const uint8_t mac[6],
                                const void *data, size_t len);

// ---------------------------------------------------------------------------
// Status callback
// ---------------------------------------------------------------------------

/**
 * Called with short human-readable status strings when the pairing state
 * changes.  Runs in the context of the pairing FreeRTOS task (NOT the LVGL
 * task) — do not call LVGL API directly; use ui_post_status() instead.
 */
typedef void (*espnow_status_cb_t)(const char *msg);

/**
 * @brief Register a callback for pairing status messages.
 *        Pass NULL to deregister.  Call after espnow_transport_init().
 */
void espnow_transport_set_status_cb(espnow_status_cb_t cb);

// ---------------------------------------------------------------------------
// Pairing
// ---------------------------------------------------------------------------

/**
 * @brief Broadcast PING packets for @p duration_s seconds, posting a status
 *        message via the registered callback every 5 s and at start/end.
 *
 * If a pairing session is already running it is stopped first.
 * Pass 0 to run indefinitely until espnow_transport_stop_pairing() is called.
 * Safe to call from any FreeRTOS task.
 */
void espnow_transport_start_pairing(uint32_t duration_s);

/**
 * @brief Stop any active pairing session immediately.
 *        Posts a "Pairing: stopped" status message via the callback.
 *        No-op if no session is running.
 */
void espnow_transport_stop_pairing(void);
