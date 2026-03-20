# Project Manifest: Agentic Drum LED System (ADLS)

## 1. Project Identity & Intent

* **Mission:** To create a low-latency, real-time reactive lighting system for a drum kit that provides visual feedback based on hit velocity and frequency.
* **Target Hardware:** Waveshare ESP32-S3-Touch-LCD-7B (Core Node), Seeed Studio XIAO ESP32-S3 (Drum Nodes), SK6812NW RGBW LED Strips, Piezo Sensors.
* **Key Frameworks:** LVGL v9 (UI), FastLED ≥3.7.7 (LED Logic), FreeRTOS (Task Management), ESP-NOW (Wireless).

---

## 2. The "Hard Truths" (Core Constraints)

Any code generated **must** adhere to these physical limits. Do not ignore these to achieve a "visual effect."

* **Power Limit (The 3.0A Per-Node Protocol):** Each Drum Node's 5V rail must not exceed **3.0A continuous** (enforced via `FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500)`). The system transport voltage is 14.8–20V — the 3.0A limit applies after local buck conversion. See `power_specs.md` for full architecture.
* **Latency Budget:** Total delay from Piezo hit to LED "Flash" must be **< 10ms**. Use non-blocking interrupts; no `delay()` calls allowed.
* **Memory Ceiling:** ESP32-S3 internal SRAM is limited. The 1024×600 LVGL draw buffer must be allocated in PSRAM using `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`. Large assets go in SPIFFS/LittleFS or as indexed palettes.

---

## 3. System Architecture Map

Refer to these files to understand the "Infrastructure" before suggesting changes.

### A. Core Configuration

* `/.agent/standards.md`: Coding style, error handling, and naming conventions.
* `/docs/hardware/pinout.md`: GPIO mapping for sensors, LEDs, and Display. **Read this before touching any pin assignments.**

### B. Functional Modules

* `/docs/software/hit_detection.md`: The math behind Piezo signal processing (ADC sampling + peak detection).
* `/docs/software/led_engine.md`: FastLED SK6812NW RGBW implementation and the lighting mode state machine.
* `/docs/software/ui_lvgl.md`: LVGL v9 Mission Control dashboard — 1024×600 layout, touch handlers, ESP-NOW bridge.

### C. Data & Assets

* `/assets/palettes/`: Color definitions for different modes (90s Rave, Ghost Note, Heat Map, Rainbow, etc.).

---

## 4. Networking & Communication Strategy

* **Asynchronous Uplink:** Drum-to-Core communication (telemetry, hit notifications) is Fire-and-Forget. The Drum Node must never block waiting for an ACK.
* **Independent Edge Execution:** Drum Nodes are autonomous. They perform trigger-and-light logic locally based on internal state, regardless of Core Node connectivity. NVS stores last-known sensitivity and mode as fallback.
* **Downlink Authority:** The Core Node is the "Source of Truth" for configuration. Transmission to Drum Nodes is the highest-priority network task.
* **Watchdog:** Each Drum Node runs a FreeRTOS watchdog. Auto-reboot after 10s without a valid ESP-NOW packet. On reboot, reload NVS state and resume — LEDs must stay alive.

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

* **Current Milestone:** Phase 2 — Drum Node PCB design and Core Node power grid wiring.
* **Active Bottleneck:** Reconciling LVGL v9 display driver initialization for the Waveshare ESP32-S3-Touch-LCD-7B (CH32V003 backlight controller, GT911 touch, RGB panel timings).
* **Recently Confirmed:** TVS diode (P6KE24A) on SP13 input and 100nF 0603 decoupling cap on XIAO VCC are locked into the Drum Node hardware spec.
