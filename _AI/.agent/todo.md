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
| Power dist. PCB | Custom 2-layer KiCad PCB | Logic/distribution backplane · 1.6mm · 1oz copper · HASL · ground pour on bottom layer Gerber · JLCPCB 5-pack ~$20–25 shipped |

### Drum Node (× number of drums)
| Component | Part | LCSC | Spec |
|---|---|---|---|
| MCU | Seeed Studio XIAO ESP32-S3 | C9900154951 | Dual-core · 5V pin · D9/D10 data · A0 trig ch1 · D3 trig ch2 · D8 ring detect · D1/D2/D4/D5 I/O |
| LED strip | SK6812NW 60 LED/m | — | 5V · RGBW · Natural White ~4000K |
| Input connector | SP13 90° 2-pin | — | Pin 1: 14.8–20V in · Pin 2: GND · Panel mount |
| LED bus | 4× solder holes 2.54mm pitch | — | 5V · GND · Data1 · Data2 — direct solder, no connector |
| Input cap | 1000µF 25V Electrolytic TH | C10750 | D10×17mm · Across SP13 input — anti-flicker reservoir |
| TVS diode | P6KE24A | — | DO-15 THT · Cathode → VIN_RAW · clamps hot-plug spikes to 24V |
| Buck converter | QS-1205CME-3A | — | MP2315 module · 20×11×5mm · 5V pad bridged before assembly |
| Decoupling cap | 100nF 0805 Ceramic | C49678 | No polarity · placed ≤3mm from XIAO VCC pin |
| Trigger jack | J3 Switched 1/4" TRS Panel Mount | — | Tip=ch1 · Ring=ch2 · Sleeve=GND · Normalling contact shorts Ring→GND on TS insert |
| Ch1 piezo resistor | R1 10kΩ 1/4W TH | C57438 | Series · current limits Tip signal · J3 tip → R1 → NET_TRIG1_CLAMP |
| Ch1 Zener | D2 1N4728A 3.3V | C58985 | DO-41 · Cathode → NET_TRIG1_CLAMP · Anode → GND |
| Ch1 Schottky | D3 1N5817 | C8598 | DO-41 · Anode → GND · Cathode → NET_TRIG1_CLAMP |
| Ch2 piezo resistor | R8 10kΩ 1/4W TH | C57438 | Series · current limits Ring signal · J3 ring → R8 → NET_TRIG2_CLAMP |
| Ch2 Zener | D4 1N4728A 3.3V | C58985 | DO-41 · Cathode → NET_TRIG2_CLAMP · Anode → GND |
| Ch2 Schottky | D5 1N5817 | C8598 | DO-41 · Anode → GND · Cathode → NET_TRIG2_CLAMP |
| Data 1 resistor | R2 330Ω 1/4W TH | C57436 | Series on D9 → PRTR IO1 · prevents signal ringing |
| Data 1 ESD | U3 PRTR5V0U2X | C12333 | SOT-363 · Pin1=GND · Pin2=IO1 · Pin3=IO2 · Pin4=VCC |
| Data 2 resistor | R7 330Ω 1/4W TH | C57436 | Series on D10 → PRTR IO1 · prevents signal ringing |
| Data 2 ESD | U4 PRTR5V0U2X | C12333 | SOT-363 · same wiring as U3 · future-use data line |
| Blue LED resistor | R3 220Ω 1/4W TH | C57435 | XIAO D1 → R3 → Blue LED → GND |
| Red LED resistor | R4 220Ω 1/4W TH | C57435 | XIAO D2 → R4 → Red LED → GND |
| Amber LED resistor | R5 1kΩ 1/4W TH | C57437 | VIN_RAW → R5 → Amber LED → GND · SP13 live indicator |
| User LED (onboard) | GPIO21 internal | — | Active LOW · on = MCU running = 5V live · use LED_BUILTIN in firmware |
| Trigger jack | J3 1/4" TRS Panel Mount | — | Piezo input · Tip = signal · Sleeve = GND |
| LED amber | LED3 5mm Amber | — | Hardwired to VIN_RAW (pre-buck) — SP13 power present |
| LED blue | LED1 5mm Blue | — | XIAO D1/GPIO2 — blink=searching · solid=paired |
| LED red | LED2 5mm Red | — | XIAO D2/GPIO3 — flashes on trigger hit |
| LED holders | 5mm Plastic Socket × 4 | — | Panel mount |
| Test button | SW1 NKK JB15 Tactile | — | XIAO D4/GPIO5 — fires trigger manually · internal pull-up |
| Mode button | SW2 NKK JB15 Tactile | — | XIAO D5/GPIO6 — cycles local color mode · internal pull-up |
| Internal wire | 18AWG High-Strand Silicone | — | All internal connections — vibration resistant |
| Trunk cable | 18AWG Silicone + Techflex Clean-Cut | — | Core Node → drum SP13 runs |
| PCB mount | 4× M2 TPU standoffs | — | Isolates PCB from drum shell vibration |
| Enclosure | 3D printed PETG | — | Hockey-puck form · ~55×25mm · honeycomb vents |

