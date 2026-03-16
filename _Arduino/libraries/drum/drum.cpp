/**
 * ddrum.cpp (8/7/2024)
 * 
 * Main class that handles drum functionality. (drum info, color, sens, triggering, led)
 * Note: Nextion.h was modified to expose protected methods (NexObject.h)
 **/
#include <drum.h>
#include <FastLED.h>

Drum::Drum() {}

Drum::Drum(String name, NexText *hitIndicator, NexButton *selectColorPreviewBtn, NexText *drumNameLabel, 
    NexText *drumInfoLabel, NexNumber *drumSensLabel, NexNumber *drumReading, int triggerPin, 
    CRGB *ledStrip, uint8_t numOfLeds, CLEDController *ledController) 
{
    name_ = name;
    color_ = LedColor();

    trigger_pin_ = triggerPin;
    // data_pin_ = dataPin;
    // clock_pin_ = clockPin;

    is_current_drum_ = false;
    is_triggered_ = false;

    hitIndicator_ = hitIndicator;
    selectColorPreviewBtn_ = selectColorPreviewBtn;
    drumNameLabel_ = drumNameLabel;
    drumInfoLabel_ = drumInfoLabel;
    drumSensLabel_ = drumSensLabel;
    drumReading_ = drumReading;

    // Load EEPROM values
    sensVal_ = ReadSensitivityValue();
    sensFactorVal_ = ReadSensitivityFactor();
    triggerTimeVal_ = ReadTriggerThreshValue();
    triggerDurationVal_ = ReadTriggerDurationValue();
    // numOfLeds_ = ReadNumOfLedsValue();
    numOfLeds_ = numOfLeds;

    // Serial.print("CONSTRUCTING drum, reading sens val: ");
    // Serial.println(name_);
    // Serial.println(ReadSensitivityValue(), DEC);
    // Serial.println(sensVal_, DEC);

    blinkLenVal_ = ReadBlinkLenValue();
    fadeDelayVal_ = ReadFadeDelayValue();
    fadeStepVal_ = ReadFadeStepsValue();
    fadeIntervalVal_ = ReadFadeIntervalValue();
    circleTimeVal_ = ReadCircleTimeValue();

    // Copy led strip ref
    ledStrip_ = ledStrip;
    ledController_ = ledController;
}

/*!
 * Indicate that this particular drum is currently selected and update the display
 **/
void Drum::SetCurrentDrum(bool isCurrent) {
    if (isCurrent) {
        is_current_drum_ = true;
        Display::SetBorderThickness(String(selectColorPreviewBtn_ -> getObjName()), 20);
        Display::SetBorderColor(String(selectColorPreviewBtn_ -> getObjName()), 23532);
    } else {
        is_current_drum_ = false;
        Display::SetBorderThickness(String(selectColorPreviewBtn_ -> getObjName()), 2);
        Display::SetBorderColor(String(selectColorPreviewBtn_ -> getObjName()), 163);
    }
}

/*!
 * TODO: Speed up and optmize!!!
 * Update the drum info on the display -> for the settings page
 **/
void Drum::SetDrumInfo() {
    // SetColor(color_.currColor_);
    
    drumSensLabel_ -> setValue(sensVal_ * sensFactorVal_);
    selectColorPreviewBtn_ -> Set_background_color_bco(color_.ToDisplayColorValue());
    drumInfoLabel_ -> setText(color_.ToColorName().c_str());
    drumInfoLabel_ -> Set_background_color_bco(color_.ToDisplayColorValue());

    ShowHitIndicator(false);
}

/*!
 * Update the drum trigger reading on the display
 **/
void Drum::SetDrumTriggerReading(int value) {
    drumReading_ -> setValue(value);
}

/*!
 * Show the hit indicator on the display for the given drum
 **/
void Drum::ShowHitIndicator(bool isShow) {
    Display::SetVisibility(String(hitIndicator_ -> getObjName()), isShow);
}

/*!
 * Cycle the color mode for the given drum
 **/
void Drum::CycleColorMode(bool next) {
    if (next) color_.SetNextColorMode();
    else color_.SetPrevColorMode();

    if (color_.IsStaticWhiteColorMode()) {
        // Immediately show white lite
        TurnOnWhite();
    } else if (color_.mode_ == ColorMode::AlwaysOnBlinkWhite) {
        // Set color to the currently color
        TurnOn();
    } else {
        // Turn off lights
        TurnOff();
    }
}

/*!
 * TODO: Finish implementing
 * Indicate that the trigger was hit for the given drum
 * 
 * Blink: 1. Show
 * Fade: 1. Show, 2. Set brightness at 100%
 * Chase: 1. set base color, 2. show 
 * FadeRainbow:
 **/
