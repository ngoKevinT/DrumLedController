# Specification: Power Management & The 3.0A Protocol

## 1. The Physics of the Limit
* **Power Supply:** 5V DC / 3.0A (15 Watts Total).
* **LED Draw (WS2812B):** ~60mA per pixel at full White (RGB 255,255,255).
* **The "Hard Wall":** With 3.0A, we can only power **50 LEDs** at full white simultaneously ($3000 / 60 = 50$).

## 2. The Scaling Algorithm (Software Governor)
The Agent must implement a `get_power_multiplier()` function:
1. Calculate `Sum_RGB` of the entire `leds[]` array.
2. Estimate Current: $I_{est} = (Sum\_RGB / 765) * 60mA$.
3. If $I_{est} > 2800mA$ (Safety Buffer), calculate `ScaleFactor = 2800 / I_{est}`.
4. Apply `ScaleFactor` to all pixels before `FastLED.show()`.

## 3. Thermal Safety
* **Passive Cooling:** Ensure the Drum Nodes are mounted with airflow; prolonged high-brightness sequences can cause the XIAO S3 to throttle or the LED adhesive to fail.
* **Low Battery State:** If the Core Node detects Battery < 3.4V, it must broadcast a "Low Power Mode" command to all nodes, capping brightness at 30%.