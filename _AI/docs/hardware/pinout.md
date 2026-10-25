# Hardware Pinout: Distributed Drum Lighting System

## 1. System Architecture
* **Core Node:** Central Command — Waveshare ESP32-S3-Touch-LCD-7B (1024×600, capacitive touch).
* **Drum Node:** Edge Controller — Seeed Studio XIAO ESP32-S3.
* **Communication:** ESP-NOW (Low Latency Peer-to-Peer).
* **Transport voltage:** 14.0–20.4V trunk (Core Node → Drum Nodes via SP13 connectors).
* **Logic voltage:** 5V local at each Drum Node, from a Pololu D36V50F5 buck. The interim bench buck is the QS-1205CME-3A.

---

## 2. Drum Node (Seeed Studio XIAO ESP32-S3)
*Role: Local sensor sampling, LED driving, and ESP-NOW slave.*

| Pin | GPIO | Function | Description |
| :--- | :--- | :--- | :--- |
| **A0/D0** | 1 | ADC1_CH0 | **Trigger ch1, TRS Tip.** Load resistor to GND (1MΩ start, tune 100k–1M) + 10kΩ 0805 series + BZT52C3V3S Zener + B5817WS Schottky. Reads about 0 when unplugged. |
| **A1/D1** | 2 | ADC1_CH1 | **Trigger ch2, TRS Ring.** Same chain, including the load resistor. Reads about 0 when no TRS plug or no ring piezo is present (software plug detect). |
| **A2/D2** | 3 | GPIO In | **Test Button** — Gebildet 7mm SPST NO, internal pull-up. Short press: manual trigger test. 5s hold: clears Core MAC from NVS, reboots to re-pair. |
| **D3** | 4 | GPIO In | **Mode Button** — Gebildet 7mm SPST NO, internal pull-up. Cycles local lighting mode. |
| **D4** | 5 | ADC1_CH4 | **Reserved — NTC thermistor** (Phase 7 thermal management). |
| **D5** | 6 | GPIO Out | **Blue LED** — Blink=searching, solid=paired, 500ms blink=re-pairing. 220Ω series. |
| **D6/TX** | 43 | UART0 TX | **Free for UART debug.** Do not assign to peripherals. |
| **D7/RX** | 44 | UART0 RX | **Free for UART debug.** Do not assign to peripherals. |
| **D8/SCK** | 7 | GPIO Out | **Red LED** — 80ms flash on trigger, solid during continuous noise. 220Ω series. |
| **D9/MISO** | 8 | RMT/DMA | **LED Data 1 (ring 1):** SN74AHCT1G125 (5V) → 330Ω → PRTR5V0U2X → SK6812 DIN. |
| **D10/MOSI** | 9 | RMT/DMA | **LED Data 2 (ring 2):** SN74AHCT1G125 (5V) → 330Ω → PRTR5V0U2X → SK6812 DIN. Active, sent in parallel with D9. |
| **—** | 21 | GPIO Out | **Onboard LED** — Active LOW. Confirms logic rail live. Replaces discrete green LED. |
| **5V** | 5V | Power In | From the buck output through a B5819WS Schottky, so USB and trunk can both be connected. Never connect trunk voltage directly. |
| **GND** | GND | Ground | Common ground — local star ground with buck GND, LED GND, sensor GND. |

### Hardwired (not XIAO-controlled)
| Connection | Description |
| :--- | :--- |
| SP13 Pin 1 → TVS P6KE27A → 1000µF 50V cap → D36V50F5 VIN | High-rail power path. The TVS stands off 23.1V and clamps at 37.5V, below the buck's 50V maximum. |
| SP13 Pin 1 → 4.7kΩ 1206 → Amber LED → GND | Pre-buck indicator showing trunk power is present. 3.9mA, 72mW. |
| Buck VOUT (5V) → B5819WS → XIAO 5V; 100nF 0603 + 10µF X7R → GND | Decoupling. Must be ≤ 3mm from the XIAO 5V pin on the PCB. |
| Each trigger channel → load R (1MΩ start) → GND | Stops floating inputs and sets signal amplitude. No normalling jack needed. |
| Buck VOUT (5V) → 1000µF 16V → GND, at the LED header | Absorbs flash current steps on the LED bus. |
| TA4F panel Pin 1 ← JST-VH header pin 1 ← Buck 5V out | LED bus 5V, from the local buck, not trunk voltage. Feed both ends of each ring. |
| TA4F panel Pin 2 ← JST-VH header pin 2 ← Local GND | LED bus GND, star ground. |
| TA4F panel Pin 3 ← JST-VH header pin 3 ← PRTR5V0U2X ← 330Ω ← AHCT1G125 ← XIAO D9 | LED bus DATA1, ring 1 DIN. |
| TA4F panel Pin 4 ← JST-VH header pin 4 ← PRTR5V0U2X ← 330Ω ← AHCT1G125 ← XIAO D10 | LED bus DATA2, ring 2 DIN. |

---

