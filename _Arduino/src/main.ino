
/**
 * main.ino (9/11/2024)
 * 
 * Main application. Manages drums, button listener, and trigger listener. Sets up the main arduino project.
 * Built for ESP32 
 **/
#include <Arduino.h>
#include <HardwareSerial.h>
#include <drum.h>
#include <FastLED.h>
#include <Nextion.h>
#include <Common.h>

#define NUM_DRUMS 4

// Trigger pins
#pragma region Trigger pins
const int trigger_0 = 13; // GPIO13 - P15/P13
const int trigger_1 = 33; // GPIO33 - P8/P33
const int trigger_2 = 34; // GPIO34 - P5/P34
const int trigger_3 = 35; // GPIO35 - P6/P35
#pragma endregion

// NOTE: Something's weird with GPIO 22 OR 27 (Pins 31 or 11)
// Maybe something weird with accesing leds in array?

// LED Strip Pins
#pragma region LED Strip Pins
// Data Pins
const int snare_led_data_pin = 19;  // GPIO19 - P31/P19
const int ttom_led_data_pin = 22;   // GPIO22 - P36/P22
const int kick_led_data_pin = 21;   // GPIO21 - P33/P21
const int ftom_led_data_pin = 23;   // GPIO23 - P37/P23

// Clock Pins
const int snare_led_clock_pin = 25; // GPIO25 - P9/P25
const int ttom_led_clock_pin = 27;  // GPIO27 - P11/P27
const int kick_led_clock_pin = 26;  // GPIO26 - P10/P26
const int ftom_led_clock_pin = 32;  // GPIO32 - P7/P32
#pragma endregion

// LED Count
#pragma region LED Count
const int snare_led_count = 57;
const int kick_led_count = 92;
const int ttom_led_count = 52;
const int ftom_led_count = 71;
#pragma endregion

// LED Container
#pragma region LED Container
CRGB snare_leds[snare_led_count];
CRGB kick_leds[kick_led_count];
CRGB ttom_leds[ttom_led_count];
CRGB ftom_leds[ftom_led_count];

CLEDController *controllers[NUM_DRUMS];
#pragma endregion

// Application state values
#pragma region Applicaion State Values
bool initApp;
bool isMainPage;
uint8_t tempSettingsValue;

// drum settings:
// 1. trigger sens
// 2. trigger sens factor
// 3. trigger time thresh
// 4. trigger duration 
// 5. num of leds (CURRENTLY DELETED)
const uint8_t drumSettingCount = 4;
uint8_t currDrumSettingIndex = 0;

// global settings:
// 1. blink length val
// 2. fade delay val
// 3. fade step val
// 4. fade interval val
// 5. circle time val
const uint8_t globalSettingCount = 5;
uint8_t currGlobalSettingIndex = 0;
#pragma endregion

// Nextion 
#pragma region Nextion

#pragma region Nextion Controls

// Loading Page (0)
NexText loadingLabel = NexText(0, 2, "loadingLabel");
NexProgressBar progressBar = NexProgressBar(0, 3, "progressBar");

// Main Page (1)
NexButton globalBtn = NexButton(1, 2, "globalBtn");
NexButton settingsBtn = NexButton(1, 36, "settingsBtn");

NexButton d0ColorSelectBtn = NexButton(1, 3, "d0_colorHit");
NexButton d1ColorSelectBtn = NexButton(1, 5, "d1_colorHit");
NexButton d2ColorSelectBtn = NexButton(1, 4, "d2_colorHit");
NexButton d3ColorSelectBtn = NexButton(1, 6, "d3_colorHit");

NexButton modeUpBtn = NexButton(1, 19, "modeUpBtn");
NexButton modeDownBtn = NexButton(1, 22, "modeDownBtn");
NexButton colorUpBtn = NexButton(1, 20, "colorUpBtn");
NexButton colorDownBtn = NexButton(1, 23, "colorDownBtn");
NexButton syncBtn = NexButton(1, 21, "syncBtn");

NexButton presetLeftBtn = NexButton(1, 38, "presetLeftBtn");
NexButton presetSelectBtn = NexButton(1, 39, "presetSelectBtn");
NexButton presetRightBtn = NexButton(1, 40, "presetRightBtn");

NexText d0_label = NexText(1, 10, "d0_label");
NexText d0_hit = NexText(1, 11, "d0_hit");
NexText d0_details = NexText(1, 12, "d0_details");
NexNumber d0_sensVal = NexNumber(1, 37, "d0_sensVal");
NexNumber d0_reading = NexNumber(1, 38, "d0_read");

