#pragma once

/*****************************************************************************
 * File        : pinout.h
 * Project     : ADLS Drum Node
 * Target      : Seeed Studio XIAO ESP32-S3
 * Description : All GPIO assignments for the drum node in one place.
 *               Never hardcode GPIO numbers outside this file.
 *
 * XIAO ESP32-S3 pin-to-GPIO mapping reference:
 *   D0/A0 → GPIO 1     D1/A1 → GPIO 2     D2/A2 → GPIO 3
 *   D3/A3 → GPIO 4     D4/A4 → GPIO 5     D5/A5 → GPIO 6
 *   D6/TX → GPIO 43    D7/RX → GPIO 44    D8/SCK → GPIO 7
 *   D9/MISO → GPIO 8   D10/MOSI → GPIO 9
 *
 * !! PIN CONFLICT WARNING !!
 *   D0 and A0 are the SAME physical pin (GPIO 1) on XIAO ESP32-S3.
 *   CLAUDE.md assigns D0 to SK6812NW data and A0 to the piezo ADC.
 *   These two functions CANNOT be active simultaneously on GPIO 1.
 *   Resolution path: use an analogue MUX, or permanently assign GPIO 1
 *   to the LED strip and move the piezo to A1 (GPIO 2) in a PCB revision.
 *   For Phase 1 (no LED strip), A0/GPIO 1 is used for piezo only.
 *****************************************************************************/

#include "driver/gpio.h"

// ---------------------------------------------------------------------------
// Onboard LED — always available, no external components required
// ---------------------------------------------------------------------------

/** Built-in user LED on XIAO ESP32-S3 (yellow). Drive HIGH to illuminate. */
static const gpio_num_t PIN_LED_ONBOARD = GPIO_NUM_21;

// ---------------------------------------------------------------------------
// External status LEDs  (LED + current-limiting resistor to GND)
// ---------------------------------------------------------------------------

/** D1 (GPIO 2) — blue LED, sync / pairing activity */
static const gpio_num_t PIN_LED_BLUE = GPIO_NUM_2;

/** D2 (GPIO 3) — red LED, trigger mirror */
static const gpio_num_t PIN_LED_RED  = GPIO_NUM_3;

// ---------------------------------------------------------------------------
// User buttons  (normally-open, connect between pin and GND — PULLUP active)
// ---------------------------------------------------------------------------

/** D4 (GPIO 5) — test button: sends an immediate ESP-NOW ping when pressed */
static const gpio_num_t PIN_BTN_TEST = GPIO_NUM_5;

/** D5 (GPIO 6) — mode button: cycles lighting mode (Phase 2) */
static const gpio_num_t PIN_BTN_MODE = GPIO_NUM_6;

// ---------------------------------------------------------------------------
// Sensor + LED strip  (see conflict warning above)
// ---------------------------------------------------------------------------

/** A0 (GPIO 1) — piezo ADC input.  Phase 1 only; shared with PIN_LED_DATA. */
static const gpio_num_t PIN_PIEZO_ADC = GPIO_NUM_1;

/** D0 (GPIO 1) — SK6812NW RMT data out.  Phase 2+; shared with PIN_PIEZO_ADC. */
static const gpio_num_t PIN_LED_DATA  = GPIO_NUM_1;
