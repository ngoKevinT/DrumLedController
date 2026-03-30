#pragma once

/*****************************************************************************
 * File        : pinout.h
 * Project     : ADLS Drum Node
 * Target      : Seeed Studio XIAO ESP32-S3
 * Description : All GPIO assignments for the drum node.
 *               Source of truth: todo.md "Confirmed Design Decisions".
 *               Never hardcode GPIO numbers outside this file.
 *
 * XIAO ESP32-S3 pin-to-GPIO mapping reference:
 *   D0/A0 → GPIO 1    D1/A1 → GPIO 2    D2/A2 → GPIO 3
 *   D3/A3 → GPIO 4    D4/A4 → GPIO 5    D5/A5 → GPIO 6
 *   D6/TX → GPIO 43   D7/RX → GPIO 44   D8/SCK → GPIO 7
 *   D9/MISO → GPIO 8  D10/MOSI → GPIO 9
 *
 * Layout rationale (EMI / noise isolation):
 *   LEFT  side (A0–D5): analog quiet — trigger inputs, ring detect, buttons
 *   RIGHT side (D6–D10): digital output — LED data, status LEDs
 *   Ground pour between rows acts as guard band.
 *****************************************************************************/

#include "driver/gpio.h"

// ---------------------------------------------------------------------------
// Onboard LED
// XIAO ESP32-S3 built-in user LED (green).  ACTIVE LOW — set LOW to light.
// Replaces a discrete green "logic rail live" indicator; no external component.
// ---------------------------------------------------------------------------
static const gpio_num_t PIN_LED_ONBOARD = GPIO_NUM_21;

// ---------------------------------------------------------------------------
// Status LEDs  (right side — away from analog trigger lines)
// External 5mm LEDs: LED anode → pin, cathode → 220Ω → GND
// ---------------------------------------------------------------------------

/** D5 (GPIO 6) — blue, left side. Blink=searching, solid=paired, 500ms blink=re-pairing */
static const gpio_num_t PIN_LED_BLUE = GPIO_NUM_6;

/** D8 (GPIO 7) — red, right side. 80ms flash on trigger, solid during continuous noise */
static const gpio_num_t PIN_LED_RED  = GPIO_NUM_7;

// ---------------------------------------------------------------------------
// User buttons  (left side — quiet digital, away from LED switching noise)
// Normally-open SPST, connect pin to GND — INPUT_PULLUP active inside firmware
// ---------------------------------------------------------------------------

/** A2/D2 (GPIO 3) — test button. Single press fires trigger manually.
 *  5-second hold triggers re-pairing mode. */
static const gpio_num_t PIN_BTN_TEST = GPIO_NUM_3;

/** D3 (GPIO 4) — mode button. Cycles local lighting mode. */
static const gpio_num_t PIN_BTN_MODE = GPIO_NUM_4;

// ---------------------------------------------------------------------------
// Trigger inputs  (left side — short analog traces, minimal switching coupling)
// Signal protection chain: 10kΩ series → 1N4728A Zener + 1N5817 Schottky at pin
// ---------------------------------------------------------------------------

/** A0/D0 (GPIO 1) — trigger channel 1, TRS Tip.  Primary piezo input. */
static const gpio_num_t PIN_TRIGGER_CH1 = GPIO_NUM_1;

/** A1/D1 (GPIO 2) — trigger channel 2, TRS Ring.  Second piezo or hi-hat choke.
 *  Hardware: 100kΩ pull-down to GND after clamp diodes.  When no TRS plug is
 *  present the ring line floats low → software reads near-zero ADC → no trigger.
 *  No dedicated ring-detect pin needed; jack is standard 3-lug (no normalling). */
static const gpio_num_t PIN_TRIGGER_CH2 = GPIO_NUM_2;

// ---------------------------------------------------------------------------
// LED strip data  (right side)
// Signal chain: pin → 330Ω series → PRTR5V0U2X ESD diode → strip DIN
// ---------------------------------------------------------------------------

/** D9 (GPIO 8) — primary SK6812NW data output, connected to strip Data1 pad */
static const gpio_num_t PIN_LED_DATA    = GPIO_NUM_8;

/** D10 (GPIO 9) — secondary data output, connected to strip Data2 pad (future) */
static const gpio_num_t PIN_LED_DATA2   = GPIO_NUM_9;

// ---------------------------------------------------------------------------
// Reserved / unassigned
// ---------------------------------------------------------------------------

/** D4 (GPIO 5) — reserved for NTC thermistor (Phase 7 thermal management) */
static const gpio_num_t PIN_NTC         = GPIO_NUM_5;

/** D6/TX (GPIO 43) and D7/RX (GPIO 44) — kept free for UART debug output.
 *  Do not assign to peripherals. */
