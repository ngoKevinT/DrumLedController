# Coding Standards & Engineering Principles: ADLS

## 1. Core Philosophy

* **Performance First:** This is a real-time system. Latency is the enemy.
* **Safety Second:** The 3.0A power limit is a hard physical constraint, not a suggestion.
* **Readability Third:** Use descriptive names so humans and other agents can audit the logic easily.

---

## 2. Power Management (The 3.0A Protocol)

Every LED-related function must respect the power budget.

* **Global Brightness:** Use `FastLED.setBrightness()` to cap the total draw.
* **Power Estimation:** Before pushing a new frame, use `calculate_max_brightness_for_current()` to ensure the sum of all R, G, and B values does not exceed 3000mA.
* **Safety Scaling:** If a mode requests 100% white on all strips, the code must automatically scale the brightness down to stay within the 3.0A limit.

---

## 3. Real-Time Logic & Timing

To maintain the **< 10ms latency budget**, follow these rules:

* **No `delay()`:** Using `delay()` is a critical failure. Use `millis()` or `micros()` for non-blocking timing.
* **FastLED Timing:** Use `EVERY_N_MILLISECONDS` or `EVERY_N_MICROSECONDS` macros for animation updates.
* **Interrupt Safety:** Piezo hit detection must occur in an ISR (Interrupt Service Routine) or a high-priority FreeRTOS task. Keep ISRs "Short and Fast."

---

## 4. C++ Implementation Standards

* **Variable Naming:** Use `snake_case` for variables and `PascalCase` for Classes.
* **Constants:** Always use `constexpr` or `const` for pin definitions and math constants. Never use `#define` for values.
* **Memory:** Avoid `new` and `delete` (Dynamic Allocation) inside the main loop to prevent heap fragmentation. Use static allocation or object pooling.
* **Hardware Abstraction:** Keep pin definitions and hardware-specific code in `pinout.h`. Do not hardcode GPIO numbers in the logic files.

---

## 5. LVGL Integration Standards

* **Main Loop Balance:** The LVGL timer handler (`lv_timer_handler()`) should be called frequently enough to stay responsive but not so often that it starves the LED or Sensor tasks.
* **Separation of Concerns:** UI logic (LVGL) must be decoupled from Hardware logic (FastLED/Sensors). Use an "Event Bridge" or a shared state struct to communicate.
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
