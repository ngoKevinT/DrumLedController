This `ui_lvgl.md` is the final blueprint for your **Core Node Command Center**. It integrates your wireframe layout, the "Staging" logic, and the high-performance "Invalidation" practices we've discussed.

As a Director of Engineering, I've architected this so the Agent treats the screen as two distinct functional zones: **Monitoring** (Passive/High Frequency) and **Control** (Active/Staged).

Place this in: `/docs/software/ui_lvgl.md`

---

# UI Specification: Core Node Command Center (LVGL)

## 1. Visual Architecture & Layout

The screen is divided into two primary vertical zones on the LilyGo T-Display-S3 (320x170 resolution).

### A. The Control Panel (Left Half)

A grid-based interaction area for adjusting the "Staged" state.

* **Mode/Color Grid:** 2x2 grid of large, tactile buttons (`MODE UP/DOWN`, `COLOR UP/DOWN`).
* **Staging Window:** A central text area (`lv_label`) displaying the *proposed* Mode and Color.
* **Visual State:** Text is **Yellow** when staged (pending) and **Green** when committed (active).


* **Sync Button:** A full-width button to broadcast current staged settings to all connected Drum Nodes.
* **Select/Submit:** The "Commit" button that triggers the Downlink ESP-NOW transmission.

### B. The Drum Monitor (Right Half)

A graphical representation of the physical drum kit.

* **Drum Node Widgets:** 4 Interactive circular/oval objects (Snare, T.Tom, F.Tom, Kick).
* **State Label:** Displays Drum Name and [MAC ID].
* **Live Feedback:** Two labels per node: `Voltage (V)` and `Threshold (T)`.
* **Selection Toggle:** Clicking a drum object sets it as the "Active Target" for the Left Panel.


* **Telemetry List:** A scrolling list below the kit showing Hex codes and Mode status for each node.

---

## 2. Interaction & Staging Logic

### The "Commit" Workflow (First Principles)

To prevent accidental changes and network congestion, all edits follow a **Stage-then-Commit** pattern:

1. **Target Selection:** User taps "SNARE" on the Right Monitor. The Left Panel populates with the Snare's *last known* state.
2. **Editing:** User taps "MODE UP". The Left Panel's Staging Window updates to the next mode and turns **Yellow**. No data is sent to the Drum Node yet.
3. **Transmission:** User taps "SELECT".
* The Core Node packs the `DrumNodeState` struct.
* The packet is sent via high-priority ESP-NOW to the Snare's MAC address.
* Upon successful transmission, the Staging Window turns **Green**.



---

## 3. Performance & Optimization

### A. Invalidation Optimization (The Senior Move)

* **No Global Refreshes:** The `lv_timer_handler()` must not trigger full-screen redraws.
* **Dirty Rectangle Tracking:** Only update the `Voltage` and `Threshold` labels when a new `DrumTelemetry` packet arrives.
* **Static Layering:** The background kit layout and button frames should be drawn once and never invalidated unless the menu page changes.

### B. Asset Management

* **Memory:** Use internal SRAM for the active screen buffer.
* **Fonts:** Use built-in Montserrat fonts (size 14-20) for clarity. Avoid custom high-res images to save SPI bandwidth.

---

## 4. Input Device (Indev) Mapping

* **Touch Screen:** Direct interaction with all buttons and drum widgets.
* **Physical Buttons (Hardware):**
* **Button A (GPIO 14):** Cycle through the 4 Drum Nodes (Selection).
* **Button B (GPIO 0):** Toggle between "Dashboard" and "Global Settings" (the Gear icons).



---

## 5. Telemetry Handling (Uplink)

Incoming `DrumTelemetry` packets (Fire-and-Forget) are processed as follows:

1. Identify `node_id`.
2. Map to the corresponding Right-Side Widget.
3. Update `peak_voltage` and `hex_color` labels.
4. **Trigger Pulse:** Briefly brighten the border of the Drum Widget to provide a visual "blink" matching the physical drum hit.

---

### **The "Senior" Checklist for the Agent**

1. **Widget Styling:** Use `lv_style_set_border_width` to highlight the "Selected" drum node.
2. **Non-Blocking Logic:** Ensure the UI remains responsive even if a Drum Node is offline.
3. **Visual Feedback:** Use a small "Syncing..." spinner or icon when the 'SELECT' button is pressed to indicate transmission.
