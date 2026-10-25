# ADLS — Claude Code Session Primer
# Paste this at the start of every new Claude Code session.
# Delete sections that aren't relevant to today's work.

---

## Orientation

Read the following files in order before doing anything else:

1. `CLAUDE.md` — constraints, pin assignments, LVGL version, hard rules
2. `manifest.md` — project intent, architecture map, active milestone
3. `docs/hardware/pinout.md` — GPIO assignments for Core Node and Drum Node
4. `docs/hardware/architecture.md` — confirmed design decisions (do not re-open these)

Do not write any code until you have read all four.

---

## Current Project State

- **Core Node:** Waveshare ESP32-S3-Touch-LCD-7B · 1024×600 · LVGL v8.4
- **Drum Node:** Seeed Studio XIAO ESP32-S3
- **Active phase:** [REPLACE WITH CURRENT PHASE, e.g. "Phase 2 — Drum Node PCB schematic"]
- **Working on today:** [REPLACE WITH TASK, e.g. "Drum Node trigger firmware — ADC sampling and peak detection"]

---

## Today's Task

[DESCRIBE WHAT YOU WANT TO BUILD OR FIX]

Examples:
- "Write the FreeRTOS trigger task for the Drum Node that samples A0/A1 at ≥10 kHz (ADC continuous mode) and queues an LED update on peak detection"
- "Write the LVGL v8.4 display init for the Waveshare 7B including CH32V003 backlight and GT911 touch"
- "Review the drum node schematic in [file] against the protection component checklist in CLAUDE.md section 7"

---

## Constraints Reminder (copy-paste as needed)

- Firmware is **ESP-IDF 5.5 C**. No Arduino and no FastLED. LEDs go through `led_hal` over Espressif `led_strip`.
- No `delay()` anywhere. Use `esp_timer`, tick counts or FreeRTOS primitives.
- **Derating:** hardware ≤ ~75% of rating; software limits 15–25% below the hardware limit (`standards.md` §0)
- LED data: **D9** (GPIO8) ring 1 and **D10** (GPIO9) ring 2, both active, through an AHCT1G125 buffer. Never D0.
- ≤ ~120 LEDs per data line; latency target ≈ 6 ms (hard limit 10 ms)
- Trigger pins: **A0** (GPIO1) ch1 and **A1/D1** (GPIO2) ch2, each with a load resistor to GND
- Buttons: **A2/D2** (GPIO3) test, **D3** (GPIO4) mode. Left side, quiet digital.
- Status LEDs: **D5** (GPIO6) blue, **D8** (GPIO7) red
- UART reserved: **D6/TX** (GPIO43) and **D7/RX** (GPIO44), kept free for debug
- LVGL version is **v8.4**. Don't use v9 APIs.
- LED cap per node: 2500 mA battery / 2000 mA wall / **1200 mA on the bench QS-1205CME buck**. Never above 2500.
- PSRAM is required for the LVGL draw buffer
- All GPIO numbers come from `pinout.h` constants. Never hardcode them inline.
- Core Node power grid is **under revision** (`power_specs.md` §7)

---

## Files Relevant to Today

[LIST THE FILES YOU WANT CLAUDE CODE TO FOCUS ON]

Examples:
- `src/esp32/drum-node/main/trigger.c`: ADC sampling and peak detection
- `src/esp32/drum-node/main/led_engine.c`: mode dispatch, RGBW conversion, governor
- `src/esp32/core-node/components/ui/ui_main.c`: LVGL main dashboard layout
- `docs/hardware/pinout.md` — reference only, do not modify
- `todo.md` — update this when the task is complete

---

## End of Session Checklist

Before closing, ensure:

- [ ] `todo.md` updated — completed tasks checked off, new debt noted
- [ ] Any new confirmed decisions added to `docs/hardware/architecture.md`
- [ ] Any pin changes reflected in `docs/hardware/pinout.md`
- [ ] `CLAUDE.md` updated if any hard constraints changed
- [ ] Commit message written — describes *why*, not just *what*
