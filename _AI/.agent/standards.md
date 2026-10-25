# Coding Standards & Engineering Principles: ADLS

## 0. Safety Margins (Derating Policy)

Every limit has a buffer. Never design to exactly 100% of a rating.

* **Hardware current and power:** a part runs at **≤ ~75% of its continuous rating** at worst case. 80% is the absolute maximum.
* **Hardware voltage:** a part's rating must be **≥ 1.25× the maximum steady voltage** and also **above the worst transient clamp**. A TVS standoff voltage must sit above the maximum supply voltage, with margin.
* **Software limits:** set **15–25% below** the hardware limit they protect. For example, if a part is rated 3.0A, the software limit is 2.5A or lower.
* **Write down the margin:** every limit in the docs states the ceiling it protects and its percentage of that ceiling (see `docs/hardware/power_specs.md`).
* **Fail-safe:** a value received over the radio is always clamped to the local hard maximum before it is used.

---

## 1. Core Philosophy

* **Performance first:** this is a real-time system. Latency is the enemy.
* **Safety second:** power limits are physical constraints, not suggestions (§0).
* **Readability third:** use descriptive names so humans and other agents can audit the logic easily.

---

## 2. Power Management

Every LED frame must respect the power budget. Full spec: `docs/hardware/power_specs.md` §4.

* **Caps:** 2500 mA battery, 2000 mA wall (provisional), 1200 mA on the bench buck. Never exceed 2500 mA.
* **Governor:** before every `led_hal_show()`, estimate current from the **post-RGBW** channel values using calibrated per-channel mA plus a 10% margin, and scale down to the active cap.
* **No bypass:** no mode, UI toggle or radio command can disable the governor.

---

## 3. Real-Time Logic & Timing

To meet the **< 10 ms latency budget** (design target ≈ 6 ms):

* **No `delay()`:** using it is a critical failure. Use `esp_timer_get_time()`, `xTaskGetTickCount()`, `vTaskDelayUntil()` or FreeRTOS notifications and queues.
* **Animation timing:** drive frames from an `esp_timer` or a periodic task. Never spin-wait.
* **Interrupt safety:** piezo sampling uses ADC continuous mode (DMA) feeding the highest-priority application task. Keep ISRs short: notify a task, and do no work in the ISR itself.

---

## 4. C Implementation Standards (ESP-IDF)

The firmware is **ESP-IDF C**. Arduino and FastLED APIs are not used.

* **Naming:** `snake_case` for variables and functions. `snake_case_t` for typedefs; existing `PascalCase` structs such as `DrumConfig` may stay.
* **Constants:** use `static const` for values. Use `enum` for compile-time integer constants such as array sizes and case labels. Keep `#define` for cases the language requires, such as string literals for NVS keys, and never for tunable values.
* **Memory:** Avoid `new` and `delete` (Dynamic Allocation) inside the main loop to prevent heap fragmentation. Use static allocation or object pooling.
* **Hardware Abstraction:** Keep pin definitions and hardware-specific code in `pinout.h`. Do not hardcode GPIO numbers in the logic files.

---

## 5. LVGL Integration Standards

* **Main Loop Balance:** The LVGL timer handler (`lv_timer_handler()`) should be called frequently enough to stay responsive but not so often that it starves the LED or Sensor tasks.
* **Separation of Concerns:** UI logic (LVGL) must be decoupled from Hardware logic (LED engine/sensors). Use an "Event Bridge" or a shared state struct to communicate.
* **Non-Blocking UI:** Do not perform heavy math or I/O operations inside an LVGL event callback.

---

## 6. Documentation & Commenting

* **The "Why," not the "How":** Don't comment `x = x + 1; // Increment x`. Comment the intent: `x++; // Advance to the next LED in the hit-streak animation`.
* **Math Explanations:** Any complex trigonometry or bitwise operations must be preceded by a comment explaining the "First Principle" logic (e.g., "Using a sine wave to modulate brightness to simulate a natural decay").

---

## 7. Agent-Human Handover Rules

* **Commit Messages:** Agents must provide concise, descriptive commit summaries.
* **Refactoring:** If an Agent sees a way to reduce "Resource Debt" (CPU cycles or RAM), it must propose the change before executing it.
* **Error Handling:** Always implement "Fail-Safe" modes. If a sensor disconnects, the LEDs should default to a "Standby" heartbeat, not stay stuck on high-brightness.

---
