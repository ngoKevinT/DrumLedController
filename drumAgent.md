Project Manifest: DrumNode-LED (Agentic Edition)Status: Design Phase / 2026
Architect: Senior Software Engineer (User)Hardware Target: ESP32-WROOM / ESP32-S31. 

System Architecture
The project uses a Hybrid Supervisory Architecture to balance real-time performance with high-level intelligence.
Layer 1 (The Edge): ESP32 Microcontrollers (Core Node + 4 Drum Nodes). Handles deterministic C++ logic, piezo trigger interrupts, and WS2812B LED driving via FastLED.
Layer 2 (The Bridge): Python-based Host (PC/Raspberry Pi). Manages Serial/WebSocket telemetry and acts as the "hands" for the Agent.
Layer 3 (The Agent): The "Senior Engineer Entity." Uses an LLM (Claude/Gemini/Llama) to reason about power constraints, generat firmware code, and interpret artistic "vibes" into lighting parameters.
2. Hardware & Power ConstraintsPower Source: 12V Battery.Regulation: 12V-to-5V Buck Converter (Master output).Limit: 3.0 Amperes (15 Watts) max total draw for the entire system.Shared Rail: Both the ESP32 and LED strips are on the same 5V output.Risk: Voltage sag during "Full White" flashes can brown oeut the ESP32.Monitoring: INA219 Current/Voltage sensor installed at the common rail junction.3. Lighting Mode DefinitionsThe system supports 7 baseline modes, designed to be Parametric (controllable via JSON config packets):IDMode NameDescription1StaticSingle base color; blinks to hitColor on piezo trigger.2FadeSingle color; flashes hitColor then decays back to baseColor.3RainbowCycles through the hue spectrum with every hit; blinks on trigger.4Rainbow FadeCycles hue on hit; flashes and decays slowly.5Static White w/ RainbowBackground is White; trigger causes a cycling rainbow pulse.6White w/ Rainbow FadeBackground is White; trigger flashes rainbow and decays.7Static Color w/ WhiteBackground is a base color; blinks White for maximum contrast.4. Agentic Logic & ValidationThe Agent is programmed to manage the Safety-Critical and Creative layers:The "Vibe" Translation: The Agent converts natural language (e.g., "90s Rave") into specific values for hue, saturation, brightness, and fadeSpeed.The Hardware Watchdog:Heuristic: $Total\_Current = (LED\_Count \times Per\_Pixel\_mA) + 150mA$.Action: If current > 2.8A, the Agent must refactor the requested mode to use lower global brightness.Firmware Verification: The Agent uses a C++ Test Harness (Mock HAL) to compile and "dry-run" new lighting modes on the PC before flashing them to the ESP32.5. State Dictionary (JSON Structure)The Agent communicates with the ESP32 using the following schema:JSON{
  "node_id": "bass_drum_22",
  "mode": 7,
  "params": {
    "base_hsv": [160, 255, 50],
    "hit_hsv": [0, 0, 255],
    "fade_rate": 15,
    "brightness_limit": 200
  },
  "safety": {
    "max_amps": 3.0,
    "temp_cutoff_c": 65
  }
}