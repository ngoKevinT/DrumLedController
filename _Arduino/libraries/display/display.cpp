/**
 * display.cpp (8/24/24)
 * 
 * Main class that will perform commands to update the Nextion display.
 **/
#include "display.h"

/*!
 *  Change the visibility of the element for the given id.
 */
void Display::SetVisibility(String id, bool isVisible) {
    // Serial.print("vis " + id + (isVisible ? ",1" : ",0"));
    String cmd = String("vis ");
    cmd += id;
    cmd += isVisible ? ",1" : ",0";
    Serial.print(cmd);
    EndCommand();
}

void Display::SetObjVisibility(NexObject obj, bool isVisible) {
    SetVisibility(String(obj.getObjCid()), isVisible);
}

/*!
 *  Change the text color for the elemnt of the given id.
 */
void Display::SetTextColor(String id, uint16_t val) {
    Serial.print(id + ".pco=");
    Serial.print(val, DEC);
    EndCommand();
}

/*!
 *  Change the background color for the element of the given id.
 */
void Display::SetBgColor(String id, uint16_t val) {
    Serial.print(id + ".bco=");
    Serial.print(val, DEC);
    EndCommand();
}

/*!
 *  Change the border thickness for the element of the given id.
 */
void Display::SetBorderColor(String id, uint16_t val) {
    Serial.print(id + ".borderc=");
    Serial.print(val, DEC);
    EndCommand();
}

/*!
 *  Change the border thickness for the element of the given id.
 */
void Display::SetBorderThickness(String id, uint8_t val) {
    Serial.print(id + ".borderw=");
    Serial.print(val, DEC);
    EndCommand();
}

/*!
 *  Set the text for the text element of the given id.
 */
void Display::SetText(String id, String val) {
    EndCommand();
    Serial.print(id + ".txt=\"" + val + "\"");
    EndCommand();
}

/*!
 *  Set the text for the text element of the given id.
 */
void Display::SetText(String id, uint8_t val) {
    EndCommand();
    Serial.print(id + ".txt=\"");
    Serial.print(val, DEC);
    Serial.print("\"");
    EndCommand();
}

/*!
 *  Set the text for the text element of the given id.
 */
void Display::SetText(String id, int val) {
    Serial.print(id + ".txt=\"");
    Serial.print(val, DEC);
    Serial.print("\"");
    EndCommand();
}

/*!
 *  Set the number for the number element of the given id.
 */
void Display::SetNumber(String id, uint8_t val) {
    Serial.print(id + ".val=");
    Serial.print(val, DEC);
    EndCommand();
}

/*!
 *  Show the main page (page 1)
 */
void Display::ShowMainPage() {
    Serial.print("page 1");
    EndCommand();
}

/*!
 *  Show the settings page (page 2)
 */
void Display::ShowSettingsPage() {
    Serial.print("page 2");
    EndCommand();
}

/*!
 *  Show the global settings page (page 3)
 */
void Display::ShowGlobalSettingsPage() {
    Serial.print("page 3");
    EndCommand();
}

/*!
 *  Show and hide the "Saved!" indicator on the settings page.
 */
void Display::ShowDrumSavedIndicator(bool hide = false) {
    if (!hide) {
        SetVisibility(DRUM_SETTINGS_SAVED_ID, true);
        delay(1000);
    }
    SetVisibility(DRUM_SETTINGS_SAVED_ID, false);
}

