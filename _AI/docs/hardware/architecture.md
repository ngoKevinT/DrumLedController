# System Architecture: ADLS / Nexus PDU

## 1. System Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        CORE NODE                                │
│                  Waveshare ESP32-S3-Touch-LCD-7B                │
│                                                                 │
│  ┌──────────────┐    ┌──────────────────────────────────────┐  │
│  │  SlimQ 150W  │    │           POWER GRID                 │  │
│  │  GaN (20V)   │──▶│  Ideal Diode #1 (wall path)          │  │
│  └──────────────┘    │  Ideal Diode #2 (battery path)       │  │
│                      │  OR splice → 24V switch → 40A fuse   │  │
│  ┌──────────────┐    │  → Main VCC Bus (14.8–20V)           │  │
│  │ 2× Tattu 4S  │    │                                      │  │
│  │ 10Ah LiPo    │    │  CC/CV 300W → Daly 4S BMS → batts   │  │
│  │  (parallel)  │──▶│  Logic 3A buck → 5V (display+meters) │  │
│  └──────────────┘    │  Nilight 6-way fuse block (10A each) │  │
│                      └───────────────────┬──────────────────┘  │
│  ┌─────────────────────────────────────┐ │                      │
│  │  LVGL v8.4 Mission Control (1024×600)│ │  6× SP13 outputs     │
│  │  · Per-drum status cards            │ │  14.8–20V trunk      │
│  │  · Sensitivity sliders              │ │  18AWG silicone      │
│  │  · Color palette picker             │ │  Techflex sleeve     │
│  │  · Battery gauge + wall indicator   │ │                      │
│  │  · Per-channel voltmeters           │ │                      │
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
   │  → TVS P6KE24A  │        │  → TVS P6KE24A  │
   │  → 1000µF cap   │        │  → 1000µF cap   │
   │  → 5V 3A buck   │        │  → 5V 3A buck   │
   │  → 100nF VCC    │        │  → 100nF VCC    │
   │                 │        │                 │
   │ A0 ← piezo      │        │ A0 ← piezo      │
   │   10kΩ + Zener  │        │   10kΩ + Zener  │
   │   + Schottky    │        │   + Schottky    │
   │                 │        │                 │
   │ D0 → 330Ω       │        │ D0 → 330Ω       │
   │   → ESD diode   │        │   → ESD diode   │
   │   → SK6812NW    │        │   → SK6812NW    │
   │     (JST-XH)    │        │     (JST-XH)    │
   │                 │        │                 │
   │ D1 → Blue LED   │        │ D1 → Blue LED   │
   │ D2 → Red LED    │        │ D2 → Red LED    │
   │ D4 ← Test btn   │        │ D4 ← Test btn   │
   │ D5 ← Mode btn   │        │ D5 ← Mode btn   │
   └─────────────────┘        └─────────────────┘
```

---

## 2. Communication Architecture

```
Core Node (ESP-NOW Master)
│
│  Downlink — DrumState broadcast (color, mode, sensitivity, brightness limit)
│  Uplink   — Hit notification + telemetry (tempC, velocity, online status)
│
├──▶ Drum Node: Kick
├──▶ Drum Node: Snare
├──▶ Drum Node: Hi-Hat
├──▶ Drum Node: Tom 1
├──▶ Drum Node: Tom 2
└──▶ Drum Node: Aux
```

**Protocol rules:**
- Drum Nodes never block waiting for ACK. All uplink is fire-and-forget.
- Drum Nodes operate autonomously. Loss of Core Node connection does not stop LED operation.
- Core Node stores MAC → drum name mapping in NVS. New nodes are registered on first contact.
- Heartbeat: Core Node polls all registered MACs every 2s. No response in 4s → OFFLINE badge on dashboard.
- Watchdog: Each Drum Node auto-reboots after 10s without a valid packet. NVS state is reloaded on boot.

---

## 3. Firmware Architecture

### Core Node
```
setup()
├── psramInit() — halt if unavailable
├── Arduino_GFX init (RGB panel, Waveshare 7B timings)
├── CH32V003 I2C → enable backlight (EXIO2)
├── GT911 touch driver init (SDA: GPIO8, SCL: GPIO9)
├── lv_init() + PSRAM draw buffer allocation
├── ESP-NOW init → register receive callback
├── NVS load → restore node MAC mappings
└── lv_scr_load(screen_splash)