## 3. Core Node (Waveshare ESP32-S3-Touch-LCD-7B)
*Role: System orchestration, LVGL Mission Control UI, ESP-NOW master, power distribution supervisor.*

| Pin / Peripheral | GPIO | Function | Description |
| :--- | :--- | :--- | :--- |
| **RGB Panel — PCLK** | 7 | LCD Clock | Internal — 1024×600 pixel clock. |
| **RGB Panel — HSYNC** | 46 | LCD Sync | Internal — Horizontal sync. |
| **RGB Panel — VSYNC** | 3 | LCD Sync | Internal — Vertical sync. |
| **RGB Panel — DE** | 5 | LCD Enable | Internal — Data enable. |
| **RGB Panel — R0–R4** | 1, 2, 42, 41, 40 | LCD Data | Internal — Red channel bits. |
| **RGB Panel — G0–G5** | 39, 0, 45, 48, 47, 21 | LCD Data | Internal — Green channel bits. |
| **RGB Panel — B0–B4** | 14, 38, 18, 17, 10 | LCD Data | Internal — Blue channel bits. |
| **Touch SDA** | 8 | I2C Data | GT911 capacitive touch controller. |
| **Touch SCL** | 9 | I2C Clock | GT911 capacitive touch controller. |
| **Backlight (CH32V003)** | 8/9 (I2C) | I2C Command | Send I2C command to CH32V003 to enable backlight via EXIO2. Cannot use digitalWrite. |
| **Bus Voltage Sense** | GPIO6? (ADC1_CH5), to confirm | ADC In | 150kΩ / 10kΩ divider proposed (37.5V → 2.3V), with 100nF at the pin. Pending the Core Node power revision. |

---

## 4. Drum Node I/O Bus Summary

Three physical connectors on the drum node enclosure wall:

| Bus | Connector | Direction | Carries |
| :--- | :--- | :--- | :--- |
| **Power in** | SP13 90° 2-pin | In ← Core Node | 14.8–20V trunk + GND |
| **Trigger in** | 1/4" TRS panel mount | In ← Piezo sensor | Analog signal + sleeve GND |
| **LED bus out** | Neutrik TA4F mini XLR panel socket | Out → LED strips | 5V · GND · DATA1 · DATA2 |

Cable end (at drum shell): **Cable Techniques LPS-TA4F** right-angle plug. Cable exits parallel to shell surface — no protrusion. 60° adjustable exit direction.

Internal routing: TA4F panel socket → short wire run → **JST-VH 4-pin header** on the PCB (10A per contact). The PCB doesn't need to sit at the enclosure wall.

---

## 5. Electrical Safety Notes

* **TVS on SP13 input (Drum Node):** P6KE27A (through-hole DO-15) across SP13 Pin 1/2.
  * Standoff is 23.1V, so the measured 20.4V wall supply sits at 88%. The clamp is 37.5V, 75% of the D36V50F5's 50V maximum.
  * Required, because drums are connected and disconnected at every show.
  * Stays through-hole: the DO-15 package has the thermal mass for the 600W transient rating.
  * (The P6KE24A was replaced because its 20.5V standoff left no margin at 20.4V.)
* **Input cap:** 1000µF 50V radial electrolytic (35V minimum if 50V doesn't fit). Through-hole, after the TVS and before the buck VIN. Secure the body with a dab of silicone against vibration.
* **Decoupling:** 100nF 0603 plus 10µF X7R at the XIAO 5V pin, ≤ 3mm away and routed before any other power traces. Prevents ADC noise, ESP-NOW packet corruption, and brown-out resets during LED updates.
* **Piezo front end, both channels:**
  * Load resistor to GND.
  * 10kΩ 0805 in series.
  * BZT52C3V3S Zener (cathode → signal).
  * B5817WS Schottky (anode → GND).

  Tune the load resistor so normal hits stay ≤ 2.5V at the pin; the clamps are for faults only. The BZT52C3V3S and B5817WS are SMD equivalents of the 1N4728A and 1N5817.
* **Data lines:** each line runs SN74AHCT1G125 (powered from 5V) → 330Ω → PRTR5V0U2X → DIN. The buffer is required: the SK6812's VIH is 3.4V and the XIAO drives only 3.3V.
* **Trace widths (1 oz copper):** ≥ 80 mil, or a pour, for buck 5V and the LED bus (2.9A). ≥ 40 mil for the trunk input (1.16A). The old 25 mil spec carries only about 1.7A at a 10°C rise.
* **No LED current through XIAO traces:** 5V and GND for the SK6812 rings connect directly to buck output, not through XIAO pads. XIAO provides data signal only.
* **Ideal diode GND pad (Core Node):** Both ideal diode modules require GND pad → star ground (22AWG). Powers internal controller IC. Without it the MOSFET stays off and no current flows.
* **Star ground (Core Node):** All grounds — wall (−), battery (−) via BMS P−, diode GND pads ×2, CC/CV GND, logic buck GND, display GND, all voltmeter blacks, all SP13 Pin 2 — must land at a single copper bus bar. No daisy-chaining.