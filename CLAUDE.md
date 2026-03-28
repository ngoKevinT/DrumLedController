# ADLS — Claude Code Project Instructions
# Automated Drum Lighting System / Nexus PDU

This file is read automatically by Claude Code at the start of every session.
Do not delete or move it. Keep it updated as design decisions are locked in.

---

## 0. Orientation — Read These First

Before writing any code or suggesting any changes, read these files in order:

1. `manifest.md` — project constraints, agent protocol, active milestone
2. `docs/hardware/pinout.md` — every GPIO assignment for both nodes
3. `docs/hardware/power_specs.md` — power architecture and per-node limits
4. `docs/hardware/architecture.md` — system block diagram, firmware structure, confirmed decisions

If any of those files contradict something in this file, the other files win —
they are more detailed. This file is the quick-start summary only.

---

## 1. Hardware — Do Not Deviate

### Core Node
- **Board:** Waveshare ESP32-S3-Touch-LCD-7B
- **Display:** 1024×600 RGB panel (not 800×480, not 1280×800)
- **Touch:** GT911 — I2C on SDA: GPIO8, SCL: GPIO9
- **Backlight:** Controlled via CH32V003 I2C command (EXIO2). Cannot use digitalWrite. Requires I2C command sequence.
- **PSRAM:** OPI PSRAM — must be enabled in board settings. Mandatory for 1024×600 LVGL draw buffer.

### Drum Node
- **Board:** Seeed Studio XIAO ESP32-S3
- **LED data pin:** D9 (GPIO8) primary, D10 (GPIO9) future secondary — never D0
- **Trigger pins:** A0/D0 (GPIO1) ch1 TRS Tip, A1/D1 (GPIO2) ch2 TRS Ring, A2/D2 (GPIO3) ring detect — all left side
- **Status LEDs:** D8 (GPIO7) blue, D7 (GPIO44) red — right side, away from analog
- **Buttons:** D3 (GPIO4) test (5s hold = re-pair), D4 (GPIO5) mode — left side
- **Onboard LED:** GPIO21, active LOW — replaces discrete green LED
- **Power input:** 5V from local buck converter — never connect trunk voltage (14.8–20V) directly to XIAO

### LED Strip
- **Part:** SK6812NW (RGBW, Natural White ~4000K), 5V, 60 LED/m
- **FastLED init:**
  ```cpp
  FastLED.addLeds<SK6812, DATA_PIN, GRB>(leds, NUM_LEDS).setRgbw(RgbwDefault());
  ```
- **Minimum FastLED version:** 3.7.7 — `.setRgbw()` does not exist in earlier versions

---

## 2. Hard Constraints — Non-Negotiable

### Latency
- Hit-to-LED path must be **< 10ms**
- `delay()` is **banned everywhere** — use `millis()`, `micros()`, or FreeRTOS primitives
- Piezo sampling must run in a high-priority FreeRTOS task or ISR
- `FastLED.show()` must never block the trigger task

### Power
- Per-node 5V rail cap: **2500mA** in firmware at all times
  ```cpp
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500);
  ```
- Wall power mode (broadcast from Core Node): tighter cap — 2500mA per node
- Battery mode: cap may be relaxed up to 12000mA per node
- Never calculate power budgets based on a flat 5V/3A system supply — see `power_specs.md` for the full 14.8–20V trunk architecture

### Memory
- LVGL draw buffer **must** be allocated in PSRAM:
  ```cpp
  lv_color_t *buf = (lv_color_t *)heap_caps_malloc(
      1024 * 60 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM
  );
  ```
- No `new` / `delete` inside the main loop or any task that runs continuously
- No dynamic allocation for LED arrays — use static `CRGB leds[NUM_LEDS]`

---

## 3. Code Standards — Enforced

See `.agent/standards.md` for the full rules. Summary:

- `snake_case` for variables and functions
- `PascalCase` for classes and structs
- `constexpr` or `const` for all constants — never `#define` for values
- All GPIO numbers defined as `constexpr` in `pinout.h` — never hardcoded inline
- Comments explain **why**, not what: `// Advance to next LED in hit-streak` not `// i++`
- Any math involving trig, bitwise ops, or signal processing needs a first-principles comment above it

---

## 4. LVGL — Version 8.4 (Waveshare driver requirement)

The project uses **LVGL v8.4** — the version the Waveshare ESP32-S3-Touch-LCD-7B
drivers are built against. Do not suggest v9 APIs — the Waveshare driver layer
is not compatible with v9 and will fail to compile.

Key v8.4 APIs in use:
```cpp
// Display driver registration
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = 1024;
disp_drv.ver_res = 600;
disp_drv.flush_cb = my_disp_flush;
disp_drv.draw_buf = &draw_buf;
lv_disp_drv_register(&disp_drv);

// Draw buffer init
static lv_disp_draw_buf_t draw_buf;
lv_disp_draw_buf_init(&draw_buf, buf, NULL, buf_size);

// Touch input driver
static lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = my_touch_read;
lv_indev_drv_register(&indev_drv);
```

`lv_conf.h` settings for 1024×600:
- `LV_COLOR_DEPTH 16`
- `LV_MEM_SIZE (192 * 1024U)` minimum
- `LV_FONT_MONTSERRAT_24 1`, `LV_FONT_MONTSERRAT_32 1`, `LV_FONT_MONTSERRAT_48 1`
- `LV_FONT_DEFAULT &lv_font_montserrat_24`
- `LV_USE_PERF_MONITOR 1`, `LV_USE_MEM_MONITOR 1`

