# System Architecture: ADLS / Nexus PDU

## 1. System Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        CORE NODE                                │
│                  Waveshare ESP32-S3-Touch-LCD-7B                │
│                                                                 │
│  ┌──────────────┐    ┌──────────────────────────────────────┐  │
│  │  SlimQ 150W  │    │   POWER GRID — UNDER REVISION         │  │
│  │  GaN (20.4V) │──▶│   (see power_specs.md §7)             │  │
│  └──────────────┘    │  Wall + battery OR-ing (ideal diodes) │  │
│                      │  → switch → main fuse → main bus      │  │
│  ┌──────────────┐    │  CC/CV → Daly 4S BMS → batteries      │  │
│  │ 2× Tattu 4S  │    │  Logic buck → 5V (display + meters)   │  │
│  │ 10Ah LiPo    │──▶│  6-way fuse block → 6× SP13 outputs   │  │
│  │  (parallel)  │    └───────────────────┬──────────────────┘  │
│  └──────────────┘                        │                      │
│  ┌─────────────────────────────────────┐ │                      │
│  │  LVGL v8.4 Command Center (1024×600)│ │  6× SP13 outputs     │
│  │  · 6 drum monitor cards             │ │  14.0–20.4V trunk    │
│  │  · Mode / color staging + sync      │ │  18AWG silicone      │
│  │  · Drum + global settings screens   │ │  Techflex sleeve     │
│  │  · Pairing management               │ │                      │
│  └─────────────────────────────────────┘ │                      │
└─────────────────────────────────────────┼───────────────────────┘
                                          │ (one cable per drum)
              ┌───────────────────────────┼──────────────────┐
              │                           │                  │
              ▼                           ▼                  ▼
   ┌─────────────────┐        ┌─────────────────┐       (×N more)
   │   DRUM NODE     │        │   DRUM NODE     │
   │ XIAO ESP32-S3   │        │ XIAO ESP32-S3   │
   │                 │        │                 │
   │ SP13 in (20V)   │        │ SP13 in (20V)   │
   │  → TVS P6KE27A  │        │  → TVS P6KE27A  │
   │  → 1000µF 50V   │        │  → 1000µF 50V   │
   │  → D36V50F5 5V  │        │  → D36V50F5 5V  │
   │  → 100nF+10µF   │        │  → 100nF+10µF   │
   │                 │        │                 │
   │ A0 ← piezo ch1  │        │ A0 ← piezo ch1  │
   │ A1 ← piezo ch2  │        │ A1 ← piezo ch2  │
   │  load R + 10kΩ  │        │  load R + 10kΩ  │
   │  Zener+Schottky │        │  Zener+Schottky │
   │                 │        │                 │
   │ D9/D10 → AHCT   │        │ D9/D10 → AHCT   │
   │  → 330Ω → ESD   │        │  → 330Ω → ESD   │
   │  → JST-VH       │        │  → JST-VH       │
   │  → TA4F → rings │        │  → TA4F → rings │
   │                 │        │                 │
   │ D5 → Blue LED   │        │ D5 → Blue LED   │
   │ D8 → Red LED    │        │ D8 → Red LED    │
   │ A2/D2 ← Test    │        │ A2/D2 ← Test    │
   │ D3 ← Mode btn   │        │ D3 ← Mode btn   │
   └─────────────────┘        └─────────────────┘
```

---

## 2. Communication Architecture

```
Core Node (ESP-NOW Master)
│
│  Downlink — espnow_config_pkt_t unicast (mode, color, brightness, timing, threshold, name)
│  Uplink   — espnow_telemetry_pkt_t fire-and-forget (raw reading, velocity)
│
├──▶ Drum Node: Kick
├──▶ Drum Node: Snare
├──▶ Drum Node: Hi-Hat
├──▶ Drum Node: Tom 1
├──▶ Drum Node: Tom 2
└──▶ Drum Node: Aux
```

**Protocol rules:**
- Drum Nodes never block waiting for an ACK. All uplink is fire-and-forget.
- Drum Nodes operate autonomously. Losing the Core Node connection does not stop LED operation.
- The Core Node stores the MAC → drum name mapping in NVS. New nodes register on first contact (PAIR_REQ / PAIR_ACK).
- Heartbeat: the Core Node polls every registered MAC every **1 s**. After **3 misses (3.5 s)** the dashboard shows OFFLINE.
- Watchdog: the ESP-IDF **task watchdog only**. A node reboots on a firmware hang, never because packets were lost. NVS state is reloaded on boot.
- Any value received over the radio is clamped to the drum node's local hard limits before use. For example, the LED cap never exceeds 2500 mA.

---

## 3. Firmware Architecture (ESP-IDF 5.5, C)

### Core Node
```
app_main()
├── espnow_transport_init()   — NVS, WiFi STA, ESP-NOW (before LVGL)
├── RGB panel + GT911 touch + IO expander backlight (vendor drivers)
├── lvgl_port_init()          — PSRAM draw buffer
├── SPIFFS mount → preload wallpapers/icons into PSRAM
├── ui_create()               — all screens built once, kept in memory
└── espnow recv cb → FreeRTOS queue → LVGL task (never call LVGL from the cb)

