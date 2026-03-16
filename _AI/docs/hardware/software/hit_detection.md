# Logic Specification: Piezo Hit Detection

## 1. The Physics of a Drum Hit

When a drum is struck, the Piezo sensor generates a decaying AC waveform. Our goal is to extract the **Peak Velocity** as quickly as possible while ignoring the secondary vibrations (mechanical crosstalk).

### Key Phases:

1. **The Attack:** The initial spike (0–2ms). This is where the velocity is determined.
2. **The Decay:** The ringing of the head (2–50ms). We must "mask" this to prevent double-triggering.
3. **The Noise Floor:** Background vibration from other drums (Crosstalk).

---

## 2. Signal Processing Pipeline

### A. High-Speed Sampling

The ESP32-S3 must sample **ADC1_CH0** (GPIO 1) at a minimum of **2kHz** (one sample every 500 microseconds).

* **Technique:** Continuous polling or a hardware timer interrupt.
* **Normalization:** Convert raw 12-bit ADC values (0–4095) to a 0–1.0 float or 0–255 byte.

### B. The Peak Detection Algorithm (State Machine)

The Agent must implement a "Debounced Peak Finder":

1. **State IDLE:** Monitor the signal. If `Sample > Threshold`, transition to `State ATTACK`.
2. **State ATTACK:** Monitor the rising edge. Store the highest value reached within a **2ms window**.
3. **State TRIGGER:** Once the signal starts to drop, send the `Peak Velocity` to the LED Engine.
4. **State MASK:** Ignore all input for a `BlinkLength` or a fixed "Refractory Period" (e.g., 30ms) to prevent double-hits. Transition back to `IDLE`.

---

## 3. Dynamic Thresholds & Sensitivity

These variables are received from the **Core Node** and stored in the Drum Node's local memory:

* **Sensitivity (`sensitivity`):** Adjusts the `Threshold`. High sensitivity = triggers on light touches (ghost notes).
* **Retrigger Guard (`retrigger_ms`):** The length of the `MASK` state. Critical for fast double-kick or blast beats.
* **Curve Mapping:** Maps the linear ADC peak to a non-linear velocity curve (e.g., Logarithmic) to make the LED response feel more natural to the human eye.

---

## 4. Advanced Logic: The "Rimshot" Vector

If we eventually add a second Piezo (one on the head, one on the shell), we will use **Vector Similarity (Dot Product)**:

* **Vector A:** `[Head_Sensor, Shell_Sensor]`
* **Logic:** A high value on both sensors simultaneously indicates a **Rimshot**. A high value on only the head indicates a **Center Hit**.

---

## 5. Performance Guardrails (First Principles)

* **Zero Latency Path:** The moment the `TRIGGER` state is reached, the LED `flash()` function must be called **before** sending the ESP-NOW packet to the Core Node.
* **Integer Math:** Use integer scaling (0–255) instead of floating-point math where possible to keep the CPU cycles available for the FastLED render loop.
* **Non-Blocking Telemetry:** * The `esp_now_send` call for telemetry must be initiated after the `TRIGGER` state has fired the LEDs. It must be executed as a non-blocking background task to ensure the next sensor sample isn't delayed.

---

### **The "Senior" Checklist for the Agent**

1. **ADC Calibration:** Use `esp_adc_cal` to account for the ESP32’s non-linear ADC response.
2. **Smoothing:** Implement a simple **Moving Average Filter** (3 samples) to remove electrical noise before checking the threshold.
3. **Hysteresis:** Ensure the threshold to *start* a hit is higher than the threshold to *end* a hit to prevent jitter.