void Drum::TriggerHit() {
    //
    ShowHitIndicator(true);

    // This call sets the is_triggered_ flag
    TriggerLights();

    if (is_triggered_ && color_.IsRainbowColorMode()) {
        color_.NextRainbowColor();
        SetDrumInfo();
        // delay(blinkLenVal_);
    }
}

/*!
 * TODO: Finish implementing
 * Indicate that the trigger hit is now done for the given drum
 **/
void Drum::TriggerHitEnd() {
    TriggerLightsEnd();

    //
    ShowHitIndicator(false);
}

/*!
 * TODO: Finish implementing
 * Turn on the led for the given drum and start the timer
 **/
void Drum::TriggerLights() {
    if (color_.IsFadeColorMode()) {
        StartFade();
        // EndFade();
    } 

    SetColor();
    TurnOn();

    this -> trigger_start_time_ = millis();
    this -> is_triggered_ = true;
}

/*!
 * TODO: Finish implementing
 * Turn off the led for the given drum once the timer is met
 **/
void Drum::TriggerLightsEnd() {
    if (color_.IsFadeColorMode()) {
        EndFade();
    } else {
        if ((millis() - trigger_start_time_) > triggerDurationVal_) {
            this -> TurnOff();
            this -> is_triggered_ = false;

            if (color_.IsStaticWhiteColorMode()) {
                TurnOnWhite();
            }
        }
    }
}

/*!
 * Returns whether or not the given drum is in a triggered state
 **/
bool Drum::IsTriggered() {
    return this -> is_triggered_;
}

bool Drum::IsTriggerValid(int rVal) {
    int scaledSensVal = sensVal_ * sensFactorVal_;

    return rVal >= scaledSensVal || rVal <= -scaledSensVal;
}

#pragma region Eeprom Reads
/*!
 * Read from the EEPROM the sensitivity for the given drum
 **/
uint8_t Drum::ReadSensitivityValue() {
    uint8_t data = 0;
    EEPROM.get(GetEepromSensAddr(), data);

    Serial.println(GetEepromSensAddr());
    Serial.println(data);

    return data;
}

/*!
 * Read from the EEPROM the sensitivity for the given drum
 **/
uint8_t Drum::ReadSensitivityFactor() {
    uint8_t data = 0;
    EEPROM.get(GetEepromSensFactorAddr(), data);

    return data;
}

/*!
 * Read from the EEPROM the sensitivity for the given drum
 **/
uint8_t Drum::ReadTriggerThreshValue() {
    uint8_t data = 0;
    EEPROM.get(GetEepromTriggerThreshAddr(), data);

    return data;
}

/*!
 * Read from the EEPROM the sensitivity for the given drum
 **/
uint8_t Drum::ReadTriggerDurationValue() {
    uint8_t data = 0;
    EEPROM.get(GetEepromTriggerDurationAddr(), data);

    return data;
}

/*!
 * Read from the EEPROM the sensitivity for the given drum
 **/
// uint8_t Drum::ReadNumOfLedsValue() {
//     uint8_t data = 0;
//     EEPROM.get(GetEepromNumOfLedsAddr(), data);

//     return data;
// }

/*!
 * Read from the EEPROM the sensitivity for the given drum
 **/
uint8_t Drum::ReadFadeDelayValue() {
    uint8_t data = 0;
    EEPROM.get(GetEepromFadeDelayAddr(), data);

    return data;
}

/*!
 * Read from the EEPROM the sensitivity for the given drum
 **/
uint8_t Drum::ReadFadeStepsValue() {
    uint8_t data = 0;
    EEPROM.get(GetEepromFadeStepsAddr(), data);

    return data;
}

/*!
 * Read from the EEPROM the number of intervals for fading
 **/
uint8_t Drum::ReadFadeIntervalValue() {
    uint8_t data = 0;
    EEPROM.get(GetEepromFadeIntervalAddr(), data);

    return data;
}

/*!
 * Read from the EEPROM the sensitivity for the given drum
 **/
uint8_t Drum::ReadBlinkLenValue() {
    uint8_t data = 0;
    EEPROM.get(GetEepromBlinkLenAddr(), data);

    return data;
}

/*!
 * Read from the EEPROM the sensitivity for the given drum
 **/
uint8_t Drum::ReadCircleTimeValue() {
    uint8_t data = 0;
    EEPROM.get(GetEepromCircleTimeAddr(), data);

    return data;
}
#pragma endregion

#pragma region Eeprom Writes
/*!
 * Save the new sensitivity value for the given drum
 **/
