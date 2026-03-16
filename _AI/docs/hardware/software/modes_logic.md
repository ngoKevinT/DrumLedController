# Logic Specification: The 7 Artistic Modes

Each mode is a unique mathematical "Vibe" that interprets velocity and time differently.

| Mode ID | Name | First Principle Behavior |
| :--- | :--- | :--- |
| **0** | **Pure Reactive** | Instant white flash, linear decay to black. |
| **1** | **Heat Map** | Color shifts: Blue (soft hit) -> Yellow (med) -> Red (hard). |
| **2** | **90s Rave** | High-velocity hits trigger a 10Hz "Strobe" for 100ms. |
| **3** | **Ghost Note** | Only hits above 50% velocity trigger LEDs. Background is a slow "Breath." |
| **4** | **Spectrum** | Each hit advances the Hue by 15 degrees (Color cycling). |
| **5** | **Trailing Edge** | Hit starts at the sensor and "travels" outward to the end of the strip. |
| **6** | **The Void** | Inverted logic: LEDs are ON; hits create a "Black Hole" that fades back to color. |

## 4. Mode Configuration
Modes are stored as an `enum` and a `switch` statement in the `led_engine.md`.