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
- "Write the FreeRTOS trigger task for the Drum Node that samples A0 at >1kHz and fires a FastLED update on peak detection"
- "Write the LVGL v8.4 display init for the Waveshare 7B including CH32V003 backlight and GT911 touch"
- "Review the drum node schematic in [file] against the protection component checklist in CLAUDE.md section 7"

---

## Constraints Reminder (copy-paste as needed)

- No `delay()` anywhere — `millis()`, `micros()`, or FreeRTOS only
- LED data pin is **D9** (GPIO8) primary, **D10** (GPIO9) future — never D0
- Trigger pins: **A0/D0** (GPIO1) ch1, **A1/D1** (GPIO2) ch2, **A2/D2** (GPIO3) ring detect — all left side
- Status LEDs: **D8** (GPIO7) blue, **D7** (GPIO44) red — right side, away from analog
- Buttons: **D3** (GPIO4) test, **D4** (GPIO5) mode — left side, quiet digital
- LVGL version is **v8.4** — do not use v9 APIs
- Per-node power cap: `FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500)`
- PSRAM required for LVGL draw buffer — check `psramInit()` on boot
- All GPIO numbers come from `pinout.h` constants — never hardcoded inline
- FastLED minimum version: **3.7.7** (required for `.setRgbw()` on SK6812NW)

---

## Files Relevant to Today

[LIST THE FILES YOU WANT CLAUDE CODE TO FOCUS ON]

Examples:
- `src/drum_node/trigger.cpp` — ADC sampling and peak detection
- `src/drum_node/led_engine.cpp` — FastLED mode state machine
- `src/core_node/ui_dashboard.cpp` — LVGL Mission Control layout
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
