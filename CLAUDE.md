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
- **LED data pins:** D9 (GPIO8) drives ring 1 and D10 (GPIO9) drives ring 2. Both are active and sent in parallel. Never use D0.
- **Trigger pins:** A0/D0 (GPIO1) ch1 TRS Tip, A1/D1 (GPIO2) ch2 TRS Ring. Each has a load resistor to GND, so unplugged channels read about 0.
- **Buttons:** A2/D2 (GPIO3) test (5 s hold = re-pair), D3 (GPIO4) mode. Left side.
- **Status LEDs:** D5 (GPIO6) blue, D8 (GPIO7) red.
- **Reserved:** D4 (GPIO5) for the NTC. D6/D7 (GPIO43/44) for UART debug.
- **Onboard LED:** GPIO21, active LOW. Replaces the discrete green LED.
- **Power input:** 5V from the local buck (Pololu D36V50F5). Never connect trunk voltage (14.0–20.4V) directly to the XIAO.

### LED Strip
- **Part:** BTF-LIGHTING SK6812 RGBW (RGB + **Cool White** ~6000K), 5V, 60 LED/m, IP30
- **Per drum:** 2 rings. The largest drum (24") is about 115 LEDs per ring. Limit: **~120 LEDs per data line** (latency).
- **Driver:** ESP-IDF `led_strip` (SK6812 timing, GRBW format) behind our `led_hal` interface. **FastLED is not used.** See `docs/hardware/software/led_engine.md`.
- **Data level:** an SN74AHCT1G125 buffer powered from 5V on each data line. The SK6812 needs VIH 3.4V; the XIAO only drives 3.3V.

---

## 2. Hard Constraints — Non-Negotiable

### Latency
- Hit-to-LED path must be **< 10ms**
- Design target **≈ 6 ms**: 1 ms detection, 4.6 ms frame for 115 LEDs, and the in-flight frame is pre-empted on a hit
- `delay()` is **banned everywhere**. Use `esp_timer`, tick counts or FreeRTOS primitives.
- Piezo sampling: ADC continuous mode (DMA), ≥ 10 kHz per channel, feeding the highest-priority application task
- `led_hal_show()` must never block the trigger task

### Power
- **Derating policy** (`.agent/standards.md` §0): hardware runs at ≤ ~75% of its rating; software limits sit 15–25% below the hardware limit they protect.
- Per-node LED cap set by the firmware governor:
  - **2500 mA** on battery
  - **2000 mA** on wall power (provisional)
  - **1200 mA** on the interim QS-1205CME bench buck
  - **Never above 2500 mA.** Values received over ESP-NOW are clamped to this.
- The governor estimates current from post-RGBW values using calibrated per-channel mA plus a 10% margin. No mode or UI toggle can disable it.
- See `docs/hardware/power_specs.md` for the trunk architecture (14.0–20.4V) and margins.

### Memory
- LVGL draw buffer **must** be allocated in PSRAM:
  ```cpp
  lv_color_t *buf = (lv_color_t *)heap_caps_malloc(
      1024 * 60 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM
  );
  ```
- No `new` / `delete` inside the main loop or any task that runs continuously
- No dynamic allocation for LED frame buffers. Use static arrays sized for the maximum LEDs per line.

---

## 3. Code Standards — Enforced

See `.agent/standards.md` for the full rules. Summary:

- Firmware is **ESP-IDF C**. No Arduino or FastLED APIs.
- `snake_case` for variables and functions; `snake_case_t` for typedefs
- `static const` for values and `enum` for compile-time integers. Never `#define` a tunable value.
- All GPIO numbers are defined as `static const gpio_num_t` in `pinout.h`. Never hardcode them inline.
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

LVGL and the LED engine must never share a task. Use a FreeRTOS queue to pass state
from the ESP-NOW receive callback to the LVGL task. Direct label updates from
an ISR or background task will cause a core panic.

---

## 5. ESP-NOW Architecture

- Core Node is **master**. It unicasts `espnow_config_pkt_t` to registered node MACs.
- Drum Nodes are **slaves** — receive config, send telemetry fire-and-forget
- Drum Nodes **never block** waiting for ACK
- Drum Nodes operate fully autonomously if Core Node is offline
- New nodes register with Core Node on first contact — MAC stored in NVS
- Heartbeat: the Core Node polls every 1 s and marks a node OFFLINE after 3 misses (3.5 s). One lost packet must not flap the status.
- Watchdog: the ESP-IDF **task watchdog only**, so a node reboots only on a firmware hang. Losing Core Node packets means it runs standalone on NVS settings, with no reboot.

### Wire format, source of truth: `src/esp32/shared/espnow_transport/espnow_transport.h`
- `espnow_config_pkt_t` (core → drum): `mode_idx`, `color` (0x00RRGGBB), idle/flash brightness, flash/decay time, `threshold` (0–1023), `retrigger_ms`, `hit_curve`, `name`
- `espnow_telemetry_pkt_t` (drum → core): `raw_reading`, `velocity`
- `espnow_pair_req_pkt_t` / `espnow_pair_ack_pkt_t`: pairing
- Modes: `drum_mode_t` in `espnow_transport.h`. v1 is SOLID, PULSE, RAINBOW, CHASE, HEAT_MAP, CUSTOM, STEALTH. Append only, never renumber. Spec: `docs/hardware/software/modes_logic.md`.

---

## 6. Pin Reference (Quick Lookup)

Full details in `docs/hardware/pinout.md`. Critical pins only:

### Drum Node (XIAO ESP32-S3)
| Pin | GPIO | Function |
|-----|------|----------|
| A0/D0 | 1  | Trigger ch1, TRS Tip (load R + 10kΩ series + Zener + Schottky) |
| A1/D1 | 2  | Trigger ch2, TRS Ring (same chain; reads about 0 when unplugged) |
| A2/D2 | 3  | Test button (INPUT_PULLUP — 5s hold triggers re-pairing) |
| D3  | 4  | Mode button (INPUT_PULLUP — cycles lighting mode) |
| D4  | 5  | Reserved — NTC thermistor (Phase 7 thermal management) |
| D5  | 6  | Blue LED — blink=searching, solid=paired, 500ms=re-pairing |
| D6  | 43 | UART TX — free for debug (do not assign to peripherals) |
| D7  | 44 | UART RX — free for debug (do not assign to peripherals) |
| D8  | 7  | Red LED — 80ms flash on trigger, solid on continuous noise |
| D9  | 8  | SK6812 ring 1 data (AHCT1G125 → 330Ω → ESD) |
| D10 | 9  | SK6812 ring 2 data (AHCT1G125 → 330Ω → ESD) |
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
| ADC | Bus voltage sense. 150kΩ/10kΩ divider proposed; pin to confirm (GPIO6?). Core power is under revision. |

---

## 7. Protection Components — Must Be Present on Drum Node PCB

These are confirmed locked-in hardware decisions. Do not omit them from any
schematic review or BOM generation.

**PCB package strategy: hybrid SMD/through-hole.** Passives and small-signal diodes
are SMD. The P6KE27A TVS and the 1000µF input cap stay through-hole:
- The TVS needs the thermal mass of the DO-15 package to handle 600W transients reliably.
- The radial cap needs mechanical resilience (lead flex plus a silicone dab) for vibration on a drum rig.

Every part is sized to the derating policy (`.agent/standards.md` §0).

| Component | Part / Value | Package | Placement | Purpose |
|-----------|-------------|---------|-----------|---------|
| TVS diode | **P6KE27A** | **Through-hole DO-15** | Across SP13 Pin 1/2 | Clamps hot-plug spikes. Standoff 23.1V (20.4V = 88%); clamp 37.5V (75% of buck max) |
| Input cap | 1000µF **50V** electrolytic (35V min) | **Through-hole radial** | After TVS, before buck VIN | Anti-flicker reservoir; 20.4V = 41% |
| Buck | **Pololu D36V50F5** | Module | After input cap | 5V 5.5A, 50V in, reverse protection to 40V; 53% loaded at worst case |
| Decoupling caps | 100nF + 10µF X7R | SMD 0603 / 0805 | ≤3mm from XIAO 5V pin | High-frequency and local bulk decoupling |
| XIAO feed diode | B5819WS | SMD SOD-323 | Buck 5V → XIAO 5V pin | Lets USB and trunk power coexist safely (40% loaded) |
| LED output cap | 1000µF 16V | Radial | At the LED connector | Absorbs flash current steps |
| Piezo series resistors | 10kΩ | SMD 0805 | Series on A0 (ch1) and A1 (ch2) | Current-limits the piezo signal |
| Piezo load resistors | 1MΩ start (tune 100k–1M) | SMD 0805 | **Both** channels, signal to GND | Stops floating inputs; sets the hardest hit to about 2.5V at the pin |
| Zener diode | BZT52C3V3S 3.3V | SMD SOD-123 | Cathode→signal, Anode→GND (both trigger channels) | Clamps positive piezo spikes (replaces 1N4728A) |
| Schottky diode | B5817WS | SMD SOD-123 | Anode→GND, Cathode→signal (both trigger channels) | Clamps negative ringing (replaces 1N5817) |
| Data buffers | SN74AHCT1G125 ×2 | SMD SOT-23-5 | D9 and D10, powered from 5V | Shifts 3.3V data up to 5V logic (SK6812 VIH is 3.4V) |
| Data resistors | 330Ω | SMD 0805 | After each buffer | Prevents signal ringing |
| LED resistors | 220Ω | SMD 0805 | Series on D5 (blue) and D8 (red) | Current-limits status LEDs (red about 6mA; blue is dim, 100Ω optional) |
| Amber LED resistor | **4.7kΩ** | SMD **1206** | SP13 Pin 1 → amber LED → GND | Pre-buck indicator; 3.9mA, 72mW (29% of rating). The old 1kΩ 0805 dissipated 0.34W. |
| ESD diode | PRTR5V0U2X | SMD SOT-363 | Both data lines, at the connector | ESD protection on LED data |
| LED header | JST-VH 4-pin | Through-hole | PCB → TA4F panel socket | 10A per contact (25% at 2.5A) |
| Trace widths | ≥80 mil (5V/LED), ≥40 mil (trunk) | 1 oz Cu | n/a | 25 mil carries only about 1.7A at a 10°C rise |

---

## 8. Active Project State

- **Current phase:** Phase 2. Drum Node PCB design, plus drum node bench bring-up.
- **Board confirmed:** Waveshare ESP32-S3-Touch-LCD-7B (Core Node), Seeed XIAO ESP32-S3 (Drum Node, hardware in hand)
- **Bench setup:** SlimQ DC (measured 20.4V) → 5A blade fuse → single drum node with the interim QS-1205CME buck. LED cap 1200 mA. Firmware is running a bench blinky.
- **Core Node power:** **under revision.** The wall-only build failed and the review found under-rated parts (`docs/hardware/power_specs.md` §7).
- **Recently locked (2026-10-24):**
  - Hardware: D36V50F5 buck, P6KE27A TVS, 50V input cap, AHCT data buffers, JST-VH LED header, load resistors on both trigger channels, 4.7kΩ amber LED resistor.
  - Firmware: ESP-IDF plus `led_strip`/`led_hal`; task watchdog only.
  - Policy: derating (standards §0).

Update this section after completing each phase milestone.

---

## 9. Agent Handover Checklist

Before ending any working session, ensure:

- [ ] `todo.md` is updated with completed tasks checked off and any new technical debt noted
- [ ] Any new confirmed hardware or software decisions are added to `docs/hardware/architecture.md` under "Confirmed Design Decisions"
- [ ] Any new pin assignments are reflected in `docs/hardware/pinout.md`
- [ ] This `CLAUDE.md` is updated if any hard constraints changed
- [ ] Commit message describes what changed and why, not just what files were touched
