# Project Mission Log: ADLS Development

## Phase 1: Communication & Discovery (The Backbone)
* [ ] **ESP-NOW Base Implementation:**
    * Initialize ESP-NOW on both XIAO and T-Display targets.
    * Define `struct DrumState` (Sensitivity, Mode, Fade, BlinkLength).
* [ ] **Auto-Discovery Protocol:**
    * [ ] Drum Node: Broadcast "Pairing" packet on boot if unassigned.
    * [ ] Core Node: Implement "Add Node" listener to register unique MAC addresses.
    * [ ] Implement "Identity Flash": Core Node pings a Drum Node, and it flashes white to identify which drum it is (e.g., "Snare").

## Phase 2: Drum Node Performance (The Edge)
* [ ] **High-Speed Trigger Logic:**
    * Sample ADC1 at > 1kHz.
    * Implement Peak Detection with adjustable threshold (Sensitivity).
* [ ] **FastLED Local Logic:**
    * Implement "Trigger-to-Action" latency path (< 10ms).
    * Create base modes: `STATIC_GLOW`, `TRIGGER_FLASH`, `VELOCITY_FADE`.
* [ ] **Async Reporting:**
    * Send hit notification back to Core Node *after* LED update to prevent visual lag.

## Phase 3: Core Node UI (The Brain)
* [ ] **LVGL Setup:**
    * Port T-Display-S3 driver for LVGL 8.3+.
    * Implement "Dashboard": Show connected nodes and hit intensity in real-time.
* [ ] **Configuration Menus:**
    * [ ] Sensitivity Slider (Writes to Drum Node).
    * [ ] Color/Mode Selector (Writes to Drum Node).
    * [ ] Global Power Master (Enforces 3.0A limit across all nodes).

## Phase 4: Creative Synthesis (The "7 Modes")
* [ ] Implement advanced artistic modes:
    * 1. "90s Rave" (High saturation, rapid blink).
    * 2. "Ghost Note" (Faint glow on low velocity, burst on high).
    * 3. "Heat Map" (Color shifts from Blue to Red based on hit frequency).
    * ... (Remaining modes to be defined by Agent).

## Phase 5: Reliability & Persistence
* [ ] **NVS Storage:** Save node pairings and configurations on the Core Node's flash memory.
* [ ] **Error Handling:** Implement "Heartbeat" check; if a Drum Node disconnects, mark it "OFFLINE" on the Core Node display.