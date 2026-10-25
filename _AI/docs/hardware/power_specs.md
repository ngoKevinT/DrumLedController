# Specification: Power Management & Per-Node Limits

All limits in this file follow the derating policy in `.agent/standards.md` §0:
hardware runs at ≤ ~75% of its rating, and every software limit sits 15–25% below
the hardware limit it protects.

---

## 1. System Power Architecture

* **Transport layer:** DC trunk from the Core Node to each Drum Node over 18AWG silicone cable with SP13 connectors.
  * Measured wall supply: **20.4V** (SlimQ 150W, DC barrel output, rated 20V / 7.5A).
  * Battery range (4S LiPo): 14.0V (depleted, under load) to 16.8V (full).
* **Local regulation:** each Drum Node has its own 5V buck. It powers the SK6812 strips and the XIAO ESP32-S3.
* **Core Node power grid:** **UNDER REVISION.** See §7. The design is being reworked because several components were found to be under-rated.
* **Current bench setup:** SlimQ DC 20.4V → 5A blade fuse → a single Drum Node, with no Core Node in the path.

---

## 2. Drum Node 5V Budget

| Load | Worst case |
|---|---|
| LEDs (software-capped, battery mode) | 2500 mA |
| XIAO ESP32-S3, WiFi TX peaks | 400 mA |
| Status LEDs and logic | 20 mA |
| **Total** | **~2.92 A** |

**Buck: Pololu D36V50F5** (5V, 5.5A typ, 6–50V in, reverse protection to 40V).
* 2.92A is about 53% of 5.5A.
* VIN at the clamp: the P6KE27A clamps at 37.5V, which is 75% of the 50V maximum.

**Trunk input current per node** (14.6W out, 90% efficiency → 16.2W in):

| Trunk voltage | Input current |
|---|---|
| 14.0V | 1.16 A |
| 20.4V | 0.80 A |

These are the figures that size the per-channel fuse, the SP13 connector and the trunk wire.

**Interim bench buck: QS-1205CME-3A.** Rated 4.5–24V in, 3A max, **2.1A continuous**, 24V maximum input. Bench use only:
* LED software cap **1200 mA**. 1.2A of LEDs plus 0.4A for the XIAO is 1.6A, which is 76% of 2.1A.
* Connect the drum node first, then plug in the wall supply.
* The output is jumper-selectable. Verify **5.0V** before attaching the XIAO.

---

## 3. LED Current Model

* **Strip:** BTF-LIGHTING SK6812 RGBW (RGB + Cool White), 5V, 60 LED/m, IP30.
* **Per-LED draw:** about 60 mA with all four channels at 255. **Calibrate on the bench:** measure each channel at 255 across a known number of LEDs, then put the per-channel mA values in the firmware constants.
* **Per-drum count:** two parallel rings. The largest drum is 24", so π × 24" ≈ 1.92 m, about **115 LEDs per ring and 230 per drum**.
* **Uncapped worst case:** 230 × 60 mA = **13.8 A**. The firmware governor is mandatory, not optional.
* At the 2500 mA cap, full white on all LEDs scales to about 18% brightness. That's acceptable for drum effects, which rarely light every LED at full white.
* **Voltage drop:** feed 5V/GND at **both ends** of each ring, to halve the drop along the strip.

---

## 4. Software Power Caps

| Mode | LED cap per node | Ceiling it protects |
|---|---|---|
| Battery | **2500 mA** | Buck 5.5A (53% with the XIAO); TA4F 5A carry (50%) |
| Wall | **2000 mA (provisional)** | Wall supply budget ≤ 80% of 150W. Final value set by the Core Node revision. |
| Bench (QS-1205CME) | **1200 mA** | 2.1A continuous rating |
| Absolute ceiling | Never above 2500 mA | Any value from the Core Node is clamped to this on the drum node |

The drum node always clamps the received cap to `min(received, 2500)`. A UI toggle must never be able to **disable** the governor; it can only choose between the wall and battery caps.

### Governor (runs before every frame is sent)

The governor does not rely on any LED library's built-in limiter. That's because our strip's per-channel currents differ from the library defaults, and RGBW white extraction changes the actual draw.

