# Project Mission Log: ADLS Development
# Automated Drum Lighting System — Nexus PDU

---

## Hardware Inventory

### Core Node
| Component | Part | Spec |
|---|---|---|
| Display / MCU | Waveshare ESP32-S3-Touch-LCD-7B | 1024×600, capacitive touch, GT911 |
| Battery | 2× Tattu 10Ah 4S LiPo | 14.8V nominal, 20Ah parallel |
| Ideal diode #1 | 50A Ideal Diode Module | Wall path OR-ing — GND pad → star ground (22AWG) |
| Ideal diode #2 | 50A Ideal Diode Module | Battery path OR-ing — GND pad → star ground (22AWG) |
| Charge controller | 300W 20A CC/CV Buck Module | Set to 16.8V / 5A → Daly BMS → batteries |
| BMS | Daly 4S 40A Common-Port BMS | P− to star ground · B− to batteries |
| Power switch | 24V Illuminated Rocker/Toggle | 20A rated · switch LED GND → star ground |
| Main fuse | 40A Automotive Blade Fuse | Inline after switch |
| Distribution | Nilight 6-Way Fuse Block | 10A blade fuse per channel |
| Logic buck | Mini 5V 3A Buck (solder pad) | Protected · fixed output · powers display + voltmeters |
| Voltmeters | 6× 0.28" 3-Wire Mini Voltmeter | Sense wire per SP13 output channel |
| Wall voltmeter | 0.28" 3-Wire Mini Voltmeter | Sense wire on wall input line |
| Battery gauge | 4S Capacity Indicator (LED bar) | Taps parallel battery positive |
| Charger input | SlimQ 150W GaN | 20V DC out → Ideal Diode #1 VIN |
| Output connectors | 6× SP13 Panel-Mount 2-pin | Pin 1: 14.8–20V · Pin 2: GND |
| Bus wire | 12AWG Silicone (red/black) | High-current trunk lines |
| Chassis | 1/2" Baltic Birch + 1/4" Lexan | Clamshell road case, table-saw lid cut |
| Mount | Roland APC-33 / Gibraltar SC-EMMP | Drum stand clamp via internal spine |
| Fasteners | M3 Nylon lock nuts + fender washers | For mount plate through Lexan floor |
| Case hardware | Piano hinge + butterfly latches | Lid and storage bay |
| VCC bus cap | 100µF 25V electrolytic | Across main VCC bus output — absorbs switch-on transient |
| Reverse polarity | P-channel MOSFET | On battery positive line — protects against reversed battery |
| Bus voltage sense | 100kΩ / 10kΩ voltage divider | → ESP32 ADC pin — displays bus voltage on LVGL dashboard |

### Drum Node (× number of drums)
| Component | Part | Spec |
|---|---|---|
| MCU | Seeed Studio XIAO ESP32-S3 | Dual-core · 5V pin · D9/D10 LED data · A0/A1(D1) trigger · D5/D8 status LEDs · A2(D2)/D3 buttons · D6(TX)/D7(RX) free |
| LED strip | SK6812NW 60 LED/m | 5V · RGBW · Natural White ~4000K |
| Input connector | SP13 90° 2-pin | Pin 1: 14.8–20V in · Pin 2: GND |
| LED connector | 4× direct solder pads at board edge | 5V · GND · Data1 · Data2 |
| Input cap | 1000µF 25V Electrolytic | Across SP13 input — anti-flicker reservoir |
| TVS diode | P6KE24A | Across SP13 Pin 1/2 — clamps hot-plug inductive spikes to 24V |
| Buck converter | Mini 5V 3A Buck (solder pad) | Fixed output · taps after TVS · powers strip + XIAO |
| Decoupling cap | 100nF 0805 Ceramic | Directly adjacent to XIAO VCC pin — high-freq decoupling |
| Piezo resistor | 10kΩ 1/4W | Series · current limits piezo signal |
| Zener diode | 1N4728A 3.3V | Cathode → signal · Anode → GND · clamps positive spikes |
| Schottky diode | 1N5817 | Anode → GND · Cathode → signal · clamps negative ringing |
| Data resistor | 330Ω 1/4W | Series on D9 → LED DIN · prevents signal ringing (D10 future) |
| ESD diode | PRTR5V0U2X | On D9/D10 data lines — ESD protection |
| Trigger jack | 1/4" TRS Panel Mount | Dual-channel piezo input · standard 3-lug (tip/ring/sleeve), no normalling needed |
| Ch2 pull-down | 100kΩ 1/4W | After clamp diodes on A1/D1 — pulls to GND when no TRS plug present · software ring detect |
| LED amber | 5mm Amber + 1kΩ | Hardwired to high rail (pre-buck) — SP13 power present |
| LED blue | 5mm Blue + 220Ω | XIAO D5 — blink=searching · solid=connected · 500ms blink=re-pairing |
| LED red | 5mm Red + 220Ω | XIAO D8 — 80ms flash on trigger · solid during continuous noise |
| LED green (onboard) | XIAO onboard LED (GPIO21) | Active LOW — replaces external green LED · logic rail live |
| LED holders | 5mm Plastic Socket × 3 | Panel mount (amber, blue, red — green is onboard) |
| Test button | Gebildet 7mm SPST NO | XIAO A2/D2 — fires trigger manually · 5s hold → re-pairing mode |
| Mode button | Gebildet 7mm SPST NO | XIAO D3 — cycles local color mode |
| Internal wire | 18AWG High-Strand Silicone | All internal connections — vibration resistant |
| Trunk cable | 18AWG Silicone + Techflex Clean-Cut sleeve | Core Node → drum SP13 runs |
| PCB mount | 4× M2 TPU standoffs | Isolates PCB from drum shell vibration |
| Enclosure | 3D printed PETG | Hockey-puck form · ~55×25mm · honeycomb vents |

