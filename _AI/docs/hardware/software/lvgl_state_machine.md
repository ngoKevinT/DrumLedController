# Logic Specification: LVGL Screen State Machine

Target: Waveshare ESP32-S3-Touch-LCD-7B (1024×600, GT911 capacitive touch), LVGL **v8.4**.
Input is **touch only**. There are no physical navigation buttons.

## 1. Screens (`ui_screen_t` in `components/ui/ui.h`)

| Screen | Purpose |
|---|---|
| `UI_SCREEN_MAIN` | Dashboard. Left: mode and color up/down buttons, sync, drum selection. Right: 6 drum monitor cards. |
| `UI_SCREEN_DRUM_SETTINGS` | Trigger settings for the selected drum (`g_selected_drum`): name, sensitivity, retrigger guard, hit curve. |
| `UI_SCREEN_GLOBAL_SETTINGS` | Settings for every drum at once: idle/flash brightness, flash and decay times, master brightness, power limit. Also pairing and the connected-drum list. |

* All screens are created once at startup (`ui_create()`) and kept in memory, so navigation is instant.
* Switching screens goes through `ui_navigate_to()`, with the LVGL port mutex held.

## 2. Data Synchronization

* Every screen reads and writes the shared model `g_drums[]` (`ui_drum_t`), with the LVGL mutex held.
* **Stage, then commit:** edits change local state only. **Save/Set** packs an `espnow_config_pkt_t` and unicasts it to that drum's MAC.
* ESP-NOW updates are sent only when a value has actually changed, to keep radio traffic low.
* Messages from other tasks (ESP-NOW receive, pairing) reach the UI only through `ui_post_status()` or a FreeRTOS queue. **Never call LVGL from those tasks directly**, or the firmware panics.

## 3. Open Items

* `g_power_limit_3a` (bool) should become a **wall/battery cap selector**. Turning it off must never disable the drum-node governor (`power_specs.md` §4).