1. **Estimate:**
   `i_est_ma = Σ_pixels (r·I_r + g·I_g + b·I_b + w·I_w) / 255 + n_leds · I_idle`
   * `I_r`, `I_g`, `I_b`, `I_w` are the calibrated per-channel currents at 255.
   * `I_idle` is about 1 mA per LED of quiescent draw.
   * The estimate runs on the **post-RGBW-conversion** values, because those are what the strip actually receives.
2. **Margin:** `i_est_ma = i_est_ma × 110 / 100` covers calibration error.
3. **Scale:** if `i_est_ma > cap_ma`, multiply every channel by `cap_ma / i_est_ma`, using an 8.8 fixed-point scale factor and integer math only.

---

## 5. Thermal Safety

* **Buck:** about 1.6W dissipated at full load (14.6W at 90% efficiency). Vent the puck and mount the buck away from the XIAO.
* **NTC on D4:** place the footprint beside the buck now; the firmware for it comes in Phase 7.
  * Below 55°C: full brightness.
  * 55–65°C: linear ramp down to 120/255.
  * 65–75°C: hold at 120/255.
  * Above 75°C: emergency dim to 40/255. This leaves 10°C of buffer below the 85°C component rating.
* **Prolonged static white:** if a node stays above 80% of its cap for more than 5 minutes with no trigger, ramp brightness down softly.

---

## 6. Transient & Fault Protection (Drum Node)

| Part | Value | Margin |
|---|---|---|
| TVS | **P6KE27A** (through-hole DO-15). Standoff 23.1V, minimum breakdown 25.7V, clamp 37.5V | 20.4V is 88% of standoff |
| Input reservoir | 1000µF **50V** radial (35V minimum if 50V doesn't fit) | 20.4V is 41% of 50V; the 37.5V clamp is below 50V |
| Reverse polarity | The D36V50F5 has built-in reverse protection to 40V, and the TVS conducts in the forward direction as a crowbar until the fuse blows | Two independent layers |
| Per-channel fuse | **3A** blade. 5A is acceptable for bench work. | 2.6× the 1.16A maximum input |

---

## 7. Core Node Power — UNDER REVISION

The current wall-only build did not work. These review findings must be resolved in the revised design:

1. **Master switch is unprotected:** a 40A fuse sits after a 20A switch. The real load is about 7.2A (6 × 1.16A plus logic). Proposed fix: a 15A main fuse (48% loaded), and verify the switch's **DC** rating.
2. **Per-channel fuses at 10A** protect only the wire. Use 3A per channel.
3. **The CC/CV charger is fed from the OR junction.** On battery power it is fed by the battery it charges, and with the switch off it drains the pack continuously through its idle current. Feed the charger from the wall side, before Ideal Diode #1.
4. **Wall budget is about 187W against 150W.**
   * Charger: 16.6V × 5A ≈ 87W.
   * Nodes: 6 × 16.2W ≈ 97W.
   * Logic: about 3W.

   The target is ≤ 120W (80%). Example that fits: wall-mode cap of 2000 mA and charger at 2A ≈ 118W.
5. **Charge voltage:** use 16.6V (4.15 V/cell) rather than 16.8V. That gives about 0.4V of margin below the Daly BMS over-voltage cutoff (about 17.0V).
6. **Parallel packs:**
   * A 20A fuse on each pack before the parallel junction.
   * A parallel balance board, so the BMS sees both packs' cells.
7. **Logic buck:** replace the 24V-max module with a **Pololu D36V28F5** (50V in). The load is about 0.6A: the Waveshare board is rated 5V 350mA, plus the voltmeters.
8. **Bus protection:** add a P6KE27A across the main bus, and change the 100µF bus cap to 35–50V. At 25V it runs at 82%.
9. **Bus voltage sense:**
   * Use a 150k/10k divider. 37.5V maps to 2.3V and 20.4V maps to 1.28V, both inside the ADC's ~3.1V full scale.
   * Add 100nF at the ADC pin.
   * Confirm a free ADC pin on the Waveshare 7B. GPIO6 (ADC1_CH5) looks unused; verify on the schematic.
10. **Low-battery thresholds (provisional):**
    * Warning at 14.0V (3.5 V/cell).
    * Low-power mode at 13.5V: LEDs capped at 30%.
    * LEDs off at 13.2V (3.3 V/cell). This happens before the BMS under-voltage cutoff, at about 2.7–3.0 V/cell.