LVGL and FastLED must never share a task. Use a FreeRTOS queue to pass state
from the ESP-NOW receive callback to the LVGL task. Direct label updates from
an ISR or background task will cause a core panic.

---

## 5. ESP-NOW Architecture

- Core Node is **master** — broadcasts `DrumState` to registered node MACs
- Drum Nodes are **slaves** — receive config, send telemetry fire-and-forget
- Drum Nodes **never block** waiting for ACK
- Drum Nodes operate fully autonomously if Core Node is offline
- New nodes register with Core Node on first contact — MAC stored in NVS
- Heartbeat: Core Node polls all MACs every 2s, marks OFFLINE after 4s silence
- Watchdog: Drum Node auto-reboots after 10s without valid packet, reloads NVS state

### DrumState struct (downlink)
```cpp
struct DrumState {
    uint8_t  mode;        // LightingMode enum
    uint8_t  brightness;  // 0–255
    uint8_t  sensitivity; // trigger threshold
    uint8_t  tempC;       // node temp (uplink field, zero on downlink)
    uint8_t  r, g, b, w;  // RGBW color target
    bool     triggered;   // hit event flag
    uint16_t velocity;    // ADC peak (0–4095)
};
```

---

## 6. Pin Reference (Quick Lookup)

Full details in `docs/hardware/pinout.md`. Critical pins only:

### Drum Node (XIAO ESP32-S3)
| Pin | GPIO | Function |
|-----|------|----------|
| A0/D0 | 1  | Trigger ch1 — TRS Tip (piezo input, 10kΩ series + Zener + Schottky) |
| A1/D1 | 2  | Trigger ch2 — TRS Ring (second piezo or hi-hat choke) |
| A2/D2 | 3  | Ring detect — TRS normalling contact (HIGH=TS, LOW=TRS) |
| D3  | 4  | Test button (INPUT_PULLUP — 5s hold triggers re-pairing) |
| D4  | 5  | Mode button (INPUT_PULLUP — cycles lighting mode) |
| D5  | 6  | Reserved — NTC thermistor (Phase 7 thermal management) |
| D7  | 44 | Red LED — 80ms flash on trigger, solid on continuous noise |
| D8  | 7  | Blue LED — blink=searching, solid=paired, 500ms=re-pairing |
| D9  | 8  | SK6812NW data out primary (330Ω series + ESD diode) |
| D10 | 9  | SK6812NW data out secondary — future (330Ω series + ESD diode) |
| —   | 21 | Onboard LED — active LOW, confirms logic rail live |
| 5V  | —  | Power from local buck — NOT trunk voltage |

### Core Node (Waveshare 7B)
| Pin | Function |
|-----|----------|
| 7   | RGB PCLK |
| 46  | HSYNC |
| 3   | VSYNC |
| 5   | DE |
| 8/9 | GT911 touch SDA/SCL + CH32V003 backlight I2C |
| ADC | Bus voltage sense (100kΩ/10kΩ divider) |

---

## 7. Protection Components — Must Be Present on Drum Node PCB

These are confirmed locked-in hardware decisions. Do not omit them from any
schematic review or BOM generation:

| Component | Value | Placement | Purpose |
|-----------|-------|-----------|---------|
| TVS diode | P6KE24A | Across SP13 Pin 1/2 | Clamps hot-plug inductive spikes |
| Input cap | 1000µF 25V electrolytic | After TVS, before buck VIN | Anti-flicker reservoir |
| Decoupling cap | 100nF 0603 ceramic | ≤3mm from XIAO VCC pin | High-freq noise suppression |
| Piezo resistor | 10kΩ 1/4W | Series on A0 (ch1) and D1 (ch2) signal lines | Current limits piezo signal |
| Zener diode | 1N4728A 3.3V | Cathode→signal, Anode→GND (both trigger channels) | Clamps positive piezo spikes |
| Schottky diode | 1N5817 | Anode→GND, Cathode→signal (both trigger channels) | Clamps negative ringing |
| Data resistor | 330Ω 1/4W | Series on D9 (primary) and D10 (future), close to XIAO | Prevents signal ringing |
| ESD diode | PRTR5V0U2X | D9 and D10 data lines | ESD protection on LED data |

---

## 8. Active Project State

- **Current phase:** Phase 2 — Drum Node PCB design + Core Node power grid wiring
- **Board confirmed:** Waveshare ESP32-S3-Touch-LCD-7B (Core Node), Seeed XIAO ESP32-S3 (Drum Node — hardware in hand)
- **Active bottleneck:** Drum Node PCB schematic — dual-channel TRS trigger, D9/D10 LED data routing
- **Recently locked:** Full drum node GPIO assignments (see section 6), dual-channel TRS trigger, D9 primary LED data, left/right side EMI separation, GPIO21 onboard LED replaces discrete green LED

Update this section after completing each phase milestone.

---

## 9. Agent Handover Checklist

Before ending any working session, ensure:

- [ ] `todo.md` is updated with completed tasks checked off and any new technical debt noted
- [ ] Any new confirmed hardware or software decisions are added to `docs/hardware/architecture.md` under "Confirmed Design Decisions"
- [ ] Any new pin assignments are reflected in `docs/hardware/pinout.md`
- [ ] This `CLAUDE.md` is updated if any hard constraints changed
- [ ] Commit message describes what changed and why, not just what files were touched