NexText d1_label = NexText(1, 13, "d1_label");
NexText d1_hit = NexText(1, 14, "d1_hit");
NexText d1_details = NexText(1, 17, "d1_details");
NexNumber d1_sensVal = NexNumber(1, 40, "d1_sensVal");
NexNumber d1_reading = NexNumber(1, 41, "d1_read");

NexText d2_label = NexText(1, 7, "d2_label");
NexText d2_hit = NexText(1, 8, "d2_hit");
NexText d2_details = NexText(1, 9, "d2_details");
NexNumber d2_sensVal = NexNumber(1, 43, "d2_sensVal");
NexNumber d2_reading = NexNumber(1, 44, "d2_read");

NexText d3_label = NexText(1, 15, "d3_label");
NexText d3_hit = NexText(1, 16, "d3_hit");
NexText d3_details = NexText(1, 18, "d3_details");
NexNumber d3_sensVal = NexNumber(1, 46, "d3_sensVal");
NexNumber d3_reading = NexNumber(1, 47, "d3_read");

// Settings Page (2)
NexButton backBtn_drum = NexButton(2, 2, "backBtn_drum");

NexButton upBtn = NexButton(2, 3, "upBtn");
NexButton downBtn = NexButton(2, 7, "downBtn");
NexButton leftBtn = NexButton(2, 4, "leftBtn");
NexButton rightBtn = NexButton(2, 6, "rightBtn");
NexButton saveBtn = NexButton(2, 5, "saveBtn");

NexNumber trigSensVal = NexNumber(2, 12, "trigSensVal");
NexNumber trigFactorVal = NexNumber(2, 13, "trigFactorVal");
NexNumber trigTimeVal = NexNumber(2, 9, "trigTimeVal");
NexNumber trigDurVal = NexNumber(2, 16, "trigDurVal");

// Global Settings Page (3)
NexButton backBtn_global = NexButton(3, 2, "backBtn_global");

NexButton upBtn_global = NexButton(3, 3, "upBtn");
NexButton downBtn_global = NexButton(3, 7, "downBtn");
NexButton leftBtn_global = NexButton(3, 4, "leftBtn");
NexButton rightBtn_global = NexButton(3, 6, "rightBtn");
NexButton saveBtn_global = NexButton(3, 5, "saveBtn");

NexNumber blinkLengthVal = NexNumber(3, 14, "blinkLengthVal");
NexNumber fadeDelayVal = NexNumber(3, 10, "fadeDelayVal");
NexNumber fadeStepVal = NexNumber(3, 11, "fadeStepVal");
NexNumber fadeInterVal = NexNumber(3, 18, "fadeInterVal");
NexNumber circleTimeVal = NexNumber(3, 12, "circleTimeVal");

// List of Nextion Objects to enable event listening
NexTouch *nex_listen_list[] = {
  &globalBtn,
  &settingsBtn,

  &d0ColorSelectBtn,
  &d1ColorSelectBtn,
  &d2ColorSelectBtn,
  &d3ColorSelectBtn,

  &modeUpBtn,
  &modeDownBtn,
  &colorUpBtn,
  &colorDownBtn,

  // &syncBtn,
  // &presetLeftBtn,
  // &presetSelectBtn,
  // &presetRightBtn,

  &backBtn_drum,
  &upBtn,
  &downBtn,
  &leftBtn,
  &rightBtn,
  &saveBtn,
  &trigSensVal,
  &trigFactorVal,
  &trigTimeVal,
  &trigDurVal,

  &backBtn_global,
  &upBtn_global,
  &downBtn_global,
  &leftBtn_global,
  &rightBtn_global,
  &saveBtn_global,
  &blinkLengthVal,
  &fadeDelayVal,
  &fadeStepVal,
  &fadeInterVal,
  &circleTimeVal,
  NULL
};

#pragma endregion

#pragma region Nextion Event Handlers

// Set d0-d3 as the current drum
#pragma region Set Current Drum
void d0ColorSelectBtnPushCallback(void *ptr) {
  setCurrentDrum(0);
  delay(100);
}
void d1ColorSelectBtnPushCallback(void *ptr) {
  setCurrentDrum(1);
  delay(100);
}
void d2ColorSelectBtnPushCallback(void *ptr) {
  setCurrentDrum(2);
  delay(100);
}
void d3ColorSelectBtnPushCallback(void *ptr) {
  setCurrentDrum(3);
  delay(100);
}
#pragma endregion