void Drum::UpdateSensitivityValue(uint8_t newVal) {
    EEPROM.write(GetEepromSensAddr(), newVal);
    EEPROM.commit();
    Display::ShowDrumSavedIndicator(false);
    sensVal_ = newVal;
}

/*!
 * Save the new sensitivity value for the given drum
 **/
void Drum::UpdateSensitivityFactor(uint8_t newVal) {
    EEPROM.write(GetEepromSensFactorAddr(), newVal);
    EEPROM.commit();
    Display::ShowDrumSavedIndicator(false);
    sensFactorVal_ = newVal;
}

/*!
 * Save the new sensitivity value for the given drum
 **/
void Drum::UpdateTriggerThreshValue(uint8_t newVal) {
    EEPROM.write(GetEepromTriggerThreshAddr(), newVal);
    EEPROM.commit();
    Display::ShowDrumSavedIndicator(false);
    triggerTimeVal_ = newVal;
}

/*!
 * Save the new sensitivity value for the given drum
 **/
void Drum::UpdateTriggerDurationValue(uint8_t newVal) {
    EEPROM.write(GetEepromTriggerDurationAddr(), newVal);
    EEPROM.commit();
    Display::ShowDrumSavedIndicator(false);
    triggerDurationVal_ = newVal;
}

/*!
 * Save the new sensitivity value for the given drum
 **/
// void Drum::UpdateNumOfLedsValue(uint8_t newVal) {
//     EEPROM.write(GetEepromNumOfLedsAddr(), newVal);
//     EEPROM.commit();
//     Display::ShowDrumSavedIndicator(false);
//     numOfLeds_ = newVal;

//     // TODO: Rebuild led strip -> Might need to restart module? need to delete the old strip and add a new one
//     // CRGB ledStrip[numOfLeds_];
//     // ledStrip_ = &ledStrip;
//     // FastLED.clearData();
// }

/*!
 * Save the new sensitivity value for the given drum
 **/
void Drum::UpdateFadeDelayValue(uint8_t newVal) {
    EEPROM.write(GetEepromFadeDelayAddr(), newVal);
    EEPROM.commit();
    Display::ShowGlobalSavedIndicator(false);

}

/*!
 * Save the new sensitivity value for the given drum
 **/
void Drum::UpdateFadeStepsValue(uint8_t newVal) {
    EEPROM.write(GetEepromFadeStepsAddr(), newVal);
    EEPROM.commit();
    Display::ShowGlobalSavedIndicator(false);
}

/*!
 * Save the new sensitivity value for the given drum
 **/
void Drum::UpdateFadeIntervalValue(uint8_t newVal) {
    EEPROM.write(GetEepromFadeIntervalAddr(), newVal);
    EEPROM.commit();
    Display::ShowGlobalSavedIndicator(false);
}

/*!
 * Save the new sensitivity value for the given drum
 **/
void Drum::UpdateBlinkLenValue(uint8_t newVal) {
    EEPROM.write(GetEepromBlinkLenAddr(), newVal);
    EEPROM.commit();
    Display::ShowGlobalSavedIndicator(false);
}

/*!
 * Save the new sensitivity value for the given drum
 **/
void Drum::UpdateCircleTimeValue(uint8_t newVal) {
    EEPROM.write(GetEepromCircleTimeAddr(), newVal);
    EEPROM.commit();
    Display::ShowGlobalSavedIndicator(false);
}
#pragma endregion

#pragma region Eeprom Addrs
/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromSensFactorAddr() {
      if (this -> name_ == "Snare") {
        return 0;
    } else if (this -> name_ == "Bass") {
        return 1;
    } else if (this -> name_ == "T.Tom") {
        return 2;
    } else {
        return 3;
    }
}

/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromSensAddr() {
    if (this -> name_ == "Snare") {
        Serial.println("GetEEpromSensAddr - Snare");
        return 4;
    } else if (this -> name_ == "Bass") {
        return 5;
    } else if (this -> name_ == "T.Tom") {
        return 6;
    } else {
        return 7;
    }
}

/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromTriggerThreshAddr() {
    if (this -> name_ == "Snare") {
        return 8;
    } else if (this -> name_ == "Bass") {
        return 9;
    } else if (this -> name_ == "T.Tom") {
        return 10;
    } else {
        return 11;
    }
}