#### Drum Node LCSC BOM Summary
| Ref | Part | LCSC | Qty |
|---|---|---|---|
| U1 | XIAO ESP32-S3 | C9900154951 | 1 |
| U2 | QS-1205CME-3A buck module | — (order from QSKJ/AliExpress) | 1 |
| U3, U4 | PRTR5V0U2X ESD | C12333 | 2 |
| C1 | 1000µF 25V electrolytic TH | C10750 | 1 |
| C2 | 100nF 0805 ceramic | C49678 | 1 |
| D1 | P6KE24A TVS DO-15 | — (DigiKey/Mouser) | 1 |
| D2, D4 | 1N4728A 3.3V Zener DO-41 | C58985 | 2 |
| D3, D5 | 1N5817 Schottky DO-41 | C8598 | 2 |
| R1, R8 | 10kΩ 1/4W TH | C57438 | 2 |
| R2, R7 | 330Ω 1/4W TH | C57436 | 2 |
| R3, R4 | 220Ω 1/4W TH | C57435 | 2 |
| R5 | 1kΩ 1/4W TH | C57437 | 1 |
| J1 | SP13 90° 2-pin | — | 1 |
| J3 | Switched 1/4" TRS Panel Mount | — | 1 |
| LED1 | 5mm Blue LED | — | 1 |
| LED2 | 5mm Red LED | — | 1 |
| LED3 | 5mm Amber LED | — | 1 |
| SW1, SW2 | NKK JB15 Tactile | — | 2 |

---

## Drum Node GPIO Assignments (XIAO ESP32-S3)

| Arduino | GPIO | Function | Net |
|---|---|---|---|
| A0 | GPIO1 | Trigger ch1 ADC input (Tip) — active TS and TRS | NET_TRIG1_CLAMP |
| D3 | GPIO4 | Trigger ch2 ADC input (Ring) — active TRS only | NET_TRIG2_CLAMP |
| D8 | GPIO7 | Ring detect / TRS vs TS sense (internal pull-up) | NET_RING_DETECT |
| D1 | GPIO2 | Blue status LED output | NET_D1_220R |
| D2 | GPIO3 | Red trigger LED output | NET_D2_220R |
| D4 | GPIO5 | Test button input (pull-up) | NET_D4_BTN |
| D5 | GPIO6 | Mode button input (pull-up) | NET_D5_BTN |
| D9 | GPIO8 | LED Data 1 output (SK6812NW DIN) | NET_D9_330R |
| D10 | GPIO9 | LED Data 2 output (future use) | NET_D10_330R |
| GPIO21 | internal | Onboard user LED (active LOW) — 5V live indicator | LED_BUILTIN |
| 5V | — | VCC from buck | NET_5V |
| GND | — | Star ground bus | NET_GND |
| D6/D7 | GPIO43/44 | Spare — NC | — |

---

## Drum Node Schematic Net List