#pragma region Cycle Color Mode
// cycle color mode
void modeUpBtnPushCallback(void *ptr) {
  getCurrentDrum() -> CycleColorMode(true);
  getCurrentDrum() -> SetDrumInfo();
  delay(100);
}
void modeDownBtnPushCallback(void *ptr) {
  getCurrentDrum() -> CycleColorMode(false);
  getCurrentDrum() -> SetDrumInfo();
  delay(100);
}
#pragma endregion

#pragma region Cycle Colors
// cycle color btns
void colorUpBtnPushCallback(void *ptr) {
  // Set next color in sequence depending on mode for current drum
  getCurrentDrum() -> color_.SetNextColor();
  getCurrentDrum() -> SetDrumInfo();
  delay(100);
}
void colorDownBtnPushCallback(void *ptr){
  // Set previous color in sequence depending on mode for current drum
  getCurrentDrum() -> color_.SetPrevColor();
  getCurrentDrum() -> SetDrumInfo();
  delay(100);
}
#pragma endregion

void syncBtnPushCallback(void *ptr) {

}

#pragma region (Song) Preset Event Callbacks
void presetLeftBtnPushCallback(void *ptr) {

}
void presetSelectBtnPushCallback(void *ptr) {

}
void presetRightBtnPushCallback(void *ptr) {

}
#pragma endregion

void globalBtnPushCallback(void *ptr) {
  showGlobalSettingsPage();
  delay(200);
}
void settingsBtnPushCallback(void *ptr) {
  showSettingsPage();
  delay(200);
}
void backBtnPushCallback(void *ptr) {
  showMainPage();
  delay(200);
}

#pragma region Drum Setting Event Callbacks
uint8_t GetDrumSettingsVal(int index) {
  switch (index) {
    case 0:
      return getCurrentDrum() -> ReadSensitivityValue();
    case 1: 
      return getCurrentDrum() -> ReadSensitivityFactor();
    case 2:
      return getCurrentDrum() -> ReadTriggerThreshValue();
    case 3:
      return getCurrentDrum() -> ReadTriggerDurationValue();
    // case 3:
    //   return getCurrentDrum() -> ReadNumOfLedsValue();
    default:
      return 0;
  }
}

void SetDrumSettingsText(int index) {
    switch (index) {
    case 0:
      Display::SetNumber(trigSensVal.getObjName(), tempSettingsValue);
      break;
    case 1: 
      Display::SetNumber(trigFactorVal.getObjName(), tempSettingsValue);
      break;
    case 2:
      Display::SetNumber(trigTimeVal.getObjName(), tempSettingsValue);
      break;
    case 3:
      Display::SetNumber(trigDurVal.getObjName(), tempSettingsValue);
      break;
    // case 3:
    //   Display::SetNumber(numLedsVal.getObjName(), tempSettingsValue);
    //   break;
    default:
      break;
  }
}

void upBtnPushCallback(void *ptr) {
  currDrumSettingIndex = (currDrumSettingIndex - 1) % drumSettingCount;
  Display::SetCurrDrumSetting(currDrumSettingIndex);
  tempSettingsValue = GetDrumSettingsVal(currDrumSettingIndex);
}
void downBtnPushCallback(void *ptr) {
  currDrumSettingIndex = (currDrumSettingIndex + 1) % drumSettingCount;
  Display::SetCurrDrumSetting(currDrumSettingIndex);
  tempSettingsValue = GetDrumSettingsVal(currDrumSettingIndex);
}
void leftBtnPushCallback(void *ptr) {
  tempSettingsValue = (tempSettingsValue - 1) % 256;
  SetDrumSettingsText(currDrumSettingIndex);
}
void rightBtnPushCallback(void *ptr) {
  tempSettingsValue = (tempSettingsValue + 1) % 256;
  SetDrumSettingsText(currDrumSettingIndex);
}
void saveBtnPushCallback(void *ptr) {
  switch(currDrumSettingIndex) {
    case 0:
      getCurrentDrum() -> UpdateSensitivityValue(tempSettingsValue);
      break;
    case 1:
      getCurrentDrum() -> UpdateSensitivityFactor(tempSettingsValue);
      break;
    case 2:
      getCurrentDrum() -> UpdateTriggerThreshValue(tempSettingsValue);
      break;
    case 3:
      getCurrentDrum() -> UpdateTriggerDurationValue(tempSettingsValue);
      break;
    // case 3:
    //   getCurrentDrum() -> UpdateNumOfLedsValue(tempSettingsValue);
    //   break;
    default:
      break;
  }
}

