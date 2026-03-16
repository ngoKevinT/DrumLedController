To handle a 300W load at a nominal 14.8V (4S LiPo), you are looking at continuous currents around **20A**. Standard small signal diodes will melt instantly.

You need **Power Schottky Rectifiers** in a **TO-220** or **TO-247** package. These look like power transistors and are designed to be bolted to a heatsink.

### The Component: MBR20100CT (or similar)

I recommend the **MBR20100CT**. It is a "Dual Center Tap" Schottky.

* **Current:** 20A (10A per leg, or 20A if you tie the legs together).
* **Voltage:** 100V (way more than your 16.8V max, which provides a huge safety buffer).
* **Low Forward Voltage:** Usually around $0.6V$ to $0.8V$ at full load, minimizing power loss and heat.

### Engineering Rule: The "OR-ing" Heatsink

Since you'll have two of these (one for Battery, one for External DC), and they will be dissipating roughly $12W$ to $15W$ of heat at full load ($Voltage Drop \times Current$), you **cannot** let them hang in the air.

* **The Mount:** Bolt them directly to a small aluminum finned heatsink inside the Core Node’s electronics bay.
* **The Insulation:** Use a **Sil-Pad** or mica insulator between the diode and the heatsink if the heatsink is shared, as the "Tab" of these diodes is usually connected to the Cathode (output).

---

### Updated `todo.md` — Nexus PDU: Electronic Drum Lighting System

#### 🟢 Phase 1: Mechanical Design & Housing

* [x] **Core Node:** Finalize Wood/Lexan hybrid clamshell design.
* [x] **Core Node:** Plan internal "Spine" reinforcement for drum stand mount.
* [ ] **Core Node:** Fabricate partitioned enclosure (Electronics Bay vs. Storage Bay).
* [ ] **Core Node:** Install aluminum heatsink area for power semiconductors.
* [ ] **Drum Nodes:** Design/Print 3D brackets for diagnostic I/O (LEDs, Test Button, Mode Button).

#### 🟡 Phase 2: Power Grid & Custom PCB Design

* [ ] **Core Node Power Path (The Schottky Protection):**
* [ ] **Source Selection:** Install **MBR20100CT** (or equivalent) Schottky diodes for Battery/DC "OR-ing."
* [ ] **Thermal Management:** Secure Schottkys to heatsink with thermal paste/insulators.
* [ ] **Bus Bars:** Layout high-current copper bus bars (use thick 12AWG or 10AWG wire).
* [ ] **Regulator:** Mount 300W buck-boost converter with vibration-dampened standoffs.


* [ ] **Drum Node PCB Layout (EasyEDA):**
* [ ] Finalize schematic: XIAO ESP32-S3 + 3.3V Zener + 1MΩ Resistor + 330Ω Data Resistor.
* [ ] Route **25mil** power traces and **Copper Ground Planes**.
* [ ] Order PCBs and components (NKK switches, LED holders, 1N4728A Diodes).



#### 🔵 Phase 3: Core Node UI & Display (1280x800)

* [ ] **Display Setup:** Configure `lv_conf.h` for **1280x800** resolution and double-buffering.
* [ ] **Mission Control UI:**
* [ ] Build "Dashboard" status cards for each connected drum.
* [ ] Create digital sliders for remote sensitivity (transmitted via ESP-NOW).
* [ ] Enable high-res fonts (Montserrat 24/32) for stage visibility.



#### 🔴 Phase 4: Wireless Protocol & Logic (ESP-NOW)

* [ ] **Protocol:** Define bi-directional `struct` (Mode, Color, Sensitivity, Telemetry).
* [ ] **Discovery:** Implement "Auto-Join" so Core Node identifies nodes by MAC address.
* [ ] **Drum Node Firmware:** * [ ] State machine for **Color Mode** and **Test** buttons.
* [ ] **NVS Storage:** Save sensitivity/mode locally on the XIAO.
* [ ] Sync LED logic (Blink = Searching, Solid = Connected).



---

### Updated Hardware Inventory (Power Focus)

* **Power Protection:** 2x MBR20100CT Schottky Diodes (Power OR-ing).
* **Trigger Protection:** 1N4728A 3.3V Zener Diodes (One per Drum Node).
* **Structure:** 1/2" Baltic Birch, 1/4" Lexan, Roland APC-33 Mount.
* **Compute:** XIAO ESP32-S3 (Drum), ESP32-S3 DevKit (Core).