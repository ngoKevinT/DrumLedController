# ADLS: Automated Drum Lighting System (Nexus PDU)

Lights up a drum kit in response to hits.

* Each drum has a **Drum Node**: a Seeed XIAO ESP32-S3 that reads a piezo trigger and drives two SK6812 RGBW LED rings. Hit to light takes less than 10 ms.
* A **Core Node** (Waveshare ESP32-S3-Touch-LCD-7B, 1024×600 touchscreen) configures every drum over ESP-NOW and supplies a 14–20V power trunk to each one.

## Layout

| Path | Contents |
|---|---|
| `src/esp32/drum-node/` | Drum Node firmware (ESP-IDF C) |
| `src/esp32/core-node/` | Core Node firmware: LVGL v8.4 UI, vendor display/touch drivers, SPIFFS assets |
| `src/esp32/shared/espnow_transport/` | ESP-NOW transport and the wire format (source of truth for packets) |
| `.agent/` | Manifest, coding standards, task list, session primer |
| `docs/hardware/` | Pinout, power spec, architecture and confirmed decisions |
| `docs/hardware/software/` | Hit detection, LED engine, lighting modes, UI specs |

## Start here

1. `../CLAUDE.md`: quick-start constraints
2. `.agent/manifest.md`: intent and active milestone
3. `docs/hardware/pinout.md`, `docs/hardware/power_specs.md`, `docs/hardware/architecture.md`
4. `.agent/todo.md`: current tasks

## Build

ESP-IDF 5.5. From `src/esp32/drum-node/` or `src/esp32/core-node/`:

```
idf.py build flash monitor
```
