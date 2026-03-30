# Hardware Pinout: Distributed Drum Lighting System

## 1. System Architecture
* **Core Node:** Central Command — Waveshare ESP32-S3-Touch-LCD-7B (1024×600, capacitive touch).
* **Drum Node:** Edge Controller — Seeed Studio XIAO ESP32-S3.
* **Communication:** ESP-NOW (Low Latency Peer-to-Peer).
* **Transport Voltage:** 14.8–20V trunk (Core Node → Drum Nodes via SP13 connectors).
* **Logic Voltage:** 5V local at each Drum Node (Mini 5V 3A Buck Converter per node).

---

## 2. Drum Node (Seeed Studio XIAO ESP32-S3)
*Role: Local sensor sampling, LED driving, and ESP-NOW slave.*

| Pin | GPIO | Function | Description |
| :--- | :--- | :--- | :--- |
| **A0/D0** | 1 | ADC1_CH0 | **Trigger ch1 — TRS Tip.** Protected by 10kΩ series + 1N4728A Zener (3.3V) + 1N5817 Schottky. |
| **A1/D1** | 2 | ADC1_CH1 | **Trigger ch2 — TRS Ring.** Same protection chain + 100kΩ pull-down to GND. Software ring detect — line reads near-zero when no TRS plug present. |
| **A2/D2** | 3 | GPIO In | **Test Button** — Gebildet 7mm SPST NO, internal pull-up. Short press: manual trigger test. 5s hold: clears Core MAC from NVS, reboots to re-pair. |
| **D3** | 4 | GPIO In | **Mode Button** — Gebildet 7mm SPST NO, internal pull-up. Cycles local lighting mode. |
| **D4** | 5 | ADC1_CH4 | **Reserved — NTC thermistor** (Phase 7 thermal management). |
| **D5** | 6 | GPIO Out | **Blue LED** — Blink=searching, solid=paired, 500ms blink=re-pairing. 220Ω series. |
| **D6/TX** | 43 | UART0 TX | **Free for UART debug.** Do not assign to peripherals. |
| **D7/RX** | 44 | UART0 RX | **Free for UART debug.** Do not assign to peripherals. |
| **D8/SCK** | 7 | GPIO Out | **Red LED** — 80ms flash on trigger, solid during continuous noise. 220Ω series. |
| **D9/MISO** | 8 | RMT/DMA | **LED Data 1** — 330Ω series → PRTR5V0U2X ESD → SK6812NW DIN (primary strip). |
| **D10/MOSI** | 9 | RMT/DMA | **LED Data 2** — 330Ω series → PRTR5V0U2X ESD → SK6812NW DIN (future secondary strip). |
| **—** | 21 | GPIO Out | **Onboard LED** — Active LOW. Confirms logic rail live. Replaces discrete green LED. |
| **5V** | 5V | Power In | From local 5V 3A Buck Converter output. Do NOT connect directly to trunk voltage. |
| **GND** | GND | Ground | Common ground — local star ground with buck GND, LED GND, sensor GND. |

### Hardwired (not XIAO-controlled)
| Connection | Description |
| :--- | :--- |
| SP13 Pin 1 → TVS P6KE24A → 1000µF cap → Buck VIN | High-rail power path. TVS clamps hot-plug spikes to 24V. |
| SP13 Pin 1 → 1kΩ → Amber LED → GND | Pre-buck indicator — trunk power present. |
| Buck VOUT (5V) → 100nF ceramic 0805 → GND | Decoupling cap. Must be ≤3mm from XIAO VCC pin on PCB. |
| Trigger ch2 clamp output → 100kΩ → GND | Pull-down on A1/D1 ring line. Keeps ch2 at GND when no TRS plug is inserted — no normalling jack needed. |
| TA4F panel Pin 1 ← JST-XH header pin 1 ← Buck 5V out | LED bus 5V — from local buck, not trunk voltage. |
| TA4F panel Pin 2 ← JST-XH header pin 2 ← Local GND | LED bus GND — star ground. |
| TA4F panel Pin 3 ← JST-XH header pin 3 ← 330Ω ← PRTR5V0U2X ← XIAO D9 | LED bus DATA1 — SK6812NW strip 1 DIN. |
| TA4F panel Pin 4 ← JST-XH header pin 4 ← 330Ω ← PRTR5V0U2X ← XIAO D10 | LED bus DATA2 — SK6812NW strip 2 DIN (future). |

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
| **Bus Voltage Sense** | ADC pin | ADC In | 100kΩ / 10kΩ divider from main VCC bus — reads rail voltage for LVGL dashboard. |

---

## 4. Drum Node I/O Bus Summary

Three physical connectors on the drum node enclosure wall:

| Bus | Connector | Direction | Carries |
| :--- | :--- | :--- | :--- |
| **Power in** | SP13 90° 2-pin | In ← Core Node | 14.8–20V trunk + GND |
| **Trigger in** | 1/4" TRS panel mount | In ← Piezo sensor | Analog signal + sleeve GND |
| **LED bus out** | Neutrik TA4F mini XLR panel socket | Out → LED strips | 5V · GND · DATA1 · DATA2 |

Cable end (at drum shell): **Cable Techniques LPS-TA4F** right-angle plug. Cable exits parallel to shell surface — no protrusion. 60° adjustable exit direction.

Internal routing: TA4F panel socket → short wire run → **JST-XH 4-pin header** on PCB. PCB does not need to be positioned at the enclosure wall.

---

## 5. Electrical Safety Notes

* **TVS on SP13 input (Drum Node):** P6KE24A across SP13 Pin 1/2. Clamps hot-plug inductive spikes before they reach the buck converter. Required — drums are connected/disconnected at every show.
* **100nF decoupling cap:** 0603 ceramic on XIAO VCC, placed ≤3mm from the VCC pin and routed before any other power traces. Prevents ADC noise, ESP-NOW packet corruption, and brown-out resets during high-speed LED updates.
* **Piezo protection trio:** 10kΩ series + 1N4728A Zener (cathode→signal) + 1N5817 Schottky (anode→GND). All three required for reliable hit detection without GPIO damage.
* **Data lines:** 330Ω series resistor on D9 (primary) and D10 (future), placed close to XIAO. PRTR5V0U2X ESD diode between resistor output and SK6812NW DIN.
* **No LED current through XIAO traces:** 5V and GND for SK6812NW connect directly to buck output, not through XIAO pads. XIAO provides data signal only.
* **Ideal diode GND pad (Core Node):** Both ideal diode modules require GND pad → star ground (22AWG). Powers internal controller IC. Without it the MOSFET stays off and no current flows.
* **Star ground (Core Node):** All grounds — wall (−), battery (−) via BMS P−, diode GND pads ×2, CC/CV GND, logic buck GND, display GND, all voltmeter blacks, all SP13 Pin 2 — must land at a single copper bus bar. No daisy-chaining.