# Logic Specification: Lighting Modes

**Status: v1 list approved 2026-10-24.**
**Source of truth for indices:** `drum_mode_t` in `src/esp32/shared/espnow_transport/espnow_transport.h`.
The Core UI labels (`ui_mode_names[]` in `components/ui/ui.c`) are tied to it by a `_Static_assert`.

Rules:
* Append new modes before `DRUM_MODE_COUNT`.
* **Never renumber.** `mode_idx` is stored in drum-node NVS and sent over ESP-NOW.

## v1 Modes

| ID | Enum | UI label | Idle behavior | Hit behavior |
|---|---|---|---|---|
| 0 | `DRUM_MODE_SOLID` | SOLID | Base color at `idle_brightness` (0 = off) | Flash to `flash_brightness` and hold for `flash_time_ms`, then decay over `decay_time_ms`. Velocity scaled by `hit_curve`. |
| 1 | `DRUM_MODE_PULSE` | PULSE | Slow breathing glow around `idle_brightness` | Brightness spike on top of the breath, then decay |
| 2 | `DRUM_MODE_RAINBOW` | RAINBOW | Hue cycle around the ring. Speed 0 gives a static rainbow gradient. | Ring flashes brighter, then decays back to the cycle |
| 3 | `DRUM_MODE_CHASE` | CHASE | Base color at `idle_brightness` | A pulse starts at the sensor position and travels outward around the ring. Velocity sets length and brightness. |
| 4 | `DRUM_MODE_HEAT_MAP` | HEAT MAP | Hue reflects the rolling hit rate over a 10 s window (Blue → Yellow → Red) | Brightness follows velocity |
| 5 | `DRUM_MODE_CUSTOM` | CUSTOM | Per-drum pattern slot, reserved for user-built patterns. **Renders as SOLID until defined.** | Same as SOLID |
| 6 | `DRUM_MODE_STEALTH` | STEALTH | Strip off. Diagnostic LEDs (blue, red, GPIO21) stay active. | No strip output. Telemetry is still sent. |

## Looks Built From Settings (no extra firmware mode)

The old mode lists had several "modes" that are really SOLID or PULSE with different settings. These can become Core Node presets later:

| Look | Built from |
|---|---|
| Reactive flash | SOLID, `idle_brightness = 0` |
| Velocity fade | SOLID, `hit_curve = LOG` or `EXP` |
| Ghost note | PULSE, `hit_curve = EXP` (soft hits barely show), low idle |
| Static rainbow | RAINBOW, speed 0 |

## Backlog (v2 candidates)

Each of these needs its own render code. They'll be appended as IDs 7 and up when built.

* **STROBE / RAVE:** strobe, either free-running or hit-triggered. **Default rate ≤ 3 Hz** (photosensitive-seizure risk at 3–30 Hz). Faster rates only behind an explicit Core Node opt-in that shows a warning.
* **BOUNCE:** a segment bounces end to end; velocity sets its speed.
* **SPECTRUM:** each hit advances the hue by 15°.
* **VOID:** the ring stays lit and each hit punches a dark hole that fades back to color.

## Mode Button (drum node D3)

* Cycles through a **favorites list** chosen on the Core Node (up to 5 modes).
* Standalone, or before any favorites have been received, it uses the built-in default list: SOLID → PULSE → RAINBOW → CHASE → STEALTH.
* The selected mode is saved to NVS. The next CONFIG from the Core Node overrides it.
* This needs a `favorites` field in `espnow_config_pkt_t` (for example a `uint8_t` bitmask). Not implemented yet; see todo.md Phase 6.

## Safety Rules (every mode)

* **Power:** every frame goes through the governor (`power_specs.md` §4). No mode may bypass it to get a visual effect.
* **Fail-safe:** if the trigger input is lost (continuous noise, or an unplugged piezo), the mode falls back to a low-brightness standby heartbeat, never a stuck full flash.
* **Unknown index:** a `mode_idx ≥ DRUM_MODE_COUNT` received over the radio is clamped to `DRUM_MODE_SOLID`.

## Implementation

A render-function dispatch table indexed by `drum_mode_t` in the LED engine (`led_engine.md`). Each function writes the RGB frame. The engine then layers hits, converts to RGBW, applies the governor and flushes.