void Display::SetCurrDrumSetting(int val) {
    // SetBorderThickness(DRUM_SETTINGS_SENS_ID, 2);
    // SetBorderThickness(DRUM_SETTINGS_SENS_FACTOR_ID, 2);
    // SetBorderThickness(DRUM_SETTINGS_SENS_TIME_ID, 2);
    // SetBorderThickness(DRUM_SETTINGS_NUM_LEDS_ID, 2);
    SetBgColor(DRUM_SETTINGS_SENS_ID, 65535);
    SetBgColor(DRUM_SETTINGS_SENS_FACTOR_ID, 65535);
    SetBgColor(DRUM_SETTINGS_SENS_TIME_ID, 65535);
    // SetBgColor(DRUM_SETTINGS_NUM_LEDS_ID, 65535);
    SetBgColor(DRUM_SETTINGS_DURATION_ID, 65535);

    switch(val) {
        case 0:
            // SetBorderThickness(DRUM_SETTINGS_SENS_ID, 20);
            SetBgColor(DRUM_SETTINGS_SENS_ID, 40339);
            break;
        case 1:
            // SetBorderThickness(DRUM_SETTINGS_SENS_FACTOR_ID, 20);
            SetBgColor(DRUM_SETTINGS_SENS_FACTOR_ID, 40339);
            break;
        case 2:
            // SetBorderThickness(DRUM_SETTINGS_SENS_TIME_ID, 20);
            SetBgColor(DRUM_SETTINGS_SENS_TIME_ID, 40339);
            break;
        case 3:
            // SetBorderThickness(DRUM_SETTINGS_NUM_LEDS_ID, 20);
            // SetBgColor(DRUM_SETTINGS_NUM_LEDS_ID, 40339);
            SetBgColor(DRUM_SETTINGS_DURATION_ID, 40339);
            break;

        default:
            break;
    }
}

/*!
 *  Show and hide the "Saved!" indicator on the settings page.
 */
void Display::ShowGlobalSavedIndicator(bool hide = false) {
    if (!hide) {
        SetVisibility(GLOBAL_SETTINGS_SAVED_ID, true);
        delay(1000);
    }
    SetVisibility(GLOBAL_SETTINGS_SAVED_ID, false);
}

void Display::SetCurrGlobalSetting(int val) {
    // SetBorderThickness(GLOBAL_SETTINGS_BLINK_ID, 2);
    // SetBorderThickness(GLOBAL_SETTINGS_FADE_DELAY_ID, 2);
    // SetBorderThickness(GLOBAL_SETTINGS_FADE_STEP_ID, 2);
    // SetBorderThickness(GLOBAL_SETTINGS_CIRCLE_TIME_ID, 2);
    SetBgColor(GLOBAL_SETTINGS_BLINK_ID, 65535);
    SetBgColor(GLOBAL_SETTINGS_FADE_DELAY_ID, 65535);
    SetBgColor(GLOBAL_SETTINGS_FADE_STEP_ID, 65535);
    SetBgColor(GLOBAL_SETTINGS_FADE_INTERVAL_ID, 65535);
    SetBgColor(GLOBAL_SETTINGS_CIRCLE_TIME_ID, 65535);

    switch(val) {
        case 0:
            // SetBorderThickness(GLOBAL_SETTINGS_BLINK_ID, 20);
            SetBgColor(GLOBAL_SETTINGS_BLINK_ID, 40339);
            break;
        case 1:
            // SetBorderThickness(GLOBAL_SETTINGS_FADE_DELAY_ID, 20);
            SetBgColor(GLOBAL_SETTINGS_FADE_DELAY_ID, 40339);
            break;
        case 2:
            // SetBorderThickness(GLOBAL_SETTINGS_FADE_STEP_ID, 20);
            SetBgColor(GLOBAL_SETTINGS_FADE_STEP_ID, 40339);
            break;
        case 3:
            SetBgColor(GLOBAL_SETTINGS_FADE_INTERVAL_ID, 40339);
            break;
        case 4:
            // SetBorderThickness(GLOBAL_SETTINGS_CIRCLE_TIME_ID, 20);
            SetBgColor(GLOBAL_SETTINGS_CIRCLE_TIME_ID, 40339);
            break;
        default:
            break;
    }
}

/*!
 *  Commands required by Nextion to finish a given command.
 */
void Display::EndCommand() {
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);

    // Serial.println("\nEnd Command");
    Serial.write("\nEnd Command\n");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
}