void trigSensValPushCallback(void *ptr) {
  Serial.println("trigSensValPushCallback hit!");

  currDrumSettingIndex = 0;
  Display::SetCurrDrumSetting(currDrumSettingIndex);
  tempSettingsValue = GetDrumSettingsVal(currDrumSettingIndex);
}
void trigFactorValPushCallback(void *ptr) {
  Serial.println("trigFactorValPushCallback hit!");

  currDrumSettingIndex = 1;
  Display::SetCurrDrumSetting(currDrumSettingIndex);
  tempSettingsValue = GetDrumSettingsVal(currDrumSettingIndex);
}
void trigTimeValPushCallback(void *ptr) {
  Serial.println("trigTimeValPushhCallback hhit!");

  currDrumSettingIndex = 2;
  Display::SetCurrDrumSetting(currDrumSettingIndex);
  tempSettingsValue = GetDrumSettingsVal(currDrumSettingIndex);
}
void trigDurValPushCallback(void *ptr) {
  currDrumSettingIndex = 3;
  Display::SetCurrDrumSetting(currDrumSettingIndex);
  tempSettingsValue = GetDrumSettingsVal(currDrumSettingIndex);
}
void numLedsValPushCallback(void *ptr) {
  Serial.println("numLedsValPushCallback hhit!");

  currDrumSettingIndex = 4;
  Display::SetCurrDrumSetting(currDrumSettingIndex);
  tempSettingsValue = GetDrumSettingsVal(currDrumSettingIndex);
}
#pragma endregion

#pragma region Global Setting Event Callbacks
uint8_t GetGlobalSettingsVal(int index) {
  switch (index) {
    case 0:
      return Drum::ReadBlinkLenValue();
    case 1: 
      return Drum::ReadFadeDelayValue();
    case 2:
      return Drum::ReadFadeStepsValue();
    case 3:
      return Drum::ReadFadeIntervalValue();
    case 4:
      return Drum::ReadCircleTimeValue();
    default:
      return 0;
  }
}

void SetGlobalSettingsText(int index) {
  switch (index) {
    case 0:
      Display::SetNumber(blinkLengthVal.getObjName(), tempSettingsValue);
      break;
    case 1: 
      Display::SetNumber(fadeDelayVal.getObjName(), tempSettingsValue);
      break;
    case 2:
      Display::SetNumber(fadeStepVal.getObjName(), tempSettingsValue);
      break;
    case 3:
      Display::SetNumber(fadeInterVal.getObjName(), tempSettingsValue);
      break;
    case 4:
      Display::SetNumber(circleTimeVal.getObjName(), tempSettingsValue);
      break;
    default:
      break;
  }
}

void upBtnGlobalPushCallback(void *ptr) {
  currGlobalSettingIndex = (currGlobalSettingIndex - 1) % globalSettingCount;
  Display::SetCurrGlobalSetting(currGlobalSettingIndex);
  tempSettingsValue = GetGlobalSettingsVal(currGlobalSettingIndex);
}
void downBtnGlobalPushCallback(void *ptr) {
  currGlobalSettingIndex = (currGlobalSettingIndex + 1) % globalSettingCount;
  Display::SetCurrGlobalSetting(currGlobalSettingIndex);
  tempSettingsValue = GetGlobalSettingsVal(currGlobalSettingIndex);
}
void leftBtnGlobalPushCallback(void *ptr) {
  tempSettingsValue = (tempSettingsValue - 1) % 256;
  SetGlobalSettingsText(currGlobalSettingIndex);
}
void rightBtnGlobalPushCallback(void *ptr) {
  tempSettingsValue = (tempSettingsValue + 1) % 256;
  SetGlobalSettingsText(currGlobalSettingIndex);
}
void saveBtnGlobalPushCallback(void *ptr) {
  switch (currGlobalSettingIndex) {
    case 0:
      Drum::UpdateBlinkLenValue(tempSettingsValue);
      break;
    case 1: 
      Drum::UpdateFadeDelayValue(tempSettingsValue);
      break;
    case 2:
      Drum::UpdateFadeStepsValue(tempSettingsValue);
      break;
    case 3:
      Drum::UpdateFadeIntervalValue(tempSettingsValue);
      break;
    case 4:
      Drum::UpdateCircleTimeValue(tempSettingsValue);
      break;
    default:
      break;
  }
}