loop()
├── lv_timer_handler()          — LVGL render tick
├── process_espnow_rx_queue()   — thread-safe bridge from ISR to LVGL
├── heartbeat_check()           — poll registered nodes every 2s
├── adc_read_bus_voltage()      — update dashboard battery/wall indicator
└── (all heavy logic in FreeRTOS tasks, not loop())
```

### Drum Node
```
setup()
├── FastLED.addLeds<SK6812, D0, GRB>().setRgbw(RgbwDefault())
├── FastLED.setBrightness(255)
├── ESP-NOW init → register receive callback
├── NVS load → restore last sensitivity, mode, color
├── pinMode(D4, INPUT_PULLUP)   — test button
├── pinMode(D5, INPUT_PULLUP)   — mode button
└── start FreeRTOS tasks

FreeRTOS tasks
├── task_trigger (HIGH priority)
│   └── Sample ADC1 A0 at >1kHz → peak detect → fire LEDs → notify Core
├── task_led_engine (MEDIUM priority)
│   └── Run active lighting mode → FastLED.show()
├── task_espnow_rx (MEDIUM priority)
│   └── Receive DrumState → update local state → NVS save if changed
└── task_buttons (LOW priority)
    └── Poll D4/D5 → debounce → cycle mode or fire test trigger
```

---

## 4. Data Structures

```cpp
// Transmitted Core Node → Drum Node (downlink)
struct DrumState {
    uint8_t  mode;        // lighting mode index (see LightingMode enum)
    uint8_t  brightness;  // 0–255 global brightness cap
    uint8_t  sensitivity; // ADC trigger threshold (0–255 mapped to ADC range)
    uint8_t  r, g, b, w;  // RGBW color target for current palette
};

// Transmitted Drum Node → Core Node (uplink, fire-and-forget)
struct DrumTelemetry {
    uint8_t  tempC;       // buck converter NTC reading (if fitted)
    uint16_t velocity;    // ADC peak value of last hit (0–4095)
    bool     online;      // heartbeat flag
};

// Lighting modes
enum LightingMode : uint8_t {
    STATIC_GLOW      = 0,
    TRIGGER_FLASH    = 1,
    VELOCITY_FADE    = 2,
    RAINBOW_DYNAMIC  = 3,
    RAINBOW_STATIC   = 4,
    RAVE_90S         = 5,
    GHOST_NOTE       = 6,
    HEAT_MAP         = 7,
    STEALTH          = 8,
};
```

---

## 5. Confirmed Design Decisions

These are locked. Do not re-open without a documented reason.

| Decision | Choice | Rationale |
|---|---|---|
| LED strip | SK6812NW 60 LED/m | Native RGBW — dedicated white diode, FastLED ≥3.7.7 `.setRgbw()` support |
| Transport voltage | 14.8–20V trunk | Thin 18AWG cables; buck headroom; no color shift on long runs |
| Power OR-ing | Active ideal diode modules | Near-zero drop vs Schottky's 0.6V × 20A = 12W heat; no heatsink needed |
| Charging topology | UPS (CC/CV + common-port BMS) | System runs while charging; seamless battery/wall switchover |
| Core Node board | Waveshare ESP32-S3-Touch-LCD-7B | 1024×600, GT911 touch, integrated — no wiring between MCU and display |
| Drum Node board | Seeed Studio XIAO ESP32-S3 | Smallest ESP32-S3 form factor; sufficient I/O; 5V pin; USB-C |
| LED data pin | D0 | RMT/DMA capable; avoids SPI/I2C conflicts |
| LED connector | JST-XH 3-pin (or 4-pin) | 3A/contact; field-replaceable strips without soldering iron |
| SP13 protection | P6KE24A TVS diode | Clamps hot-plug inductive spikes before they reach buck VIN |
| XIAO decoupling | 100nF 0603 ceramic ≤3mm from VCC | Prevents ADC noise, brown-out resets, ESP-NOW corruption under load |
| Sensitivity control | Software via ESP-NOW | Central control from Core Node UI; persisted in NVS on each node |
| Local power switch | None on Drum Nodes | Core Node is sole kill switch — prevents accidental mid-show shutoff |
| Trunk cable | 18AWG silicone + Techflex Clean-Cut | Vibration-resistant; flexible; field-repairable |
| Case | 1/2" Baltic Birch + 1/4" Lexan | DIY-friendly; impact-resistant (Lexan vs acrylic); mounts to drum stand |
