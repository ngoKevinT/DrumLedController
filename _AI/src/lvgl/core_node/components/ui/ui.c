/*****************************************************************************
 * File        : ui.c
 * Component   : ui
 * Description : Root screen for the ADLS Core Node.
 *
 *               Boilerplate hello-world layout:
 *                 - A centred "Hello, World!" label (Montserrat 20)
 *                 - A count button below it; each tap increments a counter
 *                   and re-renders the label as "Count: X"
 *
 *               This file is the ONLY place LVGL widget construction lives.
 *               Hardware logic (FastLED, ESP-NOW, sensors) must never be
 *               called from here — use the shared state struct / event bridge
 *               defined in the project's Event Bridge module (TBD).
 *****************************************************************************/

#include "lvgl.h"
#include "ui.h"

// ---------------------------------------------------------------------------
// Module-private state
// ---------------------------------------------------------------------------

static lv_obj_t *lbl_count = NULL;   // Label inside the counter button
static uint32_t  click_count = 0;    // Tap counter — never resets during runtime

// ---------------------------------------------------------------------------
// Event callbacks
// ---------------------------------------------------------------------------

/**
 * Fired on every CLICKED event for the counter button.
 * Increments the counter and re-renders the button label in-place so that
 * only the label's dirty region is re-drawn (no full-screen invalidation).
 */
static void btn_count_cb(lv_event_t *e)
{
    click_count++;
    // lv_label_set_text_fmt only redraws the label area, not the whole screen.
    lv_label_set_text_fmt(lbl_count, "Count: %u", (unsigned int)click_count);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ui_create(void)
{
    lv_obj_t *scr = lv_scr_act();

    // ------------------------------------------------------------------
    // "Hello, World!" label — top-centre of the 1024x600 canvas
    // ------------------------------------------------------------------
    lv_obj_t *lbl_hello = lv_label_create(scr);
    lv_label_set_text(lbl_hello, "Hello, World!");
    // Montserrat 20 is enabled via CONFIG_LV_FONT_MONTSERRAT_20 in sdkconfig.defaults
    lv_obj_set_style_text_font(lbl_hello, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_hello, LV_ALIGN_TOP_MID, 0, 60);

    // ------------------------------------------------------------------
    // Counter button — horizontally centred, below the hello label
    // ------------------------------------------------------------------
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 220, 70);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn, btn_count_cb, LV_EVENT_CLICKED, NULL);

    // Label is a child of the button so it centres automatically
    lbl_count = lv_label_create(btn);
    lv_label_set_text(lbl_count, "Count: 0");
    lv_obj_set_style_text_font(lbl_count, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_count);
}
