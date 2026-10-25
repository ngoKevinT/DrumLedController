# Project Manifest: Agentic Drum LED System (ADLS)

## 1. Project Identity & Intent

* **Mission:** To create a low-latency, real-time reactive lighting system for a drum kit that provides visual feedback based on hit velocity and frequency.
* **Target Hardware:** Waveshare ESP32-S3-Touch-LCD-7B (Core Node), Seeed Studio XIAO ESP32-S3 (Drum Nodes), BTF-LIGHTING SK6812 RGBW (Cool White) LED strips at 60 LED/m, piezo sensors.
* **Key Frameworks:** ESP-IDF 5.5 in C. LVGL v8.4 for the UI (the version the Waveshare drivers need). Espressif `led_strip` (RMT) behind our own `led_hal`, for the LEDs. FreeRTOS for tasks. ESP-NOW for wireless.

---

## 2. The "Hard Truths" (Core Constraints)

Any code generated **must** adhere to these physical limits. Do not ignore these to achieve a "visual effect."

* **Power limit (per node):** the firmware governor caps LED current at **2500 mA** on battery and **2000 mA** on wall power (provisional). The drum buck (Pololu D36V50F5, 5.5A) runs at about 53% at worst case. On the bench with the interim QS-1205CME buck the cap is **1200 mA**. The trunk is 14.0–20.4V and each node steps it down to 5V locally. All limits follow the derating policy in `standards.md` §0. See `docs/hardware/power_specs.md`.
* **Latency budget:** the time from a piezo hit to the LED flash must be **< 10 ms**, with a design target of about 6 ms. That means at most ~120 LEDs per data line and pre-empting the in-flight frame on a hit (`led_engine.md` §3). Use non-blocking code; `delay()` is not allowed.
* **Memory Ceiling:** ESP32-S3 internal SRAM is limited. The 1024×600 LVGL draw buffer must be allocated in PSRAM using `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`. Large assets go in SPIFFS/LittleFS or as indexed palettes.

---

## 3. System Architecture Map

Refer to these files to understand the "Infrastructure" before suggesting changes.

### A. Core Configuration

* `/.agent/standards.md`: Coding style, error handling, and naming conventions.
* `/docs/hardware/pinout.md`: GPIO mapping for sensors, LEDs, and Display. **Read this before touching any pin assignments.**

### B. Functional Modules

* `/docs/hardware/software/hit_detection.md`: piezo signal processing (ADC continuous sampling and peak detection).
* `/docs/hardware/software/led_engine.md`: LED driver stack (`led_hal` over `led_strip`), latency budget, RGBW conversion, governor.
* `/docs/hardware/software/modes_logic.md`: lighting mode list and behavior.
* `/docs/hardware/software/ui_lvgl.md`: LVGL v8.4 Command Center. 1024×600 layout, stage-then-commit, ESP-NOW bridge.
* `/docs/hardware/power_specs.md`: power budget, software caps, protection, and Core Node revision items.

### C. Data & Assets

* `/assets/palettes/`: Color definitions for different modes (90s Rave, Ghost Note, Heat Map, Rainbow, etc.).

---

## 4. Networking & Communication Strategy

* **Asynchronous Uplink:** Drum-to-Core communication (telemetry, hit notifications) is Fire-and-Forget. The Drum Node must never block waiting for an ACK.
* **Independent Edge Execution:** Drum Nodes are autonomous. They perform trigger-and-light logic locally based on internal state, regardless of Core Node connectivity. NVS stores last-known sensitivity and mode as fallback.
* **Downlink Authority:** The Core Node is the "Source of Truth" for configuration. Transmission to Drum Nodes is the highest-priority network task.
* **Watchdog:** each Drum Node runs the ESP-IDF task watchdog (`esp_task_wdt`), so it reboots **only if the firmware hangs**. Losing Core Node packets is not a fault: the node keeps running standalone on its NVS settings. After any reboot, it reloads NVS state and resumes.

---

## 5. Agent Handover Protocol

To maintain project continuity, the Agent must follow this loop:

1. **Read:** On initialization, read this `manifest.md` and `todo.md`.
2. **Audit:** Before writing code, perform a **Power and Memory Audit** against Section 2 and `power_specs.md`.
3. **Checkpoint:** After every major feature implementation, update `todo.md` with current progress and identified technical debt.
4. **Reference:** Use relative paths (e.g., `../hardware/pinout.md`) when explaining logic choices.
5. **Pin Safety:** Never hardcode GPIO numbers. Always reference constants from `pinout.h`.

---

## 6. Active Project State

* **Current milestone:** Phase 2. Drum Node PCB design, plus drum node bench bring-up on direct wall power (SlimQ → 5A fuse → node).
* **Active bottleneck:** Core Node power grid is **under revision** after the wall-only build failed (`power_specs.md` §7).
* **Recently confirmed (2026-10-24):**
  * Drum buck: Pololu D36V50F5. TVS: P6KE27A. 50V input cap. JST-VH LED header. SN74AHCT1G125 data buffers.
  * Toolchain: ESP-IDF plus `led_strip`/`led_hal`.
  * Watchdog: task watchdog only.
  * Derating policy (`standards.md` §0).