/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromNumOfLedsAddr() {
    if (this -> name_ == "Snare") {
        return 12;
    } else if (this -> name_ == "Bass") {
        return 13;
    } else if (this -> name_ == "T.Tom") {
        return 14;
    } else {
        return 15;
    }
}
/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromFadeDelayAddr() {
    // if (this -> name_ == "Snare") {
    //     return 12;
    // } else if (this -> name_ == "Bass") {
    //     return 13;
    // } else if (this -> name_ == "T.Tom") {
    //     return 14;
    // } else {
    //     return 15;
    // }
    return 16;
}
/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromFadeStepsAddr() {
    // if (this -> name_ == "Snare") {
    //     return 16;
    // } else if (this -> name_ == "Bass") {
    //     return 17;
    // } else if (this -> name_ == "T.Tom") {
    //     return 18;
    // } else {
    //     return 19;
    // }
    return 17;
}
/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromBlinkLenAddr() {
    // if (this -> name_ == "Snare") {
    //     return 20;
    // } else if (this -> name_ == "Bass") {
    //     return 21;
    // } else if (this -> name_ == "T.Tom") {
    //     return 22;
    // } else {
    //     return 23;
    // }
    return 18;
}
/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromCircleTimeAddr() {
    // if (this -> name_ == "Snare") {
    //     return 24;
    // } else if (this -> name_ == "Bass") {
    //     return 25;
    // } else if (this -> name_ == "T.Tom") {
    //     return 26;
    // } else {
    //     return 27;
    // }
    return 19;
}
/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromFadeIntervalAddr() {
    // if (this -> name_ == "Snare") {
    //     return 24;
    // } else if (this -> name_ == "Bass") {
    //     return 25;
    // } else if (this -> name_ == "T.Tom") {
    //     return 26;
    // } else {
    //     return 27;
    // }
    return 20;
}
/*!
 * Retrieve the EEPROM address for the given drum
 **/
int Drum::GetEepromTriggerDurationAddr() {
    if (this -> name_ == "Snare") {
        return 21;
    } else if (this -> name_ == "Bass") {
        return 22;
    } else if (this -> name_ == "T.Tom") {
        return 23;
    } else {
        return 24;
    }
    // return 19;
}


#pragma endregion

void Drum::SetColor() {
    fill_solid(ledStrip_, numOfLeds_, color_.currColor_);
}

void Drum::SetColor(CRGB color) {
    fill_solid(ledStrip_, numOfLeds_, color);
}

void Drum::SetColor(CRGB color, int index) {
    ledStrip_[index] = color;
}

void Drum::SetColor(CRGB color, int *indecies) {
    for (int i = 0; i < sizeof(indecies); ++i) {
        ledStrip_[i] = color;
    }
}

/*!
 * TODO: Finish implementing
 * Custom pause that allows processes to contiue
 **/
void Drum::Flush(unsigned long duration) {
    unsigned long currMillis = millis();
    unsigned long triggerStartTime = millis();

  // Loop until the time elapsed is greater than or equal to the duration
  while (currMillis - triggerStartTime < duration) {
    currMillis = millis();
  }
}

/*!
 * TODO: Finish implementing
 * Allow for partial colors on top of a whole color
 **/
void Drum::TurnOn() {
    // FastLED.show();
    ledController_ -> showLeds(128);
}

/*!
 * TODO: Finish implementing
 *
 **/
void Drum::TurnOff() {
    this -> SetColor(CRGB::Black);
    // FastLED.show();
    ledController_ -> showLeds(128);
}

/*!
 * TODO: Finish implementing
 * Turn on static white color to led
 **/
void Drum::TurnOnWhite() {
    this -> SetColor(CRGB::White);
    // FastLED.show();
    ledController_ -> showLeds(128);
}


/*!
 * TODO: Finish implementing
 * Initialize fade variables. This starts the fading sequence.
 **/
void Drum::StartFade() {
    if (color_.IsFadeColorMode()) {
        current_fade_interval_index_ = 0;
        is_fade_started_ = true;
    }
}

/*!
 * TODO: Finish implementing
 * Performs fade on a per cycle basis. Gradually lowers pwm value to 0.
 * Grandularity is based on the fade interval. Result of inital value / times 
 * we want to cycle before fade is done (led is off)
 **/
void Drum::EndFade() {
    if (is_fade_started_ && color_.IsFadeColorMode()) {
        // current_fade_interval_index_
        // fadeIntervalVal
        fract8 ratio = current_fade_interval_index_++ / fadeIntervalVal_;
        CRGB blended = blend(color_.currColor_, CRGB::Black, ratio);
        SetColor(blended);
        // FastLED.show();
        ledController_ -> showLeds(128);
        // TurnOn();

        delay(fadeDelayVal_);

        // Once fading is done, mark as done
        if (current_fade_interval_index_ >= fadeIntervalVal_) {
            is_fade_started_ = false;
            is_triggered_ = false;
        }
    }
}

/*!
 * TODO: Finish implementing
 **/
void Drum::StartChase() {

}

/*!
 * TODO: Finish implementing
 **/
void Drum::EndChase() {

}