void blinkLenValPushCallback(void *ptr) {
  currGlobalSettingIndex = 0;
  Display::SetCurrGlobalSetting(currGlobalSettingIndex);
  tempSettingsValue = GetGlobalSettingsVal(currGlobalSettingIndex);
}
void fadeDecayValPushCallback(void *ptr) {
  currGlobalSettingIndex = 1;
  Display::SetCurrGlobalSetting(currGlobalSettingIndex);
  tempSettingsValue = GetGlobalSettingsVal(currGlobalSettingIndex);
}
void fadeStepValPushCallback(void *ptr) {
  currGlobalSettingIndex = 2;
  Display::SetCurrGlobalSetting(currGlobalSettingIndex);
  tempSettingsValue = GetGlobalSettingsVal(currGlobalSettingIndex);
}
void fadeInterValPushCallback(void *ptr) {
  currGlobalSettingIndex = 3;
  Display::SetCurrGlobalSetting(currGlobalSettingIndex);
  tempSettingsValue = GetGlobalSettingsVal(currGlobalSettingIndex);
}
void circleTimeValPushCallback(void *ptr) {
  currGlobalSettingIndex = 4;
  Display::SetCurrGlobalSetting(currGlobalSettingIndex);
  tempSettingsValue = GetGlobalSettingsVal(currGlobalSettingIndex);
}
#pragma endregion

#pragma endregion

#pragma endregion

// Default drum instances
#pragma region Default Drum Instances
int dIndex;
// std::vector<Drum> drums;
Drum drums[NUM_DRUMS];
#pragma endregion

#pragma region Drum State Management
// Return pointer to the currently selected drum instance
Drum * getCurrentDrum() {
  return &drums[dIndex];
}

// Set the current drum based on the index of the default drum instances
void setCurrentDrum(int index) {
  if (index < NUM_DRUMS) {
    dIndex = index;
  } else {
    dIndex = 0;
  }

  drums[dIndex].SetCurrentDrum(true);
  drums[dIndex].TriggerHit();
  for (int i = 0; i < NUM_DRUMS; ++i) {
    if(i != dIndex) {
      drums[i].SetCurrentDrum(false);
    }
  }
}

// Cycle to the next drum based on the default drum instances
void setNextDrumAsCurrent() {
  setCurrentDrum((dIndex + 1) % NUM_DRUMS);
}

// Initialize the display info for all default drums
void initDisplayDrumValues() {
  // For each drum, update the display to show the drum details 
  for (int i = 0; i < NUM_DRUMS; ++i) {
    drums[i].SetDrumInfo();
  }
}
#pragma endregion

#pragma region Nextion Page Management
// Prep and show the main/home page on the display
void initShowMainPage() {
  if (!initApp) {
    for (int i = 0; i < 1; ++i) {
      // Show main page
      Display::ShowMainPage();  
      initDisplayDrumValues();    
      setCurrentDrum(0);        
      isMainPage = true;
      initApp = true;
    }
  }
}

// Show the main/home page on the display
void showMainPage() {
  // Display::ShowMainPage();
  Display::ShowMainPage();

  initDisplayDrumValues();
  setCurrentDrum(dIndex);
  isMainPage = true;
}

// Show the drum settings page on the display
void showSettingsPage() {
  Drum *currDrum = getCurrentDrum();

  // Show drum settings page
  Display::ShowSettingsPage();
  delay(200);

  // Init settings state
  currDrumSettingIndex = 0;
  Display::SetCurrDrumSetting(0);
  tempSettingsValue = currDrum -> Drum::ReadSensitivityValue();
  Display::ShowDrumSavedIndicator(false);

  // Set the current drum setting values
  Display::SetNumber(trigSensVal.getObjName(), tempSettingsValue);
  Display::SetNumber(trigFactorVal.getObjName(), currDrum -> ReadSensitivityFactor());
  Display::SetNumber(trigTimeVal.getObjName(), currDrum -> ReadTriggerThreshValue());
  Display::SetNumber(trigDurVal.getObjName(), currDrum -> ReadTriggerDurationValue());
  // Display::SetNumber(numLedsVal.getObjName(), currDrum -> ReadNumOfLedsValue());

  Display::SetVisibility(trigSensVal.getObjName(), true);
  Display::SetVisibility(trigFactorVal.getObjName(), true);
  Display::SetVisibility(trigTimeVal.getObjName(), true);
  Display::SetVisibility(trigDurVal.getObjName(), true);
  // Display::SetVisibility(numLedsVal.getObjName(), true);

  isMainPage = false;
}