| Net | Description | Nodes |
|---|---|---|
| NET_VIN_RAW | 14.8–20V trunk rail | J1 pin1 · D1 cathode · C1+ · U2 VIN+ · R5 pin1 |
| NET_5V | 5V regulated output | U2 VOUT+ · U1 5V · C2+ · R3/R4 pin1 · U3/U4 VCC · LED bus hole 1 |
| NET_GND | Star ground | J1 pin2 · C1− · U2 VIN−/VOUT− · U1 GND · D1 anode · D2/D3/D4/D5 anode · R1/R8 −side · U3/U4 GND · LED bus hole 2 · SW1/SW2 pin2 · LED1/LED2/LED3 cathodes · J3 sleeve |
| NET_PIEZO1_IN | Raw piezo ch1 from TRS tip | J3 tip · R1 pin1 |
| NET_TRIG1_CLAMP | Clamped ch1 signal | R1 pin2 · D2 cathode · D3 cathode · U1 A0/GPIO1 |
| NET_PIEZO2_IN | Raw piezo ch2 from TRS ring | J3 ring · R8 pin1 |
| NET_TRIG2_CLAMP | Clamped ch2 signal | R8 pin2 · D4 cathode · D5 cathode · U1 D3/GPIO4 |
| NET_RING_DETECT | TRS vs TS detect | J3 normalling contact · U1 D8/GPIO7 (internal pull-up) |
| NET_D9_330R | D9 before series R | U1 D9 · R2 pin1 |
| NET_D9_ESD_IN | D9 after R2, into U3 | R2 pin2 · U3 IO1(pin2) |
| NET_LED_DIN | Data 1 to LED bus | U3 IO2(pin3) · LED bus hole 3 |
| NET_D10_330R | D10 before series R | U1 D10 · R7 pin1 |
| NET_D10_ESD_IN | D10 after R7, into U4 | R7 pin2 · U4 IO1(pin2) |
| NET_LED_DATA2 | Data 2 to LED bus | U4 IO2(pin3) · LED bus hole 4 |
| NET_D1_220R | D1 output | U1 D1 · R3 pin1 |
| NET_LED_BLUE_A | Blue LED anode | R3 pin2 · LED1 anode |
| NET_D2_220R | D2 output | U1 D2 · R4 pin1 |
| NET_LED_RED_A | Red LED anode | R4 pin2 · LED2 anode |
| NET_LED_AMBER_A | Amber LED anode | R5 pin2 · LED3 anode |
| NET_D4_BTN | Test button | U1 D4 · SW1 pin1 |
| NET_D5_BTN | Mode button | U1 D5 · SW2 pin1 |

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
  - [ ] Include SP13 90° mount port, TRS jack cutout, 4× LED holes, 2× button holes
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
- [ ] Star ground bus bar: wall (−), batt (−) via BMS P−, diode GND pads × 2, CC/CV GND — high-current connections only · logic-side GND handled by PCB ground pour
- [ ] Daly BMS wiring: P− → star ground, B− → parallel battery negative junction, balance harness to cell junctions

### Core Node power distribution PCB
- [ ] Design split-architecture logic/distribution PCB in KiCad (~80×120mm, 2-layer)
  - High-current trunk (ideal diodes, 40A fuse, switch, batteries, CC/CV, BMS) stays point-to-point 12AWG with ring terminals — do NOT route on PCB
  - PCB handles sub-20A distribution only: fuse block feed points, logic 5V buck, voltmeter headers, ESP32 connector, ADC divider, bus cap, reverse polarity MOSFET
- [ ] Power trace widths: 200–300mil for VCC bus → fuse block feeds · 60–80mil for 5V logic rail · 12–20mil for ADC sense line
- [ ] Add full ground pour on bottom copper layer in KiCad (Fill Zone → GND net → press B) — exports in Gerber automatically, no JLCPCB upcharge
- [ ] Keep board at standard 1.6mm thickness (JLCPCB default — non-standard thickness adds cost)
- [ ] Add 4× M3 mounting holes at corners — mount to internal Baltic Birch frame via M3 brass standoffs (prevents flex, replaces need for thicker board)
- [ ] Route ADC sense line away from buck converter switching node
- [ ] Order from JLCPCB: 2-layer · 1.6mm · 1oz copper · HASL · 5-pack · ~$2–5 boards + ~$15–20 shipping
- [ ] Run ERC and DRC — zero errors before ordering