---

## Phase 1: Mechanical Fabrication

- [ ] **Core Node road case**
  - [ ] Mill 1/2" Baltic Birch into sealed 6-sided box
  - [ ] Table-saw split to create matched lid (1.5–2" slice)
  - [ ] Route 1/4" rabbet into lid frame for Lexan inlay
  - [ ] Cut Lexan faceplate — flush-mount 7" display cutout + 8× voltmeter slots
  - [ ] Install internal partition wall (Electronics Bay vs Storage Bay)
  - [ ] Glue and screw 3" plywood spine across floor for mount plate anchor
  - [ ] Drill ventilation holes — rear wall and bottom edge for convection
  - [ ] Install Roland APC-33 / Gibraltar plate through Lexan floor + spine (fender washers + nylon lock nuts)
  - [ ] Install piano hinge (full-width, rear) and butterfly latches (front × 2)
  - [ ] Attach adhesive foam weatherstripping to lid cut edge

- [ ] **Core Node internal layout**
  - [ ] Mount aluminum heatsink/spine for Ideal Diode modules and buck converters
  - [ ] Install Sil-Pads between diode tabs and heatsink
  - [ ] Locate batteries on floor for low center of gravity
  - [ ] Install 30–40mm fan with honeycomb exhaust mesh

- [ ] **Drum Node enclosures**
  - [ ] Design 3D-printable PETG hockey-puck enclosure (~55×25mm)
  - [ ] Include SP13 90° mount port, TRS jack cutout, 3× LED holes, 2× button holes
  - [ ] Add honeycomb ventilation on top and bottom faces
  - [ ] Print M2 TPU standoffs for PCB isolation

---

## Phase 2: Power Grid & PCB Design

### Core Node wiring
- [ ] Solder parallel harness for 2× Tattu batteries (verify voltages within 0.1V before connecting)
- [ ] Install P6KE24A-equivalent TVS on battery input line
- [ ] Wire Ideal Diode #1 (wall path): SlimQ → VIN, VOUT → OR splice, GND pad → star ground (22AWG)
- [ ] Wire Ideal Diode #2 (battery path): parallel batt+ → VIN, VOUT → OR splice, GND pad → star ground (22AWG)
- [ ] Wire CC/CV module: OR splice → IN(+), OUT(+) → Daly BMS B+ / battery splice
- [ ] Calibrate CC/CV: set CV to 16.80V, CC to 5.00A (bench test before connecting batteries)
- [ ] Install 24V illuminated switch: PIN 1 ← OR splice, PIN 2 → 40A fuse, PIN 3 (LED GND) → star ground
- [ ] Install 40A inline blade fuse after switch
- [ ] Install 100µF 25V cap across main VCC bus output
- [ ] Install reverse polarity P-ch MOSFET on battery positive line
- [ ] Install 100kΩ / 10kΩ voltage divider: main bus → ESP32 ADC pin (for LVGL dashboard readout)
- [ ] Wire logic 3A buck: main VCC bus → IN, 5V OUT → Waveshare 7B + all 6× voltmeter RED wires
- [ ] Wire Nilight fuse block: main VCC bus stud → input, 10A blade per channel
- [ ] Wire 6× SP13 outputs: fuse block terminal + voltmeter YELLOW sense wire per channel
- [ ] Star ground bus bar: wall (−), batt (−) via BMS P−, diode GND pads × 2, CC/CV GND, buck GND in/out, display GND, 6× SP13 Pin 2, switch LED GND, all voltmeter BLACK wires
- [ ] Daly BMS wiring: P− → star ground, B− → parallel battery negative junction, balance harness to cell junctions

### Drum Node PCB (EasyEDA / KiCad / Flux AI)
- [ ] Import XIAO ESP32-S3 footprint (LCSC: C9900124959)
- [ ] Draw schematic:
  - SP13 Pin 1 → TVS P6KE24A (across to GND) → 1000µF cap (across to GND) → buck VIN
  - Buck VOUT (5V) → 100nF 0805 ceramic cap (across to GND, placed ≤3mm from XIAO VCC)
  - 5V rail → XIAO VCC pin, SK6812NW VCC, LED indicators (via resistors)
  - Trigger ch1: ¼" TRS Tip → 10kΩ series → 1N4728A Zener (C→signal, A→GND) → 1N5817 Schottky (A→GND, C→signal) → XIAO A0 (GPIO1)
  - Trigger ch2: ¼" TRS Ring → same protection chain → 100kΩ pull-down to GND → XIAO A1/D1 (GPIO2) — software ring detect
  - LED data 1: XIAO D9 (GPIO8) → 330Ω → PRTR5V0U2X ESD → LED bus Data1 pad
  - LED data 2: XIAO D10 (GPIO9) → 330Ω → PRTR5V0U2X ESD → LED bus Data2 pad (future)
  - XIAO D5 → 220Ω → Blue LED → GND
  - XIAO D8 → 220Ω → Red LED → GND
  - XIAO A2/D2 ← Test button → GND (internal pull-up) · 5s hold → re-pairing mode
  - XIAO D3 ← Mode button → GND (internal pull-up)
  - Amber LED: tap pre-buck VCC → 1kΩ → LED → GND
  - Green LED: eliminated — use XIAO onboard LED (GPIO21, active LOW)
- [ ] Route power traces ≥ 25mil for 5V and GND
- [ ] Add copper ground plane both sides
- [ ] Add teardrops on all pads (vibration hardening)
- [ ] Add 4× M2 mounting holes at corners
- [ ] Verify no traces narrower than 12mil for signal lines
- [ ] Run ERC and DRC — zero errors before ordering
- [ ] Order PCBs (suggest 5-pack minimum for spares)
- [ ] Order components: see Hardware Inventory table above

---

## Phase 3: Core Node Display & UI (1024×600)

- [ ] **LVGL v8.4 environment**
  - [ ] Configure `lv_conf.h`:
    - `#if 1` master enable
    - `LV_COLOR_DEPTH 16`
    - `LV_MEM_SIZE (192 * 1024U)` — increased for 1024×600
    - `LV_DPI_DEF 170`
    - `LV_FONT_MONTSERRAT_24 1`, `LV_FONT_MONTSERRAT_32 1`, `LV_FONT_MONTSERRAT_48 1`
    - `LV_FONT_DEFAULT &lv_font_montserrat_24`
    - `LV_USE_PERF_MONITOR 1`, `LV_USE_MEM_MONITOR 1`
  - [ ] Verify PSRAM enabled in board settings (OPI PSRAM)
  - [ ] Allocate draw buffer in PSRAM: `heap_caps_malloc(1024 * 60 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM)`
  - [ ] Implement `psramInit()` check on boot — halt if PSRAM unavailable
  - [ ] Configure Arduino_GFX RGB panel driver for Waveshare 7B pin mapping
  - [ ] Implement CH32V003 I2C command to enable backlight (EXIO2, GPIO8/9)
  - [ ] Configure GT911 capacitive touch driver (SDA: GPIO8, SCL: GPIO9)
  - [ ] Add VS Code include path: `X:/Users/.../Arduino/libraries/**`

- [ ] **Mission Control dashboard**
  - [ ] Splash screen with "NEXUS PDU" label and connection progress bar
  - [ ] 6-drum status card grid — per card: drum name, trigger indicator, voltage, temp, mode
  - [ ] Battery gauge (bar + percentage from 4S capacity indicator or ADC divider)
  - [ ] Wall power indicator (green = connected, gray = battery only)
  - [ ] Bus voltage readout from ADC voltage divider
  - [ ] Per-drum sensitivity slider (transmits value via ESP-NOW on release)
  - [ ] Global color palette picker (transmits to all nodes simultaneously)
  - [ ] Enable Montserrat 32/48 for primary readouts — legible from behind kit

---

## Phase 4: Communication & Discovery (ESP-NOW)

- [ ] **Data structure**
  ```cpp
  struct DrumState {
      uint8_t  mode;          // lighting mode index
      uint8_t  brightness;    // 0–255 global
      uint8_t  sensitivity;   // trigger threshold
      uint8_t  tempC;         // node temperature
      uint8_t  r, g, b, w;   // RGBW color target
      bool     triggered;     // hit event flag
      uint16_t velocity;      // ADC peak value of last hit
  };
  ```

- [ ] **Core Node (Master)**
  - [ ] Initialize ESP-NOW on boot
  - [ ] Implement "Add Node" listener — register drum nodes by MAC address on first contact
  - [ ] Store MAC → drum name mapping in NVS
  - [ ] Broadcast `DrumState` to all registered MACs on setting change
  - [ ] Implement "Identity Flash" — ping single node → node flashes white 3× for physical identification
  - [ ] Heartbeat check every 2s — mark node OFFLINE on dashboard if no response
  - [ ] Detect wall vs battery mode via ADC divider → broadcast brightness limit if on wall power

- [ ] **Drum Node (Slave)**
  - [ ] Broadcast pairing packet on boot if no Core Node MAC stored in NVS
  - [ ] Blue LED: blink 500ms interval while searching, solid when paired
  - [ ] Receive `DrumState` packets — update mode, sensitivity, color
  - [ ] Save received sensitivity and mode to NVS on change
  - [ ] Enable FreeRTOS watchdog timer — auto-reboot after 10s without valid packet
  - [ ] On reboot, reload last settings from NVS and resume — LEDs stay alive

---

## Phase 5: Trigger & FastLED Performance

- [ ] **ADC trigger engine**
  - [ ] Sample ADC1 (A0) at > 1kHz using `esp_adc_cal` for accurate readings
  - [ ] Peak detection with configurable threshold (sensitivity value from Core Node)
  - [ ] Debounce: ignore secondary peaks within 80ms of first hit
  - [ ] Map ADC peak value to velocity uint16 (0–4095)
  - [ ] Fire LED update immediately on peak detection (< 10ms trigger-to-action target)
  - [ ] Send hit notification to Core Node after LED update (not before — avoids visual lag)

- [ ] **FastLED SK6812NW configuration**
  ```cpp
  FastLED.addLeds<SK6812, DATA_PIN, GRB>(leds, NUM_LEDS).setRgbw(RgbwDefault());
  ```
  - [ ] Confirm `.setRgbw()` API available (FastLED ≥ 3.7.7)
  - [ ] Test white channel extraction: `CRGB(255,255,255)` should activate W diode only

---

## Phase 6: Lighting Modes (The 7+ Modes)

- [ ] **Base modes**
  - [ ] `STATIC_GLOW` — solid RGBW color, full brightness
  - [ ] `TRIGGER_FLASH` — LEDs off at idle, burst on hit, fade out over configurable duration
  - [ ] `VELOCITY_FADE` — brightness and color depth scale with hit ADC peak value

- [ ] **Artistic modes**
  - [ ] `RAINBOW_DYNAMIC` — continuous hue cycling across strip
  - [ ] `RAINBOW_STATIC` — fixed rainbow gradient mapped across strip length
  - [ ] `90S_RAVE` — high saturation, rapid stroboscopic blink synced to trigger
  - [ ] `GHOST_NOTE` — faint ambient glow at idle, sharp saturated burst on hard hit, nearly invisible on soft hit
  - [ ] `HEAT_MAP` — color shifts Blue → Red based on rolling hit frequency (hits/10s window)
  - [ ] `STEALTH` — all LEDs off, diagnostic LEDs still active

- [ ] **Mode state machine (Drum Node)**
  - [ ] Cycle through modes on Mode button press (D3)
  - [ ] Override local mode if Core Node broadcasts a global palette change
  - [ ] Save current mode index to NVS after button press

---

## Phase 7: Thermal Management

- [ ] Apply neutral-cure electronics silicone to large capacitors on 300W CC/CV module (vibration dampening)
- [ ] Thermal paste between Ideal Diode module tabs and heatsink
- [ ] Verify 30–40mm fan spins on Core Node power-on
- [ ] Firmware thermal throttle on drum nodes (if NTC added):
  - < 55°C → full brightness
  - 55–65°C → linear ramp to 120/255
  - 65–75°C → hold at 120/255
  - > 75°C → emergency dim to 40/255
- [ ] Report `tempC` in ESP-NOW telemetry struct → display on Core Node dashboard

---

## Phase 8: Reliability & Field Readiness

- [ ] NVS storage: node MAC → name pairings on Core Node
- [ ] NVS storage: sensitivity, mode, last color on each Drum Node
- [ ] Heartbeat monitor: OFFLINE badge on dashboard after 4s no response
- [ ] Global brightness soft-cap when on wall power (wall detection via ADC divider)
- [ ] `FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500)` on wall mode · `(5, 12000)` on battery
- [ ] Pre-show checklist: verify all 6 voltmeters reading expected voltage, all nodes show ONLINE, battery gauge above 80%
- [ ] Assemble field repair kit: spare 10A fuses, spare SP13 plugs, pre-crimped JST-XH pigtails, spare XIAO, spare TVS diodes

---

## Confirmed Design Decisions

- **LED strip**: SK6812NW (RGBW, Natural White ~4000K), 5V, 60 LED/m — FastLED native RGBW support via `.setRgbw()`
- **Transport voltage**: 14.8–20V trunk from Core Node → local 5V conversion at each drum node
- **Trunk cable**: 18AWG high-strand silicone wire in Techflex Clean-Cut braided sleeve
- **Power OR-ing**: Active ideal diode modules (not passive Schottkys) — near-zero voltage drop, no heatsink required for diodes
- **Charging**: Internal CC/CV module charges batteries while system runs (UPS topology) — common-port BMS
- **Sensitivity**: Software-controlled from Core Node UI, transmitted via ESP-NOW, persisted in NVS on each drum node
- **No local power switch on drum nodes**: Core Node is the only kill switch — prevents accidental mid-show shutoff
- **Drum node connectors**: 4-pin solder pads at board edge for LED strip (5V, GND, Data1, Data2), SP13 90° for power trunk
- **TVS diode on SP13 input**: P6KE24A — protects buck converter from hot-plug inductive spikes
- **100nF ceramic cap on XIAO VCC**: 0805 package, placed ≤3mm from VCC pin — high-frequency decoupling
- **LED data pins**: D9 (GPIO8) primary, D10 (GPIO9) future — both with 330Ω series + PRTR5V0U2X ESD
- **Dual-channel trigger**: TRS jack (standard 3-lug, no normalling needed) — Tip (A0/GPIO1), Ring (A1/D1/GPIO2) with 100kΩ pull-down for software ring detect
- **Green LED eliminated**: replaced by XIAO onboard LED (GPIO21, active LOW)
- **Amber LED resistor**: 1kΩ (pre-buck rail tap, not 220Ω)
- **Buttons**: Gebildet 7mm SPST NO panel-mount — Test (A2/D2) with 5s hold re-pairing, Mode (D3)
- **Status LEDs on right side**: Blue (D5/GPIO6), Red (D8/GPIO7) — keeps switching noise away from analog trigger pins on left
- **GPIO optimization**: Left side = analog/quiet (A0 trigger ch1, A1/D1 trigger ch2, A2/D2 test btn, D3 mode btn, D4 free/NTC, D5 blue LED), Right side = digital output (D10/D9 LED data, D8 red LED) + UART (D7/RX, D6/TX free)
- **Free pins**: D4 (GPIO5, left side) reserved for NTC thermistor, D6/TX (GPIO43) and D7/RX (GPIO44) reserved for UART debug