// Show the global settings page on the display
void showGlobalSettingsPage() {
  // Show global settings page
  Display::ShowGlobalSettingsPage();
  delay(200);

  // Init settings state
  currGlobalSettingIndex = 0;
  Display::SetCurrGlobalSetting(0);
  tempSettingsValue = Drum::ReadBlinkLenValue();
  Display::ShowGlobalSavedIndicator(false);

  // TODO: Store these values instead of always reading them?
  // Set the global setting values
  Display::SetNumber(blinkLengthVal.getObjName(), Drum::ReadBlinkLenValue());
  Display::SetNumber(fadeDelayVal.getObjName(), Drum::ReadFadeDelayValue());
  Display::SetNumber(fadeStepVal.getObjName(), Drum::ReadFadeStepsValue());
  Display::SetNumber(fadeInterVal.getObjName(), Drum::ReadFadeIntervalValue());
  Display::SetNumber(circleTimeVal.getObjName(), Drum::ReadCircleTimeValue());

  Display::SetVisibility(blinkLengthVal.getObjName(), true);
  Display::SetVisibility(fadeDelayVal.getObjName(), true);
  Display::SetVisibility(fadeStepVal.getObjName(), true);
  Display::SetVisibility(fadeInterVal.getObjName(), true);
  Display::SetVisibility(circleTimeVal.getObjName(), true);

  isMainPage = false;
}
#pragma endregion

// Analog trigger listener
void triggerListener() {

  // Serial.println();
  // Serial.print("reading: ");
  // Serial.println(drum0Val, DEC);
  // float voltage = drum0Val * (5.0 / 1023.0);
  // Serial.print("voltage: ");
  // Serial.println(voltage);
  //   Serial.print("reading: ");
  // Serial.println(drum1Val, DEC);
  // voltage = drum1Val * (5.0 / 1023.0);
  // Serial.print("voltage: ");
  // Serial.println(voltage);
  //   Serial.print("reading: ");
  // Serial.println(drum2Val, DEC);
  // voltage = drum2Val * (5.0 / 1023.0);
  // Serial.print("voltage: ");
  // Serial.println(voltage);
  //   Serial.print("reading: ");
  // Serial.println(drum3Val, DEC);
  // voltage = drum3Val * (5.0 / 1023.0);
  // Serial.print("voltage: ");
  // Serial.println(voltage);
  // Serial.print("drum0: ");
  // Serial.println(drums[0].hitIndicator_ -> getObjName());
  // Serial.println(drums[0].hitIndicator_ -> getObjCid());

  int drum0Val = analogRead(drums[0].trigger_pin_);
  // drums[0].SetDrumTriggerReading(drum0Val);
  if (drums[0].IsTriggerValid(drum0Val)) {
    drums[0].TriggerHit();
  }

  int drum1Val = analogRead(drums[1].trigger_pin_);
  // drums[1].SetDrumTriggerReading(drum1Val);
  if (drums[1].IsTriggerValid(drum1Val)) {
    drums[1].TriggerHit();
  }

  int drum2Val = analogRead(drums[2].trigger_pin_);
  // drums[2].SetDrumTriggerReading(drum2Val);
  if (drums[2].IsTriggerValid(drum2Val)) {
    drums[2].TriggerHit();
  }

  int drum3Val = analogRead(drums[3].trigger_pin_);
  // drums[3].SetDrumTriggerReading(drum3Val);
  if (drums[3].IsTriggerValid(drum3Val)) {
    drums[3].TriggerHit();
  }
}

// Deactivate trigger listener
void untriggerListener() {
  if (drums[0].IsTriggered()) {
    drums[0].TriggerHitEnd();
  }

  if (drums[1].IsTriggered()) {
    drums[1].TriggerHitEnd();
  }

  if (drums[2].IsTriggered()) {
    drums[2].TriggerHitEnd();
  }

  if (drums[3].IsTriggered()) {
    drums[3].TriggerHitEnd();
  }
}


