/*****************************************************************************
 * File        : ui.c
 * Component   : ui
 * Description : Top-level screen manager for the ADLS Core Node.
 *
 *               Owns three screens:
 *                 UI_SCREEN_MAIN             — live dashboard (default)
 *                 UI_SCREEN_GLOBAL_SETTINGS  — brightness, power limits, pairing
 *                 UI_SCREEN_DRUM_SETTINGS    — per-node sensitivity / retrigger
 *
 *               All screen construction is delegated to the individual modules.
 *               This file only wires navigation between them.
 *****************************************************************************/

#include "lvgl.h"
#include "ui.h"
#include "ui_main.h"
#include "ui_global_settings.h"
#include "ui_drum_settings.h"

void ui_navigate_to(ui_screen_t screen)
{
    lv_obj_t *target = NULL;
    switch (screen) {
    case UI_SCREEN_MAIN:            target = ui_main_get_screen();            break;
    case UI_SCREEN_GLOBAL_SETTINGS: target = ui_global_settings_get_screen(); break;
    case UI_SCREEN_DRUM_SETTINGS:   target = ui_drum_settings_get_screen();   break;
    default: return;
    }
    lv_scr_load(target);
}

void ui_create(void)
{
    // Build all screens up-front so navigation between them is instant
    // (no widget construction cost on first visit).
    ui_main_init();
    ui_global_settings_init();
    ui_drum_settings_init();

    // Start on the main dashboard.
    ui_navigate_to(UI_SCREEN_MAIN);
}