### Drum Node PCB (EasyEDA Pro v2.2.45.4)
- [ ] Create XIAO ESP32-S3 schematic symbol (14-pin, manually drawn — no LCSC library entry)
  - Left side pins 1–8: D0/A0, D1, D2, D3, D4, D5, D6/TX, D7/RX
  - Right side pins 9–14: D8/SCK, D9/MISO, D10/MOSI, 3V3, GND, 5V
  - Use community footprint from OSHWLab for PCB pads
- [ ] Import QS-1205CME-3A module as 4-pin generic symbol (VIN+, VIN−, VOUT+, VOUT−)
  - EasyEDA component ID: 83f60a073cc545eb836decbf8807d3c0
  - Bridge 5V solder pad on back of each module BEFORE soldering to PCB
- [ ] Draw schematic — power path:
  - J1 SP13 Pin1 → D1 P6KE24A (K→VIN_RAW, A→GND) → C1 1000µF (+ to VIN_RAW) → U2 QS-1205CME-3A VIN+
  - U2 VIN− → GND star bus
  - U2 VOUT+ → C2 100nF (placed ≤3mm from XIAO VCC, no polarity) → NET_5V rail
  - NET_VIN_RAW → R5 1kΩ → LED3 Amber → GND (SP13 live indicator)
  - NET_5V live indicator: use onboard GPIO21 user LED (active LOW) — no external component needed
- [ ] Draw schematic — signal path (TRS dual-channel trigger):
  - J3 switched TRS jack: Tip → NET_PIEZO1_IN · Ring → NET_PIEZO2_IN · Sleeve → GND · Normalling contact → NET_RING_DETECT
  - Ch1 protection: J3 tip → R1 10kΩ → NET_TRIG1_CLAMP · D2 1N4728A (K→TRIG1, A→GND) · D3 1N5817 (A→GND, K→TRIG1) · U1 A0/GPIO1
  - Ch2 protection: J3 ring → R8 10kΩ → NET_TRIG2_CLAMP · D4 1N4728A (K→TRIG2, A→GND) · D5 1N5817 (A→GND, K→TRIG2) · U1 D3/GPIO4
  - Ring detect: J3 normalling contact → NET_RING_DETECT → U1 D8/GPIO7 (internal pull-up, reads LOW on TS insert)
  - U1 D9/GPIO8 → R2 330Ω → U3 PRTR5V0U2X pin2(IO1) → pin3(IO2) → LED bus hole 3 (Data1)
  - U1 D10/GPIO9 → R7 330Ω → U4 PRTR5V0U2X pin2(IO1) → pin3(IO2) → LED bus hole 4 (Data2)
  - U3 pin1(GND) → NET_GND · U3 pin4(VCC) → NET_5V
  - U4 pin1(GND) → NET_GND · U4 pin4(VCC) → NET_5V
  - U1 D1/GPIO2 → R3 220Ω → LED1 Blue → GND
  - U1 D2/GPIO3 → R4 220Ω → LED2 Red → GND
  - U1 D4/GPIO5 ← SW1 Test button → GND (internal pull-up)
  - U1 D5/GPIO6 ← SW2 Mode button → GND (internal pull-up)
- [ ] Draw schematic — LED bus (4× solder holes, 2.54mm pitch, board edge):
  - Hole 1: NET_5V
  - Hole 2: NET_GND
  - Hole 3: NET_LED_DIN (Data 1 from U3)
  - Hole 4: NET_LED_DATA2 (Data 2 from U4 — future use)
- [ ] PCB layout rules:
  - Power traces (NET_5V, NET_VIN_RAW): ≥ 1.0mm (40mil)
  - VIN_RAW traces: ≥ 0.635mm (25mil) minimum
  - Signal traces: ≥ 0.3mm (12mil) minimum
  - Clearance between nets: ≥ 0.5mm (20mil)
  - Ground plane both layers
  - Teardrops on all pads (vibration hardening)
  - 4× M2 mounting holes at corners
  - C2 placed ≤3mm from XIAO VCC pad — first component on VOUT+ trace
  - D1 P6KE24A placed immediately after J1 SP13 on VIN_RAW before C1
  - SW node from buck module: route short and direct to first passive — keep away from A0/trigger traces
  - LED bus solder holes at board edge with strain relief clearance
