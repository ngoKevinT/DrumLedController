# Project Mission Log: ADLS Development
# Automated Drum Lighting System — Nexus PDU

---

## Hardware Inventory

### Core Node: ⚠️ POWER GRID UNDER REVISION
> The wall-only build failed (2026-10-24). The review found under-rated parts and a charger drain path.
> The rows below are the **original** design. The required changes are listed in
> `docs/hardware/power_specs.md` §7: 15A main fuse, 3A channel fuses, charger fed from
> the wall side, D36V28F5 logic buck, P6KE27A bus TVS, 35–50V bus cap, 150k/10k divider,
> 16.6V charge voltage, per-pack fuses plus a parallel balance board, and the wall power budget.

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
| VCC bus cap | 100µF 25V electrolytic | Across main VCC bus output, absorbs switch-on transient. **Revision: 35–50V** |
| Bus voltage sense | 100kΩ / 10kΩ voltage divider | → ESP32 ADC pin, shows bus voltage on the LVGL dashboard. **Revision: 150k/10k + 100nF** |

### Drum Node (× number of drums)
| Component | Part | Spec |
|---|---|---|
| MCU | Seeed Studio XIAO ESP32-S3 | Dual-core · 5V pin · D9/D10 LED data · A0/A1 trigger · D5/D8 status LEDs · A2(D2)/D3 buttons · D6(TX)/D7(RX) free |
| LED strip | BTF-LIGHTING SK6812 RGBW, 60 LED/m | 5V · RGB + Cool White ~6000K · IP30 · 2 rings per drum (≈115 LEDs per ring at 24") |
| Input connector | SP13 90° 2-pin | Pin 1: 14.0–20.4V in · Pin 2: GND · 13A rating (9% loaded) |
| LED header (PCB) | JST-VH 4-pin | 5V · GND · Data1 · Data2 · 10A per contact (25% loaded) |
| LED bus connector | TA4F panel socket | 5A carry (50% loaded). Check part number; the TA4F cable plug is obsolete, TA4FX replaces it. |
| Input cap | 1000µF **50V** radial electrolytic (35V min) | After the TVS, anti-flicker reservoir |
| TVS diode | **P6KE27A** (DO-15) | Across SP13 Pin 1/2. Standoff 23.1V, clamp 37.5V |
| Buck converter | **Pololu D36V50F5** | 5V 5.5A, 6–50V in, reverse protection to 40V. Bench interim: QS-1205CME-3A |
| Decoupling caps | 100nF 0603 + 10µF X7R | ≤ 3mm from the XIAO 5V pin |
| XIAO feed diode | B5819WS (SOD-323) | Buck 5V → XIAO 5V; allows USB plus trunk at the same time |
| LED output cap | 1000µF 16V | At the LED header |
| Piezo series R | 10kΩ 0805 | Both channels |
| Piezo load R | 1MΩ 0805 (tune 100k–1M) | **Both** channels, signal to GND. Stops floating inputs; sets amplitude |
| Zener diode | BZT52C3V3S (SOD-123) | Cathode → signal · Anode → GND · both channels |
| Schottky diode | B5817WS (SOD-123) | Anode → GND · Cathode → signal · both channels |
| Data buffer | SN74AHCT1G125 ×2 (SOT-23-5) | 3.3V → 5V logic for SK6812 (VIH 3.4V) |
| Data resistor | 330Ω 0805 ×2 | After each buffer |
| ESD diode | PRTR5V0U2X | Both data lines, at the header |
| Trigger jack | 1/4" TRS Panel Mount | Dual-channel piezo input · standard 3-lug (tip/ring/sleeve), no normalling needed |
| LED amber | 5mm Amber + **4.7kΩ 1206** | Pre-buck indicator · 3.9mA, 72mW (the old 1kΩ 0805 dissipated 0.34W) |
| LED blue | 5mm Blue + 220Ω (100Ω optional, brighter) | XIAO D5 · blink = searching · solid = connected · 500ms blink = re-pairing |
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

## Open Conflicts & Unverified Assumptions (2026-10-24 review)

Unresolved items where the docs, the code or the hardware disagree, or rest on something not yet verified.
Resolve each one and record the outcome in `docs/hardware/architecture.md`.

### Power (blocked on the Core Node revision, `power_specs.md` §7)
- [ ] **TODO:** Redesign the Core Node power grid. Resolve all ten findings in §7: switch vs fuse rating, channel fuses, charger drain path, wall budget, charge voltage, per-pack fuses and balance board, logic buck, bus TVS and cap, bus sense divider, low-battery thresholds.
- [ ] **TODO:** Finalize the **wall-mode LED cap**. 2000 mA is provisional and depends on the charger current and the wall supply chosen in the redesign.
- [ ] **TODO:** Confirm the **low-battery thresholds** (14.0 / 13.5 / 13.2V) against the actual Daly BMS under-voltage cutoff.
- [ ] **TODO:** Confirm the master switch's **DC** current rating. Many "20A" rockers are rated for AC or 12V DC only.
- [ ] **TODO:** Confirm a free **ADC pin** on the Waveshare 7B for bus voltage sense. GPIO6 (ADC1_CH5) looks unused but isn't documented as free.

### Drum node hardware
- [ ] **TODO:** Verify the XIAO ESP32-S3 **5V pin ↔ USB VBUS** connection on the Seeed schematic. That decides whether the B5819WS feed diode is mandatory or just extra margin.
- [ ] **TODO:** **TA4F** is listed obsolete at Digi-Key (TA4FX replaces the cable plug). Confirm current part numbers for the panel socket and the right-angle cable plug, and their solder-cup wire gauge.
- [ ] **TODO:** **LED bus voltage drop:** measure 5V at the far end of each ring at the 2500 mA cap; it must stay ≥ 4.5V. Decide how to feed both ends of each ring through a 4-pin TA4F (separate power leads inside the drum, or heavier gauge).
- [ ] **TODO:** Confirm the Waveshare 7B backlight controller chip. The docs say CH32V003, but the Waveshare wiki only says "IO extension" (EXIO2 = backlight). Also add GT911 INT (GPIO4) to `pinout.md`.

### Drum node firmware
- [ ] **TODO:** **`DRUM_NODE_ID` is hard-coded to 1** in drum-node `main.c`, so every drum would send `node_id = 1`. The wire format says 1–6 identify drums. Have the Core assign `node_id` in PAIR_ACK and save it in NVS.
- [ ] **TODO:** Spike **frame pre-emption**: confirm whether `led_strip` can abort an in-flight RMT transmission. If not, `led_hal` needs a custom RMT encoder.
- [ ] **TODO:** Verify the strip's **byte order** (GRBW assumed) with an R/G/B/W test pattern.
- [ ] **TODO:** Verify the **ADC full-scale** at 12 dB attenuation (~3.1V assumed) using `adc_cali`. The piezo level target (2.5V) is derived from it.
- [ ] **TODO:** `raw_reading` and `threshold` are 10-bit on the wire, but the ADC is 12-bit. Keep the ×4 mapping documented in one place, or move the wire format to 12-bit.

### Code vs standards
- [ ] **TODO:** `#define` for tunable values breaks `standards.md` §4. Examples: `NODE_NAME_MAX_LEN` in drum `main.c`; slider ranges (`THRESH_MIN`, `BRIGHTNESS_MAX`, `FTIME_*`, `DECAY_*`) in the UI screens; `UI_MAX_DRUMS`. Convert them to `static const` or `enum`.
- [ ] **TODO:** The UI has a **"Power Limit (3A)" on/off toggle** (`g_power_limit_3a`). That conflicts with "the governor can never be disabled". Replace it with a wall/battery cap selector (see Phase 8).
- [ ] **TODO:** UI file header comments say "BLE/ESP-NOW". BLE isn't used, so remove those references.

### Operations
- [ ] **TODO:** Pre-show procedure: connect all drum SP13 plugs **with the Core Node switch off**. The 1000µF input caps cause an inrush spark when hot-plugged, which pits the contacts over time.

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
  - [ ] **TODO: Fit check.** Confirm the Pololu D36V50F5 buck (25.4 × 25.4 × 9.5 mm) and the 1000µF 50V radial cap fit inside the ~55 × 25 mm puck alongside the PCB, TA4F socket and TRS jack. Leave airflow around the buck (≈1.6W at full load). If the 50V cap doesn't fit, fall back to 35V minimum. If the buck doesn't fit, enlarge the puck rather than downgrade the buck.
  - [ ] Add honeycomb ventilation on top and bottom faces
  - [ ] Print M2 TPU standoffs for PCB isolation

---

## Phase 2: Power Grid & PCB Design

### Core Node wiring: ⚠️ ON HOLD, power grid under revision (`power_specs.md` §7)
> The steps below describe the original design and are kept for reference. **Don't continue building them**
> until the revision is done. Current bench setup: SlimQ DC → 5A blade fuse → single drum node.

- [ ] Solder parallel harness for 2× Tattu batteries (verify voltages within 0.1V before connecting)
- [ ] Install a TVS on the main bus (revision: P6KE27A, not P6KE24A)
- [ ] Wire Ideal Diode #1 (wall path): SlimQ → VIN, VOUT → OR splice, GND pad → star ground (22AWG)
- [ ] Wire Ideal Diode #2 (battery path): parallel batt+ → VIN, VOUT → OR splice, GND pad → star ground (22AWG)
- [ ] Wire CC/CV module: OR splice → IN(+), OUT(+) → Daly BMS B+ / battery splice
- [ ] Calibrate CC/CV: set CV to 16.80V, CC to 5.00A (bench test before connecting batteries)
- [ ] Install 24V illuminated switch: PIN 1 ← OR splice, PIN 2 → 40A fuse, PIN 3 (LED GND) → star ground
- [ ] Install 40A inline blade fuse after switch
- [ ] Install 100µF 25V cap across main VCC bus output
- [ ] Install 100kΩ / 10kΩ voltage divider: main bus → ESP32 ADC pin (for LVGL dashboard readout)
- [ ] Wire logic 3A buck: main VCC bus → IN, 5V OUT → Waveshare 7B + all 6× voltmeter RED wires
- [ ] Wire Nilight fuse block: main VCC bus stud → input, 10A blade per channel
- [ ] Wire 6× SP13 outputs: fuse block terminal + voltmeter YELLOW sense wire per channel
- [ ] Star ground bus bar: wall (−), batt (−) via BMS P−, diode GND pads × 2, CC/CV GND, buck GND in/out, display GND, 6× SP13 Pin 2, switch LED GND, all voltmeter BLACK wires
- [ ] Daly BMS wiring: P− → star ground, B− → parallel battery negative junction, balance harness to cell junctions

### Drum Node PCB (EasyEDA / KiCad / Flux AI)
- [ ] Import XIAO ESP32-S3 footprint (LCSC: C9900124959)
- [ ] Draw schematic:
  - SP13 Pin 1 → TVS P6KE27A (across to GND) → 1000µF 50V cap (across to GND) → D36V50F5 VIN
  - Buck VOUT (5V) → B5819WS → XIAO 5V pin; 100nF 0603 + 10µF X7R to GND, ≤ 3mm from the pin
  - Buck VOUT (5V) → 1000µF 16V → JST-VH 5V (LED bus); feed both ends of each ring
  - Trigger ch1: ¼" TRS Tip → load R (1MΩ) to GND → 10kΩ series → BZT52C3V3S (K → signal) + B5817WS (K → signal, A → GND) → XIAO A0 (GPIO1)
  - Trigger ch2: ¼" TRS Ring → same chain, including the load R → XIAO A1/D1 (GPIO2)
  - LED data 1: XIAO D9 (GPIO8) → SN74AHCT1G125 (5V) → 330Ω → PRTR5V0U2X → JST-VH Data1
  - LED data 2: XIAO D10 (GPIO9) → SN74AHCT1G125 (5V) → 330Ω → PRTR5V0U2X → JST-VH Data2
  - XIAO D5 → 220Ω → Blue LED → GND
  - XIAO D8 → 220Ω → Red LED → GND
  - XIAO A2/D2 ← Test button → GND (internal pull-up; optional 10k external + 100nF) · 5s hold → re-pairing mode
  - XIAO D3 ← Mode button → GND (same)
  - Amber LED: tap pre-buck VCC → 4.7kΩ 1206 → LED → GND
  - Green LED: eliminated; use the XIAO onboard LED (GPIO21, active LOW)
  - NTC footprint on D4 (GPIO5), next to the buck (10k NTC + 10k divider; may be left unpopulated at first)
- [ ] Route 5V/LED power **≥ 80 mil** (or pour) and trunk input **≥ 40 mil** (1 oz). 25 mil carries only about 1.7A at a 10°C rise.
- [ ] Add copper ground plane both sides
- [ ] Add teardrops on all pads (vibration hardening)
- [ ] Add 4× M2 mounting holes at corners
- [ ] Verify no traces narrower than 12mil for signal lines
- [ ] Run ERC and DRC — zero errors before ordering
- [ ] Order PCBs (suggest 5-pack minimum for spares)
- [ ] Order components: see Hardware Inventory table above

### Drum Node PCB: 2026-10-24 design review (decisions applied above)
Triggered by the bench measurement (wall supply = 20.4V) and the derating policy (`standards.md` §0):
- [x] TVS P6KE24A → **P6KE27A** (the 24A's 20.5V standoff left no margin at 20.4V)
- [x] Buck QS-1205CME-3A → **Pololu D36V50F5**. The QS is 2.1A continuous, 24V max in, no OVP or reverse protection, and listed not-for-sale. ([listing](https://www.qskj.cc/shop/mini-dc-dc-buck-step-down-module-4-24v-12v-24v-to-5v-3a-power-module-qs-1205cme-3a-6684))
- [x] Input cap 25V → **50V** (35V min)
- [x] Amber LED resistor 1kΩ 0805 (0.34W) → **4.7kΩ 1206** (72mW)
- [x] LED header JST-XH (83% loaded) → **JST-VH** (25%)
- [x] Add **SN74AHCT1G125** data buffers (SK6812 VIH 3.4V vs 3.3V GPIO)
- [x] Add a **load resistor on ch1** (it floated when unplugged) to match ch2
- [x] Add B5819WS XIAO feed diode, 10µF local cap, 1000µF LED output cap
- [x] Trace widths 25 mil → 80 mil (5V/LED) and 40 mil (trunk)
- [ ] Mechanical check: D36V50F5 (25.4×25.4mm) and the 50V cap fit inside the ~55mm puck
- [ ] Bench: tune the piezo load R so the hardest hit is ≈ 2.5V at the pin
- [ ] Bench: calibrate SK6812 per-channel mA for the governor constants

### Bench interim rules (QS-1205CME-3A fitted)
- Connect the drum node before plugging in the wall supply; disconnect in reverse. Keep bench leads short to avoid cable-inductance hot-plug spikes.
- The output is jumper-selectable (0.8–17V adjustable / 1.8–12V fixed). Verify the 5V setting and measure 5.0V before attaching the XIAO.
- Firmware LED cap **1200 mA** while this buck is fitted. 1.2A + 0.4A for the XIAO is 76% of its 2.1A continuous rating. Restore 2500 mA after the buck is replaced.
- The EN pin is 2–5V logic. Never tie it to trunk voltage.
- Bench fuse: 5A blade (acceptable for now); 3A is preferred.

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
  - [ ] Check PSRAM on boot (`esp_psram_get_size()`); halt with an error if it's unavailable
  - [ ] RGB panel driver for the Waveshare 7B (vendor ESP-IDF driver in `core-node/vendor/`)
  - [ ] IO-expander I2C command to enable the backlight (EXIO2, GPIO8/9)
  - [ ] GT911 capacitive touch driver (SDA: GPIO8, SCL: GPIO9)
  - [ ] Fix the VS Code IntelliSense include path for ESP-IDF headers (`.vscode/c_cpp_properties.json`)

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

- [x] **Data structure:** source of truth is `src/esp32/shared/espnow_transport/espnow_transport.h` (`espnow_config_pkt_t`, `espnow_telemetry_pkt_t`, pairing packets)
- [ ] Add `temp_c` and the active power cap to telemetry

- [ ] **Core Node (Master)**
  - [x] Initialize ESP-NOW on boot
  - [x] Pairing listener: register drum nodes by MAC on first contact (PAIR_REQ/ACK)
  - [ ] Store MAC → drum name mapping in NVS
  - [ ] Unicast `espnow_config_pkt_t` to the drum on Save/Sync
  - [ ] "Identity Flash": ping a single node, which flashes white 3× for physical identification
  - [ ] Heartbeat: poll every **1 s**, mark OFFLINE after **3 misses (3.5 s)**
  - [ ] Wall vs battery detection, then broadcast the matching LED cap (2000 / 2500 mA). Pending the power revision.

- [ ] **Drum Node (Slave)**
  - [x] Broadcast a pairing packet on boot if no Core Node MAC is stored in NVS
  - [x] Blue LED: 500ms blink while searching, solid when paired, slow blink when standalone
  - [x] Receive CONFIG packets and update `DrumConfig`; save to NVS
  - [ ] Clamp every received value to local hard limits (LED cap ≤ 2500 mA, ranges per field)
  - [ ] Enable the ESP-IDF **task watchdog** (`esp_task_wdt`), so the node reboots only on a firmware hang, **not** on packet loss
  - [x] On reboot, reload the last settings from NVS and resume

---

## Phase 5: Trigger & LED Performance

- [ ] **ADC trigger engine** (`docs/hardware/software/hit_detection.md`)
  - [ ] ADC continuous mode (DMA) on A0 + A1, **≥ 10 kHz per channel**, `adc_cali` curve fitting
  - [ ] 3-sample moving average; peak detect over a 1 ms attack window; hysteresis (off = ¾ on)
  - [ ] Mask: `retrigger_ms` from the Core Node (default 25 ms, range 0–500)
  - [ ] Plug detect: a channel that stays at the noise floor is treated as disconnected
  - [ ] Queue the LED update **before** telemetry

- [ ] **LED driver** (`docs/hardware/software/led_engine.md`)
  - [ ] `led_hal` over Espressif `led_strip`: 2 RMT channels (D9, D10), SK6812 timing, GRBW
  - [ ] Verify the strip's byte order with a test pattern (R, G, B, W one at a time)
  - [ ] Spike: frame pre-emption (`led_hal_abort()`). If `led_strip` can't abort, write a custom RMT encoder.
  - [ ] RGB → RGBW conversion (W = min(r,g,b)); `(255,255,255)` should light only the W LED
  - [ ] Governor: calibrated per-channel mA + 10% margin; cap 1200 mA (bench) / 2500 (battery) / 2000 (wall)
  - [ ] **Measure** hit-to-light latency with GPIO toggles and a scope. Target ≤ 6 ms; hard limit 10 ms.

---

## Phase 6: Lighting Modes

- [x] **v1 mode list approved:** SOLID, PULSE, RAINBOW, CHASE, HEAT_MAP, CUSTOM, STEALTH (`docs/hardware/software/modes_logic.md`)
- [x] Shared `drum_mode_t` enum in `espnow_transport.h`; `ui_mode_names[]` tied to it with a `_Static_assert`
- [ ] Implement a render-function dispatch table indexed by `drum_mode_t` in the LED engine
- [ ] Clamp a received `mode_idx ≥ DRUM_MODE_COUNT` to SOLID
- [ ] Mode button favorites: add a `favorites` field (bitmask) to `espnow_config_pkt_t`, a picker in the Core UI, and the drum default list (SOLID → PULSE → RAINBOW → CHASE → STEALTH)
- [ ] Core Node presets: Reactive, Velocity Fade, Ghost Note, Static Rainbow (settings bundles, no firmware modes)
- [ ] CUSTOM: define the per-drum pattern format (renders as SOLID until then)
- [ ] Backlog modes (v2): STROBE/RAVE (≤ 3 Hz default, faster only with a UI opt-in and warning), BOUNCE, SPECTRUM, VOID
- [ ] Fail-safe: if the trigger input is lost, fall back to a low-brightness standby heartbeat

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
- [ ] Heartbeat monitor: OFFLINE badge after 3 missed polls (3.5 s)
- [ ] LED cap per node: **2000 mA wall / 2500 mA battery**, enforced by the drum-node governor and clamped locally (never higher than 2500)
- [ ] Replace the UI's `g_power_limit_3a` on/off toggle with a wall/battery cap selector (the governor can never be disabled)
- [ ] Pre-show checklist: all 6 voltmeters read the expected voltage, all nodes show ONLINE, battery gauge above 80%
- [ ] Field repair kit: spare 3A fuses, spare SP13 plugs, pre-crimped JST-VH pigtails, spare XIAO, spare P6KE27A TVS diodes

---

## Confirmed Design Decisions

Full table with rationale: `docs/hardware/architecture.md` §5. Short list:

- **Derating policy**: hardware ≤ ~75% of rating; software limits 15–25% below the hardware limit (`standards.md` §0)
- **Toolchain**: ESP-IDF 5.5 C; LEDs via Espressif `led_strip` behind our own `led_hal`. FastLED is not used.
- **LED strip**: BTF-LIGHTING SK6812 RGBW (Cool White ~6000K), 5V, 60 LED/m; 2 rings per drum, ≤ ~120 LEDs per data line
- **Transport voltage**: 14.0–20.4V trunk from the Core Node, converted to 5V locally at each drum node
- **Trunk cable**: 18AWG high-strand silicone wire in Techflex Clean-Cut braided sleeve
- **Core Node power grid** (OR-ing, UPS charging, fuses): **under revision** (`power_specs.md` §7)
- **Sensitivity**: software-controlled from the Core Node UI, sent over ESP-NOW, saved in NVS on each drum node
- **Watchdog**: task watchdog only. Packet loss means standalone mode, not a reboot.
- **No local power switch on drum nodes**: the Core Node is the only kill switch, which prevents an accidental mid-show shutoff
- **Drum node connectors**: SP13 90° for the power trunk; JST-VH 4-pin header → TA4F panel socket for the LED bus (5V, GND, Data1, Data2)
- **Drum buck**: Pololu D36V50F5 (5V 5.5A, 50V in, reverse protection)
- **TVS diode on SP13 input**: P6KE27A (standoff 23.1V, clamp 37.5V)
- **Input cap**: 1000µF 50V (35V min)
- **XIAO decoupling**: 100nF 0603 + 10µF X7R ≤ 3mm from the 5V pin; B5819WS feed diode
- **LED data pins**: D9 (GPIO8) ring 1, D10 (GPIO9) ring 2. Both active, each with AHCT1G125 → 330Ω → PRTR5V0U2X.
- **Dual-channel trigger**: TRS jack (standard 3-lug, no normalling needed). Tip (A0/GPIO1), Ring (A1/D1/GPIO2). Load resistor on **both** channels.
- **Green LED eliminated**: replaced by the XIAO onboard LED (GPIO21, active LOW)
- **Amber LED resistor**: 4.7kΩ 1206 (pre-buck rail tap)
- **Buttons**: Gebildet 7mm SPST NO panel-mount — Test (A2/D2) with 5s hold re-pairing, Mode (D3)
- **Status LEDs on right side**: Blue (D5/GPIO6), Red (D8/GPIO7) — keeps switching noise away from analog trigger pins on left
- **GPIO optimization**: Left side = analog/quiet (A0 trigger ch1, A1/D1 trigger ch2, A2/D2 test btn, D3 mode btn, D4 free/NTC, D5 blue LED), Right side = digital output (D10/D9 LED data, D8 red LED) + UART (D7/RX, D6/TX free)
- **Free pins**: D4 (GPIO5, left side) reserved for NTC thermistor, D6/TX (GPIO43) and D7/RX (GPIO44) reserved for UART debug
- **No reverse polarity MOSFET on Core Node**: FQP27P06 P-ch MOSFET removed — redundant given polarized XT60 battery connectors, Daly BMS built-in protection, and ideal diode module on battery path. Three independent layers already cover this failure mode.
