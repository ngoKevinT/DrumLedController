/**
 * drum.h (11/05/2024) 
 * 
 * Note: Nextion.h was modified to expose protected methods
 **/

#ifndef Drum_h
#define Drum_h

#include <EEPROM.h>
#include <FastLED.h>
#include <Nextion.h>
#include <Wire.h>
#include <display.h>
#include "ledColor.h"

// const int DURATION = 100;
// const int FADE_STATIC_DURATION = 20;

// const uint8_t FADE_INTERVAL = 20;
// const uint8_t FADE_DELAY = 1;

// TODO: LED BRIGHTNESS setting

class Drum {
    public:
        Drum();
        Drum(String name, NexText *hitIndicator, NexButton *selectColorPreviewBtn, NexText *drumNameLabel, 
            NexText *drumInfoLabel, NexNumber *drumSensLabel, NexNumber *drumReading, int triggerPin, 
            CRGB *ledStrip, uint8_t numOfLeds, CLEDController *ledController
        );
        void SetCurrentDrum(bool isCurrent);
        void SetDrumInfo();
        void SetDrumTriggerReading(int value);
        void ShowHitIndicator(bool isShow);
        void CycleColorMode(bool next);
        void TriggerHit();
        void TriggerHitEnd();
        bool IsTriggered();

        uint8_t ReadSensitivityFactor();
        void UpdateSensitivityFactor(uint8_t newVal);
        uint8_t ReadSensitivityValue();
        void UpdateSensitivityValue(uint8_t newVal);
        uint8_t ReadTriggerThreshValue();
        void UpdateTriggerThreshValue(uint8_t newVal);
        uint8_t ReadTriggerDurationValue();
        void UpdateTriggerDurationValue(uint8_t newVal);
        // uint8_t ReadNumOfLedsValue();
        // void UpdateNumOfLedsValue(uint8_t newVal);

        static uint8_t ReadBlinkLenValue();
        static void UpdateBlinkLenValue(uint8_t newVal);
        static uint8_t ReadFadeDelayValue();
        static void UpdateFadeDelayValue(uint8_t newVal);
        static uint8_t ReadFadeStepsValue();
        static void UpdateFadeStepsValue(uint8_t newVal);
        static uint8_t ReadFadeIntervalValue();
        static void UpdateFadeIntervalValue(uint8_t newVal);
        static uint8_t ReadCircleTimeValue();
        static void UpdateCircleTimeValue(uint8_t newVal);

        // TODO: static array of preset modes (name, color, and modes to be set across all drums) -> store a vector of all pertinent info, 
        // helper function to help set the state

        String name_;
        LedColor color_;

        int trigger_pin_;
        int data_pin_;
        int clock_pin_;

        bool is_current_drum_;
        bool is_triggered_;

        NexText *hitIndicator_;
        NexButton *selectColorPreviewBtn_;
        NexText *drumNameLabel_;
        NexText *drumInfoLabel_;
        NexNumber *drumSensLabel_;
        NexNumber *drumReading_;

        CRGB *ledStrip_;
        CLEDController *ledController_;

        uint8_t sensVal_;
        uint8_t sensFactorVal_;
        uint8_t triggerTimeVal_;
        uint8_t triggerDurationVal_;

        uint8_t numOfLeds_;

        static inline uint8_t blinkLenVal_;
        static inline uint8_t fadeDelayVal_;
        static inline uint8_t fadeStepVal_;
        static inline uint8_t fadeIntervalVal_;
        static inline uint8_t circleTimeVal_;

        void TriggerLights();
        void TriggerLightsEnd();

        bool IsTriggerValid(int rVal);
    private:
        int GetEepromSensAddr();
        int GetEepromSensFactorAddr();
        int GetEepromTriggerThreshAddr();
        int GetEepromTriggerDurationAddr();
        int GetEepromNumOfLedsAddr();

        static int GetEepromBlinkLenAddr();
        static int GetEepromFadeDelayAddr();
        static int GetEepromFadeStepsAddr();
        static int GetEepromFadeIntervalAddr();
        static int GetEepromCircleTimeAddr();

        void SetColor();
        void SetColor(CRGB color);
        void SetColor(CRGB color, int index);
        void SetColor(CRGB color, int *indecies);

        void Flush(unsigned long duration);

        void TurnOn();
        void TurnOff();
        void TurnOnWhite();

        void StartFade();
        void EndFade();

        void StartChase();
        void EndChase();

        unsigned long trigger_start_time_;
        uint8_t current_fade_interval_index_;
        bool is_fade_started_;
};

#endif
