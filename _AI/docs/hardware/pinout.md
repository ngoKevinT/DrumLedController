# Hardware Pinout: Distributed Drum Lighting System

## 1. System Architecture
* **Core Node:** Central Command (LilyGo T-Display-S3).
* **Drum Node:** Edge Controller (Seeed Studio XIAO ESP32-S3).
* **Communication:** ESP-NOW (Low Latency Peer-to-Peer).

---

## 2. Drum Node (Seeed Studio XIAO ESP32-S3)
*Role: Local sensor sampling and LED driving.*

| Pin | GPIO | Function | Description |
| :--- | :--- | :--- | :--- |
| **A0** | 1 | ADC1_CH0 | **Piezo Trigger (+)** - High-speed analog input. |
| **D10** | 10 | RMT/DMA | **LED Data Out** - Connection to WS2812B/SK6812. |
| **5V** | 5V | Power | Input from 3.0A rail (Share ground with LEDs). |
| **GND** | GND | Ground | Common ground for MCU, Sensor, and LEDs. |

---

## 3. Core Node (LilyGo T-Display-S3)
*Role: System orchestration and LVGL user interface.*

| Pin/Peripheral | GPIO | Function | Description |
| :--- | :--- | :--- | :--- |
| **LCD Parallel Bus**| 39-42, 45, 48 | 8-bit Data | Internal - ST7789 Display interface. |
| **LCD Backlight** | 15 | PWM | Internal - Must be HIGH to view screen. |
| **Button A (Right)**| 14 | Input | On-board - User navigation (Menu/Next). |
| **Button B (Left)** | 0 | Input | On-board - User navigation (Select/Enter). |
| **Battery Sense** | 4 | ADC | Internal - Voltage monitoring (100k/100k divider). |
| **Expansion IO** | 1, 2, 10 | GPIO | Available for physical Sync cables if needed. |

---

## 4. Electrical Safety Notes
* **Level Shifting:** The S3 pins are 3.3V. If LED flickering occurs, use a 74HCT125 level shifter to 5V.
* **Piezo Protection:** Place a 1M Ohm resistor in parallel with the Piezo and a 3.3V Zener diode across A0/GND to clip high-voltage spikes.
* **Power:** Do not draw LED current through the XIAO or T-Display traces. Connect the 5V 3.0A supply directly to the LED strip rails.