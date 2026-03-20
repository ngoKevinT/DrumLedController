# ADLS — Automated Drum Lighting System
### Nexus PDU · v5

A distributed, real-time reactive LED lighting system for a drum kit. Each drum runs an autonomous edge controller that detects hits via piezo sensor and drives an SK6812NW RGBW strip locally. A central Core Node handles power distribution, wireless configuration, and a live Mission Control dashboard.

---

## Hardware at a Glance

| Role | Board | Notes |
|---|---|---|
| Core Node | Waveshare ESP32-S3-Touch-LCD-7B | 1024×600 · capacitive touch · LVGL v9 |
| Drum Node (×N) | Seeed Studio XIAO ESP32-S3 | One per drum · autonomous edge logic |
| LED Strip | SK6812NW · 60 LED/m | 5V · RGBW · Natural White ~4000K |
| Transport | 14.8–20V trunk via SP13 connectors | Local 5V buck conversion at each node |
| Battery | 2× Tattu 10Ah 4S LiPo (parallel) | 20Ah · 14.8V nominal |
| Charger | SlimQ 150W GaN | 20V DC · UPS topology via ideal diodes |

---

## Project Structure

```
/
├── README.md               ← You are here
├── manifest.md             ← Agent handover protocol, constraints, architecture map
├── todo.md                 ← Phase tracker and task checklist
├── docs/
│   ├── hardware/
│   │   ├── pinout.md       ← GPIO assignments for Core Node and Drum Node
│   │   ├── power_specs.md  ← Power architecture, 3.0A per-node protocol, thermal rules
│   │   └── architecture.md ← System block diagram and design decisions
│   └── software/
│       ├── hit_detection.md
│       ├── led_engine.md
│       └── ui_lvgl.md
├── .agent/
│   └── standards.md        ← Coding standards, naming conventions, agent rules
├── src/
│   ├── core_node/          ← Waveshare 7B firmware (LVGL, ESP-NOW master)
│   └── drum_node/          ← XIAO firmware (FastLED, piezo trigger, ESP-NOW slave)
└── assets/
    └── palettes/           ← Color definitions per lighting mode
```

---

## Key Design Principles

**Edge autonomy.** Drum Nodes trigger and light independently. They do not wait for the Core Node to authorize a flash — the hit-to-LED path is entirely local and targets < 10ms latency.

**UPS power topology.** The Core Node runs on wall power when available and switches to battery seamlessly via active ideal diode OR-ing. Batteries charge in the background while the system runs.

**Transport high, convert local.** 14.8–20V travels over thin 18AWG trunk cables to minimize voltage drop and wire weight. Each Drum Node steps down to 5V locally via a dedicated buck converter.

**3.0A per-node soft cap.** `FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500)` enforces a per-node brightness ceiling. The Core Node broadcasts a tighter cap when running on wall power to stay within the 150W GaN charger budget.

---

## Quick Reference

| Document | What's in it |
|---|---|
| `manifest.md` | Hard constraints, agent protocol, active project state |
| `todo.md` | Full phase-by-phase task checklist |
| `docs/hardware/pinout.md` | Every GPIO assignment with protection circuit notes |
| `docs/hardware/power_specs.md` | Power architecture, scaling algorithm, thermal limits |
| `docs/hardware/architecture.md` | System block diagram, confirmed design decisions |
| `.agent/standards.md` | C++ style, real-time rules, LVGL integration standards |

---

## References

- [Waveshare ESP32-S3-Touch-LCD-7B Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7B)
- [LVGL v9 Documentation](https://docs.lvgl.io/master/index.html)
- [LVGL Examples](https://github.com/lvgl/lvgl/tree/master/examples)
- [FastLED SK6812 RGBW](https://github.com/FastLED/FastLED)
- [Seeed XIAO ESP32-S3 Pinout](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
