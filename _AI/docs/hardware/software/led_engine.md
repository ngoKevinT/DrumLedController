# Logic Specification: FastLED Engine

## 1. Architectural Intent
The LED Engine is a "Consumer" of the Hit Detection system. It must be non-blocking and frame-rate independent. It uses a **Layered Buffer** approach:
* **Background Layer:** The idle state (e.g., a faint breathing glow).
* **Event Layer:** The reactive flash triggered by a drum hit.

## 2. Timing & Animation
* **Target Frame Rate:** 60-100 FPS (handled via `EVERY_N_MILLISECONDS`).
* **Temporal Blending:** Animations must use `nblend()` or `fadeToBlackBy()` to ensure smooth transitions between rapid hits.
* **Velocity Scaling:** The `Peak Velocity` from `hit_detection.md` must be mapped to the `V` (Value) in HSV color space.

## 3. The Render Loop (First Principles)
1. **Apply Decay:** All active pixels are faded toward the background color based on `fade_duration`.
2. **Process New Hits:** If a hit is detected, "inject" color into the buffer at the trigger point.
3. **Apply Global Corrections:** Apply the **3.0A Power Governor** (scaling brightness down if needed).
4. **Flush:** Call `FastLED.show()`.
