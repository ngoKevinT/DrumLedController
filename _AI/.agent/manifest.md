# Project Manifest: Agentic Drum LED System (ADLS)

## 1. Project Identity & Intent

* **Mission:** To create a low-latency, real-time reactive lighting system for a drum kit that provides visual feedback based on hit velocity and frequency.
* **Target Hardware:** ESP32 (Main Controller), WS2812B/SK6812 LED Strips, Piezo Sensors.
* **Key Frameworks:** LVGL (UI), FastLED (LED Logic), FreeRTOS (Task Management).

---

## 2. The "Hard Truths" (Core Constraints)

Any code generated **must** adhere to these physical limits. Do not ignore these to achieve a "visual effect."

* **Power Limit (The 3.0A Protocol):** Total system draw must never exceed **3.0 Amps**. Agents must calculate peak brightness based on LED count ($P = I \times V$).
* **Latency Budget:** Total delay from Piezo hit to LED "Flash" must be **< 10ms**. Use non-blocking interrupts; no `delay()` calls allowed.
* **Memory Ceiling:** ESP32 SRAM is limited. Large LVGL assets must be stored in SPIFFS/LittleFS or optimized as indexed palettes.

---

## 3. System Architecture Map

Refer to these files to understand the "Infrastructure" before suggesting changes.

### A. Core Configuration

* `/.agent/standards.md`: Coding style, error handling, and naming conventions.
* `/docs/hardware/pinout.md`: GPIO mapping for sensors, LEDs, and Display.

### B. Functional Modules

* `/docs/software/hit_detection.md`: The math behind Piezo signal processing (ADC sampling + Dot Product similarity).
* `/docs/software/led_engine.md`: FastLED implementation and the "7 Modes" logic.
* `/docs/software/ui_lvgl.md`: The state machine for the on-drum display and menu system.

### C. Data & Assets

* `/assets/palettes/`: Color definitions for different genres (90s Rave, Metal, Jazz).

---

## 4. Networking & Communication Strategy

Asynchronous Uplink: Drum-to-Core communication (Telemetry) is Fire-and-Forget. The Drum Node must never block for an acknowledgement (ACK).

Independent Edge Execution: Drum Nodes are autonomous. They must perform trigger-and-light logic locally based on their internal state, regardless of Core Node connectivity.

Downlink Authority: The Core Node is the "Source of Truth" for configurations. Transmission to Drum Nodes is the highest-priority network task.

---

## 5. Agent Handover Protocol

To maintain project continuity, the Agent must follow this loop:

1. **Read:** On initialization, the Agent must read this `manifest.md` and the `todo.md`.
2. **Audit:** Before writing code, the Agent must perform a **Power and Memory Audit** against Section 2.
3. **Checkpoint:** After every major feature implementation, the Agent must update `/.agent/todo.md` with current progress and identified technical debt.
4. **Reference:** Use relative paths (e.g., `../hardware/pinout.md`) when explaining logic choices.

---

## 6. Active Project State

* **Current Milestone:** Phase 1 - Basic Piezo-to-LED triggering and Power Management.
* **Active Bottleneck:** Optimizing the LVGL render loop to prevent stuttering during high-speed drum rolls.

---
