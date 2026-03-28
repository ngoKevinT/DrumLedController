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
- **Drum Node:** Seeed Studio XIAO ESP32-S3 · EasyEDA Pro v2.2.45.4
- **Active phase:** [REPLACE WITH CURRENT PHASE, e.g. "Phase 2 — Drum Node PCB schematic"]
- **Working on today:** [REPLACE WITH TASK, e.g. "Drum Node PCB layout — place and route power path"]

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
- LED Data 1 pin is **D9/GPIO8** — not D0, not any other pin
- LED Data 2 pin is **D10/GPIO9** — future use, NC in current build
- Trigger ch1 (Tip) ADC is **A0/GPIO1** — always active for TS and TRS
- Trigger ch2 (Ring) ADC is **D3/GPIO4** — only sampled in TRS stereo mode
- Ring detect is **D8/GPIO7** — internal pull-up · LOW = TS mono · HIGH = TRS stereo
- LVGL version is **v8.4** — do not use v9 APIs
- Per-node power cap: `FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500)`
- PSRAM required for LVGL draw buffer — check `psramInit()` on boot
- All GPIO numbers come from `pinout.h` constants — never hardcoded inline
- FastLED minimum version: **3.7.7** (required for `.setRgbw()` on SK6812NW)
- Buck module (QS-1205CME-3A) 5V pad must be bridged BEFORE soldering to PCB
- PRTR5V0U2X pin assignment: Pin1=GND · Pin2=IO1(in) · Pin3=IO2(out) · Pin4=VCC
- C2 100nF decoupling cap has NO polarity — ceramic cap
- P6KE24A TVS: cathode (banded end) → VIN_RAW · anode → GND
- Power trace width ≥ 1.0mm (40mil) · signal ≥ 0.3mm (12mil) · clearance ≥ 0.5mm
- Onboard user LED is **GPIO21, active LOW** — `digitalWrite(LED_BUILTIN, LOW)` = ON · no external component
- Blue LED state machine: pairing blink = 500ms · paired = solid · offline = 1000ms blink
- Red LED: flashes 80ms on any trigger detection (ch1 or ch2) · stays solid if continuous noise above threshold
- Test button hold ≥ 5s → pairing mode · short press → manual trigger · use `millis()` — no blocking
- TRS jack must be **switched type** with normalling contact — not a basic unswitched stereo jack

---

## Drum Node Pin Assignments (XIAO ESP32-S3)

| Arduino | GPIO | Function |
|---|---|---|
| A0 | GPIO1 | Trigger ch1 — Tip — TS and TRS |
| D3 | GPIO4 | Trigger ch2 — Ring — TRS only |
| D8 | GPIO7 | Ring detect — TRS vs TS sense |
| D1 | GPIO2 | Blue status LED |
| D2 | GPIO3 | Red trigger LED |
| D4 | GPIO5 | Test button (internal pull-up) |
| D5 | GPIO6 | Mode button (internal pull-up) |
| D9 | GPIO8 | LED Data 1 — SK6812NW DIN |
| D10 | GPIO9 | LED Data 2 — future use |
| GPIO21 | internal | Onboard user LED — active LOW — 5V live |
| D6/D7 | GPIO43/44 | Spare — NC |

---

## Key LCSC Part Numbers (Drum Node)

| Part | LCSC |
|---|---|
| XIAO ESP32-S3 | C9900154951 |
| PRTR5V0U2X ESD (×2) | C12333 |
| 1000µF 25V electrolytic | C10750 |
| 100nF 0805 ceramic (no polarity) | C49678 |
| 1N4728A 3.3V Zener (×2) | C58985 |
| 1N5817 Schottky (×2) | C8598 |
| 10kΩ 1/4W TH resistor (×2) | C57438 |
| 330Ω 1/4W TH resistor (×2) | C57436 |
| 220Ω 1/4W TH resistor (×2) | C57435 |
| 1kΩ 1/4W TH resistor | C57437 |

Non-LCSC parts: P6KE24A (DigiKey/Mouser) · QS-1205CME-3A buck module (QSKJ/AliExpress)

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
