# UI Specification: Core Node Command Center (LVGL v8.4)

**Target:** Waveshare ESP32-S3-Touch-LCD-7B. 1024×600 RGB panel, GT911 touch, LVGL **v8.4**.
The source of truth for the layout is the header comment of each screen file in
`src/esp32/core-node/components/ui/`. This document summarizes those files and the rules behind them.

Screen states and navigation: `lvgl_state_machine.md`.

---

## 1. Main Dashboard (`ui_main.c`)

Two 512px panels.

**Left: control panel**
* `MODE UP / MODE DOWN` and `COLOR UP / COLOR DOWN` buttons. These stage changes for the selected drum.
* `SYNC COLOR & MODE`: sends the staged settings.
* A status label (last action), plus `←  SET  →` for drum selection.

**Right: drum monitor**
* A debug line, then a 2×3 grid of **drum cards** (244×148 px each). Each card shows:
  * Color swatch with name, `T:` threshold and `R:` live reading.
  * Mode name and color hex.
  * **Selected** state: a 4px white left border.
  * **Hit flash:** the card background jumps to the drum's color, and a one-shot 300 ms timer restores it.
* Bottom-right icon buttons open Drum Settings (`S:/settings_drum.png`) and Global Settings (`S:/settings.png`).

## 2. Drum Settings (`ui_drum_settings.c`)

For `g_drums[g_selected_drum]`:
* Name (textarea with an on-screen keyboard).
* Sensitivity, 0–1023.
* Retrigger guard, in ms.
* Hit curve: LINEAR / LOG / EXP.
* Save.

## 3. Global Settings (`ui_global_settings.c`)

* **Left panel:** idle and flash brightness, flash time (10–2000 ms), decay time (50–3000 ms), master brightness, power limit.
* **Right panel:** Enter Pairing Mode, connection status, connected-drum list (6 slots), debug line, Save.

---

## 4. Interaction: Stage, Then Commit

1. **Select:** tap a drum card. The left panel loads that drum's last-known state.
2. **Edit:** changes update the local `g_drums[]` model only. Nothing is transmitted yet.
3. **Commit:** Sync/Save packs an `espnow_config_pkt_t` and unicasts it to the drum's MAC.

## 5. Performance Rules

* **Draw buffer in PSRAM** (`heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`). It's too large for internal SRAM at 1024×600.
* **Large images** (wallpapers, icons) live in SPIFFS and are loaded into PSRAM at boot.
* **Fonts:** Montserrat 24/32/48 for primary readouts so they're legible from behind the kit; 12–20 for card detail.
* **Invalidate only what changed:** update a card's labels only when new telemetry arrives. No full-screen redraws.
* **Thread safety:** ESP-NOW and pairing callbacks never call LVGL. They post through `ui_post_status()` or a FreeRTOS queue, and the LVGL task applies the update with the port mutex held.

## 6. Telemetry Handling (Uplink)

When an `espnow_telemetry_pkt_t` arrives:
1. Look up the drum by `hdr.node_id` or MAC.
2. Queue an update to the LVGL task: the `R:` reading and a card hit flash.
3. Mark a drum OFFLINE after 3 missed heartbeats (1 s poll, 3.5 s timeout).
