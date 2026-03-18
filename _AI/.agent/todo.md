# Project Mission Log: Nexus PDU (ADLS Development)

## 🟢 Phase 1: Mechanical Fabrication & Power Grid
* [x] **Core Node Case Design:** Finalize Wood/Lexan hybrid clamshell.
* [x] **Structural Integrity:** Plan internal "Spine" reinforcement for drum stand mount.
* [ ] **Enclosure Fabrication:**
    * [ ] Build partitioned enclosure (Electronics Bay vs. Storage Bay).
    * [ ] Install aluminum heatsink/spine for power semiconductor mounting.
* [ ] **High-Current Power Path:**
    * [ ] **Source Selection:** Install **MBR20100CT** Schottky diodes for Battery/DC OR-ing.
    * [ ] **Thermal Management:** Secure Schottkys to heatsink with thermal paste and Sil-Pads.
    * [ ] **Bus Bars:** Layout high-current copper bus bars using 12AWG/10AWG wire.
    * [ ] **Regulation:** Mount 300W buck-boost converter with vibration-dampened standoffs.

## 🟡 Phase 2: Drum Node Engineering (The Edge)
* [ ] **Custom PCB Design (EasyEDA):**
    * [ ] Schematic: XIAO ESP32-S3 + 3.3V Zener (1N4728A) + 1MΩ resistor + 330Ω data resistor.
    * [ ] Routing: 25mil power traces, teardrop pads, and dual-layer ground planes for vibration resistance.
* [ ] **Diagnostic I/O:**
    * [ ] Install "Sync" (Blue), "Trigger" (Red), "12V-20V Power" (Amber), and "5V Power" (Green) LEDs.
    * [ ] Mount physical "Test" and "Color Mode" tactile switches.
* [ ] **Trigger Firmware:**
    * [ ] Sample ADC1 at > 1kHz with peak detection logic.
    * [ ] Implement "Trigger-to-Action" latency path (< 10ms).

## 🔵 Phase 3: Core Node UI (The Brain - 1024x600)
* [ ] **Display & Driver Setup:**
    * [ ] Configure `lv_conf.h` for **1024x600** resolution and 192KB memory pool.
    * [ ] Implement **CH32V003** I2C command to enable backlight via EXIO2.
    * [ ] Configure **GT911** touch driver (SDA: GPIO8, SCL: GPIO9).
* [ ] **Mission Control Dashboard:**
    * [ ] Design 1024x600 status cards for each connected drum.
    * [ ] Create digital sliders for remote sensitivity (transmitted via ESP-NOW).
    * [ ] Enable high-res Montserrat (24/32/48) fonts for stage visibility.

## 🔴 Phase 4: Communication & Discovery
* [ ] **ESP-NOW Protocol:**
    * [ ] Define `struct DrumState` (Sensitivity, Mode, Fade, Color Palette).
    * [ ] Implement "Auto-Join": Core Node identifies and registers nodes by MAC address.
* [ ] **Diagnostic Handshake:**
    * [ ] Implement "Blink-to-Solid" sync light logic.
    * [ ] Implement "Identity Flash": Pulse a specific drum white via the Core Node UI to confirm placement.

## 🌈 Phase 5: Creative Synthesis (The "Lighting Engine")
* [ ] **Base Modes:**
    * [ ] `RAINBOW_DYNAMIC`: Continuous hue cycling.
    * [ ] `RAINBOW_STATIC`: Fixed rainbow gradient across the strip.
    * [ ] `VELOCITY_FADE`: Brightness/Color depth based on hit intensity.
* [ ] **Artistic Modes:**
    * [ ] `90S_RAVE`: High saturation, rapid stroboscopic blinking.
    * [ ] `GHOST_NOTE`: Faint ambient glow on low velocity, sharp burst on high.
    * [ ] `HEAT_MAP`: Color shifts (Blue → Red) based on hit frequency over time.
    * [ ] `STEALTH`: All LEDs off except for minimal diagnostic indicators.

## 🛡️ Phase 6: Reliability & Persistence
* [ ] **NVS Storage:** Save node pairings and sensitivity settings locally on each node.
* [ ] **Heartbeat Monitor:** Implement "OFFLINE" visual alerts if a node stops responding.
* [ ] **Safety Limits:** Implement global 3.0A soft-cap enforcement per node.

---

### Hardware Inventory (Final Specs)
* **Compute:** Waveshare ESP32-S3-Touch-LCD-7B (Core), XIAO ESP32-S3 (Drum).
* **Power Protection:** 2x MBR20100CT Schottky Diodes (Power OR-ing).
* **Trigger Protection:** 1N4728A 3.3V Zener Diodes.
* **Chassis:** 1/2" Baltic Birch, 1/4" Lexan, Roland APC-33 Mount.