- [ ] Run ERC — zero errors before proceeding
- [ ] Run DRC — zero errors before ordering
- [ ] Order PCBs (5-pack minimum for spares)
- [ ] Order components per LCSC BOM table above
- [ ] Order QS-1205CME-3A modules from QSKJ or AliExpress (order spares — 2 per node minimum)
- [ ] Order P6KE24A from DigiKey or Mouser (not on LCSC)

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
  - [ ] On boot: if no Core Node MAC stored in NVS → enter pairing mode automatically
  - [ ] **Pairing mode** (entered on boot with no stored MAC, or manually via Test button hold):
    - Blue LED blinks at 500ms interval while scanning for Core Node
    - Broadcast pairing packet every 500ms until Core Node responds
    - On successful pair: blue LED goes solid · store Core Node MAC in NVS
  - [ ] **Test button hold → pairing mode** (manual re-pair):
    - Monitor D4/GPIO5 in firmware loop — track hold duration with `millis()`
    - If held ≥ 5 seconds: clear stored Core Node MAC from NVS → enter pairing mode
    - Blue LED begins 500ms blink immediately on 5s threshold — no reboot required
    - Short press (< 5s): fires manual trigger as before
  - [ ] **Blue LED state machine**:
    - Pairing / scanning: blink 500ms on / 500ms off
    - Paired and connected: solid on
    - OFFLINE / heartbeat timeout: slow blink 1000ms on / 1000ms off (distinguishable from pairing blink)
  - [ ] **Red LED behaviour**:
    - Flash on any trigger detection — both ch1 (Tip) and ch2 (Ring)
    - Flash duration: 80ms (matches debounce window) — visible but snappy
    - Active regardless of pairing state — useful for diagnosing trigger noise pre-show
    - If ADC is reading continuous noise above threshold on either channel: red LED stays on — acts as noise indicator
  - [ ] Receive `DrumState` packets — update mode, sensitivity, color
  - [ ] Save received sensitivity and mode to NVS on change
  - [ ] Enable FreeRTOS watchdog timer — auto-reboot after 10s without valid packet
  - [ ] On reboot, reload last settings from NVS and resume — LEDs stay alive

---

## Phase 5: Trigger & FastLED Performance

- [ ] **ADC trigger engine (dual-channel TRS)**
  - [ ] On boot: read D8/GPIO7 (ring detect) — LOW = TS mono mode, HIGH = TRS stereo mode
  - [ ] Sample ADC ch1 (A0/GPIO1) at > 1kHz using `esp_adc_cal` — always active
  - [ ] Sample ADC ch2 (D3/GPIO4) at > 1kHz — only processed in TRS stereo mode
  - [ ] Peak detection with configurable threshold (sensitivity value from Core Node)
  - [ ] Debounce: ignore secondary peaks within 80ms of first hit per channel
  - [ ] Map ADC peak value to velocity uint16 (0–4095) per channel
  - [ ] Fire LED update immediately on peak detection (< 10ms trigger-to-action target)
  - [ ] Flash red LED (D2/GPIO3) for 80ms on any peak detection — both ch1 and ch2
  - [ ] If ADC value on either channel stays continuously above threshold (noise floor exceeded): hold red LED on until signal drops — pre-show noise diagnostic
  - [ ] Send hit notification to Core Node after LED update (not before — avoids visual lag)
  - [ ] Include channel ID in hit notification struct (ch1 = head/main, ch2 = rim/secondary)

- [ ] **Button handler (D4/GPIO5 — Test button)**
  - [ ] Track press duration using `millis()` — no blocking delays
  - [ ] Short press (< 5s): fire manual trigger event on ch1 — same path as real piezo hit
  - [ ] Hold ≥ 5s: enter pairing mode — clear NVS Core Node MAC, begin blue LED blink, broadcast pairing packets
  - [ ] Use FreeRTOS task or polling in main loop — do not block ADC sampling during button hold

