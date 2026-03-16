/**
 * display.h (8/24/24)
 * 
 * Header file for display.cpp - Manages cmds to be sent to the Nextion Display
 **/

#ifndef Display_h
#define Display_h

#include <Arduino.h>
#include <Nextion.h>

#define DRUM_SETTINGS_SENS_ID "trigSensVal"
#define DRUM_SETTINGS_SENS_FACTOR_ID "trigFactorVal"
#define DRUM_SETTINGS_SENS_TIME_ID "trigTimeVal"
#define DRUM_SETTINGS_DURATION_ID "trigDurVal"

#define DRUM_SETTINGS_NUM_LEDS_ID "numLedsVal"
#define DRUM_SETTINGS_SAVED_ID "savedLabel"

#define GLOBAL_SETTINGS_BLINK_ID "blinkLengthVal"
#define GLOBAL_SETTINGS_FADE_DELAY_ID "fadeDelayVal"
#define GLOBAL_SETTINGS_FADE_STEP_ID "fadeStepVal"
#define GLOBAL_SETTINGS_FADE_INTERVAL_ID "fadeInterVal"
#define GLOBAL_SETTINGS_CIRCLE_TIME_ID "circleTimeVal"
#define GLOBAL_SETTINGS_SAVED_ID "savedLabel_g"

class Display {
  public: 
    static void SetVisibility(String id, bool isVisible);
    static void SetObjVisibility(NexObject obj, bool isVisible);
    static void SetTextColor(String id, uint16_t val);
    static void SetBgColor(String id, uint16_t val);
    static void SetBorderColor(String id, uint16_t val);
    static void SetBorderThickness(String id, uint8_t val);
    static void SetText(String id, String val);
    static void SetText(String id, uint8_t val);
    static void SetText(String id, int val);
    static void SetNumber(String id, uint8_t val);

    static void ShowMainPage();
    static void ShowSettingsPage();  // Might need to pass pointer instead of actual object
    static void ShowGlobalSettingsPage();  // Might need to pass pointer instead of actual object

    static void SetCurrDrumSetting(int val);
    static void ShowDrumSavedIndicator(bool hide);

    static void SetCurrGlobalSetting(int val);
    static void ShowGlobalSavedIndicator(bool hide);

  // private:
    static void EndCommand();
};

#endif