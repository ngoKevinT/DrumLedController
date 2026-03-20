# Specification: Power Management & The 3.0A Per-Node Protocol

## 1. System Power Architecture

* **Transport Layer:** 14.8–20V DC trunk from Core Node to each Drum Node via 18AWG silicone trunk cable and SP13 connectors.
* **Local Regulation:** Each Drum Node contains a Mini 5V 3A Buck Converter. The buck converts the trunk voltage to a stable 5V for the SK6812NW strip and XIAO ESP32-S3.
* **Core Node Supply:** 2× Tattu 10Ah 4S LiPo in parallel (20Ah @ 14.8V nominal) OR SlimQ 150W GaN charger (20V DC). Active ideal diode modules perform automatic OR-ing with near-zero voltage drop.
* **Total System Capacity:** ~300W peak. Up to 6 Drum Nodes simultaneously.

---

## 2. Per-Node Power Budget (The 3.0A Protocol)

The 3.0A limit applies to the **5V rail at each individual Drum Node**, not the system as a whole.

* **LED Draw (SK6812NW):** ~60mA per pixel at full White (all RGBW channels at 255).
* **Typical strip:** 60 LEDs/m × 2 strips per drum ≈ 200 LEDs per node worst case.
* **Worst-case draw:** 200 LEDs × 60mA = 12A @ 5V per node at full white.
* **The 3.0A soft cap:** In firmware, `FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500)` limits each node to 2.5A on the 5V rail during normal operation, protecting the buck converter and keeping thermal load manageable.
* **Battery mode cap:** 2.5A per node × 6 nodes = 15A from the trunk. At 14.8V, that is ~13.5A from the battery bank — well within the 20Ah Tattu pack's continuous discharge rating.
* **Wall mode cap:** Core Node detects wall power via ADC voltage divider. Broadcasts reduced brightness limit to all nodes to stay within the 150W GaN charger capacity.

---

## 3. The Scaling Algorithm (Software Governor)

Each Drum Node must implement `apply_power_limit()` before every `FastLED.show()` call:

```cpp
// Called once per frame, before FastLED.show()
void apply_power_limit(bool wall_power) {
    uint16_t ma_limit = wall_power ? 2500 : 12000;
    FastLED.setMaxPowerInVoltsAndMilliamps(5, ma_limit);
}
```

For manual scaling when a specific mode needs finer control:
1. Calculate `sum_rgb` across the entire `leds[]` array (sum of all R + G + B values).
2. Estimate current: `i_est_ma = (sum_rgb / 765.0f) * 60.0f`.
3. If `i_est_ma > 2400` (safety buffer below 2500 cap), calculate `scale = 2400.0f / i_est_ma`.
4. Apply scale to all pixels before `FastLED.show()`.

---

## 4. Thermal Safety

* **Buck converter:** Primary heat source per node at ~5W dissipation under full load. Mount against enclosure wall or heatsink surface. Apply thermal paste.
* **Prolonged static white:** The highest-stress scenario. Firmware should implement soft brightness ramping if the node has been at >80% brightness for >5 continuous minutes without a trigger event.
* **Low battery state:** Core Node monitors bus voltage via ADC divider. If bus voltage drops below 13.5V (indicating batteries near depletion), broadcast "Low Power Mode" to all nodes — cap brightness at 30% and increase heartbeat frequency to warn of impending shutdown.
* **Trunk voltage range:** Buck converters on Drum Nodes must support 9V–25V input to safely handle the full range from a depleted 4S pack (14.0V) to a freshly charged pack through the 20V wall charger (20V).