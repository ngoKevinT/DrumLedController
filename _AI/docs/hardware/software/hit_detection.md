# Logic Specification: Piezo Hit Detection

## 1. The Physics of a Drum Hit

When a drum is struck, the piezo sensor produces a decaying AC waveform. The goal is to extract the **peak velocity** as fast as possible while ignoring the vibrations that follow (mechanical crosstalk).

1. **Attack:** the initial spike, 0–2 ms. Velocity is determined here.
2. **Decay:** the drum head rings for 2–50 ms. This must be masked out to prevent double triggers.
3. **Noise floor:** background vibration from the other drums (crosstalk).

---

## 2. Analog Front End (per channel)

```
Piezo ──┬── R_load (1 MΩ start, tune 100k–1M) ── GND
        └── 10kΩ series ──┬── BZT52C3V3S (K→sig) ── GND
                          ├── B5817WS   (K→sig) ── GND
                          └── XIAO ADC1 pin (A0 = ch1, A1 = ch2)
```

* **Load resistor on BOTH channels.** Without one, an unplugged channel floats and picks up noise from the LED data lines, which causes false triggers. It also bleeds off the piezo's DC charge.
* **Signal level target:** tune R_load on the bench so the **hardest hit reaches about 2.5V at the pin**, roughly 80% of the ADC's ~3.1V full scale at 12 dB attenuation.
  * The clamp diodes then act only during faults and never in normal playing.
  * The BZT52C3V3's soft knee would otherwise compress loud hits together, losing velocity detail.
* **Plug detect:** with a load resistor fitted, a channel with nothing plugged in reads near zero. Firmware treats a channel that stays below the noise floor as "not connected", so it needs no dedicated detect pin.

---

## 3. Signal Processing Pipeline

### A. Sampling

* **ESP-IDF ADC continuous mode** (`adc_continuous_*`, DMA-driven) on ADC1. ADC1 keeps working while WiFi is on; ADC2 doesn't.
* **≥ 10 kHz per channel**, so 20 kHz total for two channels. The S3 supports far more than that.
  * Why so fast: the attack lasts only 1–2 ms. At 2 kHz the attack window holds only 2–4 samples and the peak is easily missed. At 10 kHz it holds 10–20.
* **Calibration:** `adc_cali_*` with the curve-fitting scheme. `esp_adc_cal` is deprecated in ESP-IDF 5.x.
* **Smoothing:** a 3-sample moving average before the threshold check. That's 0.3 ms at 10 kHz.

### B. Peak Detection State Machine

1. **IDLE:** when `sample > threshold_on`, move to ATTACK.
2. **ATTACK:** track the highest sample within a **1 ms window**.
3. **TRIGGER:** at the end of the window, or as soon as the signal falls, pass the peak velocity to the LED engine **immediately**.
4. **MASK:** ignore input for `retrigger_ms` (default **25 ms**, configurable 0–500 ms from the Core Node). Return to IDLE only once `sample < threshold_off`.

**Hysteresis:** `threshold_off = threshold_on × 3/4`. This prevents chatter right at the threshold.

---

## 4. Dynamic Thresholds & Sensitivity

These come from the Core Node in `espnow_config_pkt_t` (see `espnow_transport.h`) and are saved in drum-node NVS:

* **`threshold`** (0–1023): on the wire, a 10-bit scale. The firmware maps it to the 12-bit ADC range (×4). A lower value means more sensitive, picking up ghost notes.
* **`retrigger_ms`** (0–500): length of the MASK state. Critical for fast double-kick and blast beats.
* **`hit_curve`** (LINEAR / LOG / EXP): maps the linear ADC peak to perceived brightness.

---

## 5. Advanced: Rimshot Detection (future)

With two piezos, one on the head (ch1) and one on the shell (ch2):

* Vector `[head, shell]`. High on both at once means a **rimshot**. High on the head only means a **center hit**.

---

## 6. Performance Guardrails

* **Latency budget:** detection takes ≤ 1 ms of the ≈ 6 ms target (full budget in `led_engine.md` §3).
* **LEDs first:** at TRIGGER, the LED update is queued **before** ESP-NOW telemetry is sent.
* **Integer math only** in the sampling and detection path.
* **Non-blocking telemetry:** `esp_now_send` is called from a lower-priority task, fed by a queue.
* **Task priority:** the trigger task has the highest application priority and is pinned to the core that isn't running WiFi.