// Arduino setup
void setup() {
  // Start serial data transmission @ 9600 bits/s and upgrade speed
  Serial.begin(115200);
  // Serial.begin(921600);
  delay(200);

  // Initialize Nextion 
  nexInit();
  delay(200);

  // Set progress @0%
  Display::SetNumber(progressBar.getObjName(), 0);
  delay(200);

  // Initalize eeprom usage
  EEPROM.begin(512);

  // Initialize drums
  // drums.push_back(Drum("Snare", &d0_hit, &d0ColorSelectBtn, &d0_label, &d0_details, &d0_sensVal, &d0_reading, trigger_0, snare_leds, snare_led_count, controllers[0]));
  // drums.push_back(Drum("Bass", &d1_hit, &d1ColorSelectBtn, &d1_label, &d1_details, &d1_sensVal, &d1_reading, trigger_1, kick_leds, kick_led_count, controllers[1]));
  // drums.push_back(Drum("T.Tom", &d2_hit, &d2ColorSelectBtn, &d2_label, &d2_details, &d2_sensVal, &d2_reading, trigger_2, ttom_leds, ttom_led_count, controllers[2]));
  // drums.push_back(Drum("F.Tom", &d3_hit, &d3ColorSelectBtn, &d3_label, &d3_details, &d3_sensVal, &d3_reading, trigger_3, ftom_leds, ftom_led_count, controllers[3]));
  // drums[0] = &Drum("Snare", &d0_hit, &d0ColorSelectBtn, &d0_label, &d0_details, &d0_sensVal, &d0_reading, trigger_0, snare_leds, snare_led_count);
  // drums[1] = &Drum("Bass", &d1_hit, &d1ColorSelectBtn, &d1_label, &d1_details, &d1_sensVal, &d1_reading, trigger_1, kick_leds, kick_led_count);
  // drums[2] = &Drum("T.Tom", &d2_hit, &d2ColorSelectBtn, &d2_label, &d2_details, &d2_sensVal, &d2_reading, trigger_2, ttom_leds, ttom_led_count);
  // drums[3] = &Drum("F.Tom", &d3_hit, &d3ColorSelectBtn, &d3_label, &d3_details, &d3_sensVal, &d3_reading, trigger_3, ftom_leds, ftom_led_count);

  // Set progress @10%
  Display::SetNumber(progressBar.getObjName(), 10);
  delay(200);

  d0ColorSelectBtn.attachPush(d0ColorSelectBtnPushCallback, &d0ColorSelectBtn);
  d1ColorSelectBtn.attachPush(d1ColorSelectBtnPushCallback, &d1ColorSelectBtn);
  d2ColorSelectBtn.attachPush(d2ColorSelectBtnPushCallback, &d2ColorSelectBtn);
  d3ColorSelectBtn.attachPush(d3ColorSelectBtnPushCallback, &d3ColorSelectBtn);

  modeUpBtn.attachPush(modeUpBtnPushCallback, &modeUpBtn);
  modeDownBtn.attachPush(modeDownBtnPushCallback, &modeDownBtn);
  colorUpBtn.attachPush(colorUpBtnPushCallback, &colorUpBtn);
  colorDownBtn.attachPush(colorDownBtnPushCallback, &colorDownBtn);

  // syncBtn.attachPush(syncBtnPushCallback, &syncBtn);

  // Set progress @20%
  Display::SetNumber(progressBar.getObjName(), 20);
  delay(200);

  // TODO: Preset drum buttons
  presetLeftBtn.attachPush(presetLeftBtnPushCallback, &presetLeftBtn);
  presetSelectBtn.attachPush(presetSelectBtnPushCallback, &presetSelectBtn);
  presetRightBtn.attachPush(presetRightBtnPushCallback, &presetRightBtn);

  // Set progress @30%
  Display::SetNumber(progressBar.getObjName(), 30);
  delay(200);

  settingsBtn.attachPush(settingsBtnPushCallback, &settingsBtn);
  globalBtn.attachPush(globalBtnPushCallback, &globalBtn);
  backBtn_drum.attachPush(backBtnPushCallback, &backBtn_drum);
  backBtn_global.attachPush(backBtnPushCallback, &backBtn_global);

  // Set progress @40%
  Display::SetNumber(progressBar.getObjName(), 40);
  delay(200);

  upBtn.attachPush(upBtnPushCallback, &upBtn);
  downBtn.attachPush(downBtnPushCallback, &downBtn);
  leftBtn.attachPush(leftBtnPushCallback, &leftBtn);
  rightBtn.attachPush(rightBtnPushCallback, &rightBtn);
  saveBtn.attachPush(saveBtnPushCallback, &saveBtn);

  trigSensVal.attachPush(trigSensValPushCallback, &trigSensVal);
  trigFactorVal.attachPush(trigFactorValPushCallback, &trigFactorVal);
  trigTimeVal.attachPush(trigTimeValPushCallback, &trigTimeVal);
  trigDurVal.attachPush(trigDurValPushCallback, &trigDurVal);
  // numLedsVal.attachPush(numLedsValPushCallback, &numLedsVal);

  // Set progress @50%
  Display::SetNumber(progressBar.getObjName(), 50);
  delay(200);

  upBtn_global.attachPush(upBtnGlobalPushCallback, &upBtn_global);
  downBtn_global.attachPush(downBtnGlobalPushCallback, &downBtn_global);
  leftBtn_global.attachPush(leftBtnGlobalPushCallback, &leftBtn_global);
  rightBtn_global.attachPush(rightBtnGlobalPushCallback, &rightBtn_global);
  saveBtn_global.attachPush(saveBtnGlobalPushCallback, &saveBtn_global);

  blinkLengthVal.attachPush(blinkLenValPushCallback, &blinkLengthVal);
  fadeDelayVal.attachPush(fadeDecayValPushCallback, &fadeDelayVal);
  fadeStepVal.attachPush(fadeStepValPushCallback, &fadeStepVal);
  fadeInterVal.attachPush(fadeInterValPushCallback, &fadeInterVal);
  circleTimeVal.attachPush(circleTimeValPushCallback, &circleTimeVal);

  // Set progress @60%
  Display::SetNumber(progressBar.getObjName(), 60);
  delay(200);

  // Init LEDs
  // FastLED.addLeds<APA102HD, snare_led_data_pin, snare_led_clock_pin, BGR>(snare_leds, snare_led_count);
  // FastLED.addLeds<APA102HD, kick_led_data_pin, kick_led_clock_pin, BGR>(kick_leds, kick_led_count);
  // FastLED.addLeds<APA102HD, ttom_led_data_pin, ttom_led_clock_pin, BGR>(ttom_leds, ttom_led_count);
  // FastLED.addLeds<APA102HD, ftom_led_data_pin, ftom_led_clock_pin, BGR>(ftom_leds, ftom_led_count);

  controllers[0] = &FastLED.addLeds<APA102HD, snare_led_data_pin, snare_led_clock_pin, BGR>(snare_leds, snare_led_count);
  controllers[1] = &FastLED.addLeds<APA102HD, kick_led_data_pin, kick_led_clock_pin, BGR>(kick_leds, kick_led_count);
  controllers[2] = &FastLED.addLeds<APA102HD, ttom_led_data_pin, ttom_led_clock_pin, BGR>(ttom_leds, ttom_led_count);
  controllers[3] = &FastLED.addLeds<APA102HD, ftom_led_data_pin, ftom_led_clock_pin, BGR>(ftom_leds, ftom_led_count);

  // drums.push_back(Drum("Snare", &d0_hit, &d0ColorSelectBtn, &d0_label, &d0_details, &d0_sensVal, &d0_reading, trigger_0, snare_leds, snare_led_count, controllers[0]));
  // drums.push_back(Drum("Bass", &d1_hit, &d1ColorSelectBtn, &d1_label, &d1_details, &d1_sensVal, &d1_reading, trigger_1, kick_leds, kick_led_count, controllers[1]));
  // drums.push_back(Drum("T.Tom", &d2_hit, &d2ColorSelectBtn, &d2_label, &d2_details, &d2_sensVal, &d2_reading, trigger_2, ttom_leds, ttom_led_count, controllers[2]));
  // drums.push_back(Drum("F.Tom", &d3_hit, &d3ColorSelectBtn, &d3_label, &d3_details, &d3_sensVal, &d3_reading, trigger_3, ftom_leds, ftom_led_count, controllers[3]));

  drums[0] = Drum("Snare", &d0_hit, &d0ColorSelectBtn, &d0_label, &d0_details, &d0_sensVal, &d0_reading, trigger_0, snare_leds, snare_led_count, controllers[0]);
  drums[1] = Drum("Bass", &d1_hit, &d1ColorSelectBtn, &d1_label, &d1_details, &d1_sensVal, &d1_reading, trigger_1, kick_leds, kick_led_count, controllers[1]);
  drums[2] = Drum("T.Tom", &d2_hit, &d2ColorSelectBtn, &d2_label, &d2_details, &d2_sensVal, &d2_reading, trigger_2, ttom_leds, ttom_led_count, controllers[2]);
  drums[3] = Drum("F.Tom", &d3_hit, &d3ColorSelectBtn, &d3_label, &d3_details, &d3_sensVal, &d3_reading, trigger_3, ftom_leds, ftom_led_count, controllers[3]);

  // Set led strip temperature
  FastLED.setTemperature(ColorTemperature::ClearBlueSky);

  // Set progress @100%
  Display::SetNumber(progressBar.getObjName(), 100);
  delay(200);

  // Show main page
  initApp = false;

  // Don't know why but this has to be called in the main loop a couple of times for the initial cmds to pass. Timing issue?
  initShowMainPage();
}

// Arduino loop
void loop() {
  // Nextion event handler loop
  nexLoop(nex_listen_list);
  
  // Trigger Listener
  if (isMainPage) {
    triggerListener();
    untriggerListener();
  }
}
