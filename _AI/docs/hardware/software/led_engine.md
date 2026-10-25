# Logic Specification: LED Engine

## 1. Architectural Intent

The LED Engine consumes hits from the Hit Detection system (`hit_detection.md`). It must be non-blocking and frame-rate independent. It uses a **layered buffer**:

* **Background layer:** the idle state, for example a faint breathing glow.
* **Event layer:** the reactive flash triggered by a drum hit.

## 2. Driver Stack

```
modes (modes_logic.md)
   ↓  RGB / HSV frame buffer
led_engine — layering, decay, RGB→RGBW conversion, power governor
   ↓  GRBW bytes
led_hal    — init / set_pixel / show / abort       ← the ONLY strip-specific file
   ↓
Espressif led_strip (RMT + DMA) or a custom RMT encoder
```

* **Toolchain:** ESP-IDF C with Espressif's `led_strip` component (SK6812 timing, GRBW pixel format).
  * FastLED is not used. On ESP-IDF 5+, its ESP32 output driver wraps `led_strip` anyway, so it offers no speed advantage, and it would pull in C++ and possibly the Arduino core.
* **Strip-agnostic:** `led_hal` is the only file that knows the chipset. Changing to another single-wire chipset, or to a clocked SPI strip such as APA102/SK9822, means replacing `led_hal` only.
* **Two outputs:** D9 drives ring 1 and D10 drives ring 2, each on its own RMT channel and transmitted **in parallel**.
* **Static allocation:** frame buffers are static arrays sized for the maximum LED count per line. No heap allocation in the render path.

## 3. Latency Budget (target ≤ 6 ms, hard limit 10 ms)

Each RGBW pixel is 32 bits at 1.25 µs per bit, which is **40 µs per LED**.

| Stage | Time |
|---|---|
| Hit detection (attack window) | ≤ 1 ms |
| Render plus governor | < 0.5 ms |
| Frame transmit, 115 LEDs per line, both lines in parallel | 4.6 ms |
| **Total** | **≈ 6 ms** |

**Rules that follow from this:**
1. **At most ~120 LEDs per data line.** Beyond that, the frame time alone eats the budget margin.
2. **Pre-empt frames on hit:** when a hit arrives during an in-flight background frame, `led_hal_abort()` stops the transmission and the hit frame is sent immediately.
   * Without this, the worst case is 4.6 ms (in-flight frame) + 4.6 ms (hit frame) + 1 ms (detection) ≈ 10.2 ms, which is over budget.
   * If the `led_strip` API can't abort a transmission, `led_hal` uses a custom RMT encoder instead.
3. **Hit before telemetry:** the LED update is queued **before** any ESP-NOW telemetry is sent.
4. **Measure the result, don't trust the calculation:** toggle a spare GPIO at the ADC threshold crossing and at the RMT transmit start, and check the gap on a scope.

## 4. Timing & Animation

* **Frame rate:** 60–100 FPS for the background layer, driven by an `esp_timer` or a FreeRTOS task period, never by `delay()`. At 4.6 ms per frame, 100 FPS keeps the line busy 46% of the time. Don't go higher.
* **Temporal blending:** use integer blend and fade helpers (8-bit scale). Rapid hits must blend, not hard-cut.
* **Velocity scaling:** map the hit velocity through the configured `hit_curve` (linear / log / exp) to the V (value) of the HSV color.

## 5. RGB → RGBW Conversion

The strip's white channel is **Cool White (~6000 K)**. That's close to the white point of the RGB LEDs, so extracting white directly works well:

```c
// The W LED renders the common grey component more efficiently than mixing
// R+G+B, so move min(r,g,b) into W and subtract it from each color channel.
uint8_t w = MIN(r, MIN(g, b));
r -= w; g -= w; b -= w;
```

Run the governor **after** this conversion (see `power_specs.md` §4).

## 6. Render Loop

1. **Apply decay:** fade active pixels toward the background color over `decay_time_ms`.
2. **Process new hits:** inject color at the trigger point (event layer).
3. **Convert:** RGB → RGBW.
4. **Governor:** scale to the active power cap (battery 2500 / wall 2000 / bench 1200 mA).
5. **Flush:** `led_hal_show()`. The RMT and DMA hardware does the transmit, so the trigger task, which has higher priority, is never blocked.