- [ ] **FastLED SK6812NW configuration**
  ```cpp
  #define DATA_PIN   D9   // GPIO8 — primary LED data
  #define DATA_PIN_2 D10  // GPIO9 — secondary LED data (future use)
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
  - [ ] Cycle through modes on Mode button press (D5/GPIO6)
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
- [ ] Assemble field repair kit: spare 10A fuses, spare SP13 plugs, spare JST-XH pigtails, spare XIAO, spare P6KE24A TVS, spare QS-1205CME-3A modules

---

## Confirmed Design Decisions

- **LED strip**: SK6812NW (RGBW, Natural White ~4000K), 5V, 60 LED/m — FastLED native RGBW support via `.setRgbw()`
- **Transport voltage**: 14.8–20V trunk from Core Node → local 5V conversion at each drum node
- **Trunk cable**: 18AWG high-strand silicone wire in Techflex Clean-Cut braided sleeve
- **Power OR-ing**: Active ideal diode modules (not passive Schottkys) — near-zero voltage drop, no heatsink required for diodes
- **Charging**: Internal CC/CV module charges batteries while system runs (UPS topology) — common-port BMS
- **Sensitivity**: Software-controlled from Core Node UI, transmitted via ESP-NOW, persisted in NVS on each drum node
- **No local power switch on drum nodes**: Core Node is the only kill switch — prevents accidental mid-show shutoff
- **Drum node LED bus**: 4× direct solder holes at board edge (2.54mm pitch) — no connector · 5V · GND · Data1 · Data2
- **LED Data 1**: D9/GPIO8 — through 330Ω (R2) and PRTR5V0U2X (U3) to LED bus hole 3
- **LED Data 2**: D10/GPIO9 — through 330Ω (R7) and PRTR5V0U2X (U4) to LED bus hole 4 — future use pin, NC for current build
- **TVS diode on SP13 input**: P6KE24A DO-15 THT — cathode to VIN_RAW, anode to GND — protects buck from hot-plug spikes
- **Buck converter**: QS-1205CME-3A (MP2315 module, 20×11×5mm) — bridge 5V pad on back before PCB assembly
- **Decoupling cap on XIAO VCC**: 100nF 0805 ceramic (C49678) — no polarity — placed ≤3mm from VCC pin
- **All resistors**: Through-hole 1/4W axial — R1=10kΩ, R2/R7=330Ω, R3/R4/R6=220Ω, R5=1kΩ
- **ESD protection**: PRTR5V0U2X SOT-363 (C12333) on both data lines — Pin1=GND, Pin2=IO1(in), Pin3=IO2(out), Pin4=VCC
- **Trigger input**: Switched 1/4" TRS panel-mount jack — Tip=ch1 (always active), Ring=ch2 (TRS only), Sleeve=GND · normalling contact detects TS vs TRS insert via D8/GPIO7 pull-up
- **Trigger ch1 (Tip)**: A0/GPIO1 — through R1 10kΩ + 1N4728A Zener + 1N5817 Schottky clamp — active for both TS and TRS cables
- **Trigger ch2 (Ring)**: D3/GPIO4 — through R8 10kΩ + 1N4728A Zener + 1N5817 Schottky clamp — only sampled in TRS stereo mode
- **Ring detect**: D8/GPIO7 — internal pull-up · reads LOW when normalling contact shorts Ring→GND (TS insert) · reads HIGH when TRS opens normalling contact
- **EDA tool**: EasyEDA Pro v2.2.45.4
- **5V live indicator**: onboard XIAO user LED (GPIO21, active LOW, yellow) — `digitalWrite(LED_BUILTIN, LOW)` on boot · no external LED or resistor needed · LED_BUILTIN constant maps correctly when XIAO ESP32-S3 board is selected in Arduino IDE
- **PCB trace widths**: 5V/VIN_RAW power ≥ 1.0mm · signal ≥ 0.3mm · clearance ≥ 0.5mm