Tasks
├── LVGL task                 — lv_timer_handler(), holds port mutex
├── ESP-NOW / pairing task    — PAIR_ACK, config unicast, heartbeat poll (1 s)
└── bus voltage task          — ADC divider → dashboard (pending power revision)
```

### Drum Node
```
app_main()
├── espnow_transport_init()   — NVS, WiFi STA, ESP-NOW
├── NVS load → name, Core MAC, DrumConfig
├── led_hal_init()            — 2 RMT channels (D9 ring 1, D10 ring 2), SK6812 GRBW
├── adc_continuous init       — A0/A1, ≥10 kHz per channel, adc_cali
├── esp_task_wdt              — firmware-hang watchdog
└── start tasks

Tasks (priority high → low)
├── task_trigger              — ADC DMA frames → peak detect → notify LED engine
│                               (LEDs first; telemetry queued after)
├── task_led_engine           — modes → RGBW convert → governor → led_hal_show()
│                               (pre-empts in-flight frame on hit)
├── task_espnow_tx            — telemetry queue → esp_now_send (non-blocking)
├── on_recv (WiFi task ctx)   — PAIR_ACK / CONFIG → clamp → state + NVS
├── task_buttons              — A2/D2 test (5 s hold = re-pair), D3 mode
└── task_status_leds          — blue (pairing state), red (trigger), GPIO21 alive
```

---

## 4. Data Structures

**Source of truth: `src/esp32/shared/espnow_transport/espnow_transport.h`**. Don't duplicate the structs in docs.

| Packet | Direction | Key fields |
|---|---|---|
| `espnow_hdr_t` | all | `type`, `node_id` (0 = core, 1–6 = drums), `seq` |
| `espnow_config_pkt_t` | core → drum | `mode_idx`, `color` (0x00RRGGBB), `idle_brightness`, `flash_brightness`, `flash_time_ms`, `decay_time_ms`, `threshold` (0–1023), `retrigger_ms`, `hit_curve`, `name` |
| `espnow_telemetry_pkt_t` | drum → core | `raw_reading` (0–1023), `velocity` (0–255) |
| `espnow_pair_req_pkt_t` / `_ack_` | both | `name` |

* Drum-side live state: `DrumConfig` in `drum-node/main/main.c`, which mirrors the config packet.
* Core-side model: `ui_drum_t` in `components/ui/ui.h`.
* Lighting modes: `drum_mode_t` in `espnow_transport.h` (7 v1 modes); behavior is specified in `software/modes_logic.md`.
* **Planned telemetry additions:** `temp_c` (NTC) and the active power cap, for the dashboard.

---

## 5. Confirmed Design Decisions

These are locked. Don't reopen one without a documented reason.
Every hardware choice follows the derating policy in `.agent/standards.md` §0.

| Decision | Choice | Rationale |
|---|---|---|
| Firmware toolchain | ESP-IDF 5.5, C | Already running the ESP-NOW and LVGL code; no Arduino layer |
| LED driver | Espressif `led_strip` (RMT/DMA) behind our own `led_hal` | Official and maintained. FastLED's ESP-IDF 5 driver wraps `led_strip` anyway, so there's no speed gain. `led_hal` makes the strip type swappable in one file. |
| LED strip | BTF-LIGHTING SK6812 RGBW (Cool White), 60 LED/m | Dedicated white LED; cool white sits near the RGB white point, so white extraction is clean |
| LEDs per data line | ≤ ~120 (largest drum: 115 per ring, 2 rings) | 40 µs per RGBW LED. Keeps the frame at ≤ 4.8 ms, inside the latency budget. |
| LED data pins | D9 (GPIO8) ring 1, D10 (GPIO9) ring 2. Both active, sent in parallel. | RMT/DMA capable; avoids SPI/I2C conflicts |
| Hit frame pre-emption | `led_hal_abort()` cancels the in-flight background frame on a hit | Without it the worst case is ≈ 10.2 ms, over budget |
| Data level shifting | SN74AHCT1G125 per line, powered from 5V | SK6812 VIH is 3.4V; a 3.3V GPIO has no margin. AHCT VIH is 2.0V. |
| Drum buck | Pololu D36V50F5 (5V, 5.5A, 6–50V in, reverse protection to 40V) | 53% loaded at 2.92A; 50V input is above the 37.5V TVS clamp |
| Trunk voltage | 14.0–20.4V | Thin 18AWG cables; buck headroom; no color shift on long runs |
| SP13 protection | P6KE27A TVS (through-hole DO-15) | Standoff 23.1V (20.4V = 88%); clamp 37.5V. Through-hole for thermal mass at the 600W rating. |
| Input reservoir | 1000µF **50V** radial electrolytic (35V minimum) | 20.4V = 41%; through-hole leads flex under vibration (silicone dab) |
| Trigger front end | Load resistor on both channels + 10kΩ series + BZT52C3V3S + B5817WS | Load R stops floating inputs and sets the hardest hit to about 2.5V; the clamps act only during faults |
| Drum Node PCB strategy | Hybrid SMD/through-hole | Passives and small-signal diodes SMD; TVS and input cap through-hole |
| XIAO decoupling | 100nF 0603 + 10µF X7R, ≤ 3mm from the 5V pin | Prevents ADC noise, brown-outs and ESP-NOW corruption under load |
| XIAO feed diode | B5819WS from buck 5V to XIAO 5V | USB debugging with trunk power connected can't back-feed |
| LED header (PCB) | JST-VH 4-pin (5V, GND, D1, D2) | 10A per contact, 25% at 2.5A. JST-XH (3A) would run at 83%. |
| LED bus connector | TA4F panel socket (5A carry rating, 50% load) | Check current part numbers; Digi-Key lists the TA4F cable plug as obsolete (TA4FX replaces it) |
| Amber LED resistor | 4.7kΩ 1206 | 72mW, 29% of rating. The original 1kΩ 0805 dissipated 0.34W, 270% of rating. |
| Watchdog | Task watchdog only | Packet-loss reboot conflicted with standalone mode |
| Lighting modes (v1) | SOLID, PULSE, RAINBOW, CHASE, HEAT_MAP, CUSTOM, STEALTH (`drum_mode_t`) | Trimmed to distinct behaviors; other looks are settings presets; the rest are in the v2 backlog |
| Strobe rate | ≤ 3 Hz by default; faster only with an explicit UI opt-in | Photosensitive-seizure risk at 3–30 Hz |
| Sensitivity control | Software via ESP-NOW | Central control from the Core Node UI; saved in NVS on each node |
| Local power switch | None on Drum Nodes | The Core Node is the only kill switch, which prevents an accidental mid-show shutoff |
| Trunk cable | 18AWG silicone + Techflex Clean-Cut | Vibration-resistant, flexible, field-repairable; about 12% loaded |
| Core Node board | Waveshare ESP32-S3-Touch-LCD-7B | 1024×600, GT911 touch, integrated |
| Drum Node board | Seeed Studio XIAO ESP32-S3 | Smallest ESP32-S3 form factor; enough I/O; USB-C |
| Case | 1/2" Baltic Birch + 1/4" Lexan | DIY-friendly; impact-resistant; mounts to the drum stand |

**Under revision (not locked):** the Core Node power grid (ideal-diode OR-ing, UPS charging topology, fuses, switch). See `power_specs.md` §7.

---

## 6. Open Items

| Item | Status |
|---|---|
| D36V50F5 fit (25.4 × 25.4 mm) inside the ~55 mm PETG puck | Mechanical check pending |
| 50V vs 35V input cap: physical size in the puck | Use 50V if it fits, otherwise 35V minimum |
| Piezo load resistor value | Tune on the bench: hardest hit ≈ 2.5V at the pin |
| SK6812 per-channel current calibration | Measure on the bench and put the values into governor constants |
| `led_strip` abort support for frame pre-emption | Spike it. If it isn't supported, `led_hal` uses a custom RMT encoder. |
| Mode button favorites | Needs a `favorites` field in `espnow_config_pkt_t` and a picker in the Core UI |
| Core Node power grid | Under revision (`power_specs.md` §7) |
