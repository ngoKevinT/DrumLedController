# Logic Specification: LVGL State Machine

## 1. UI Hierarchy
The interface is designed as a **Nested Finite State Machine (FSM)**:
* **STATE_DASHBOARD:** Real-time view of all Drum Nodes (Battery, Signal, Recent Hits).
* **STATE_NODE_CONFIG:** Focus on a single node to tune `sensitivity` or `blink_length`.
* **STATE_GLOBAL_SETTINGS:** Brightness master, 3.0A limit toggle, and Node Pairing menu.

## 2. Input Mapping (LilyGo Buttons)
* **Button A (Short Press):** Cycle through menu items (Navigation).
* **Button B (Short Press):** Confirm/Enter sub-menu.
* **Button B (Long Press):** Return to Dashboard (Escape).

## 3. Data Synchronization
* When a value is changed in LVGL, the Core Node must flag that node for a "Sync Update."
* Updates are pushed via ESP-NOW only when a value changes to minimize airtime "Resource Debt."