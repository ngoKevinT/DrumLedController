/**
 * ledColor.h (8/22/24)
 * 
 * Header file for ledColor.cpp - Handles color state and mode for the given instance
 **/

#ifndef LedColor_h 
#define LedColor_h

#include "Arduino.h"
#include "FastLED.h"

enum ColorMode {
  Blink,
  BlinkRainbow,
  AlwaysOnWhiteBlink,
  AlwaysOnWhiteBlinkRainbow,

  AlwaysOnBlinkWhite,

  Fade,
  FadeRainbow,

  Chase,
  ChaseRainbow,
  AlwaysOnWhiteChase,
  AlwaysOnWhiteChaseRainbow
};

const int num_of_colors = 13;

class LedColor {
  public: 
    LedColor();
    LedColor(CRGB color, ColorMode mode);
    void SetPrevColor();
    void SetNextColor();
    void SetPrevColorMode();
    void SetNextColorMode();
    void NextRainbowColor();
    String ToColorName();   // "S:RRR|GGG|BBB"
    uint16_t ToDisplayColorValue(); // Decimal RGB 565
    CRGB GetWhiteColor();

    bool IsRainbowColorMode();
    bool IsStaticWhiteColorMode();
    bool IsFadeColorMode();

    ColorMode mode_;
    CRGB currColor_;
  private:
    String ToDisplayModeValue();
    void InitColorObjects();
    void SetColor(int index);
    void SetColor();

    int cIndex;
    CRGB colors[num_of_colors];
};

#endif
