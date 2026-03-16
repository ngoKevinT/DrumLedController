/**
 * ledColor.cpp (8/22/24)
 * 
 * Main class that handles color properties for the drum's led output.
 **/
#include "ledColor.h"
#include <FastLED.h>

/*!
 * Constructor. Initialize with default color (white) and static color mode
 **/
LedColor::LedColor() {
    currColor_ = CRGB::White;
    mode_ = ColorMode::Blink;

    // Initialize static color objects
    InitColorObjects();
}

/*!
 * Constructor. Initialize color object with parameters
 **/
LedColor::LedColor(CRGB color, ColorMode mode) {
    currColor_ = color;
    mode_ = mode;

    // Initialize static color objects
    InitColorObjects();
}


/*!
 * Sets the previous color in the default color options based on the current mode
 **/
void LedColor::SetPrevColor() {
    if(--cIndex < 0) {
        cIndex = num_of_colors - 1;
    }

    // For rainbow modes, do not allow the first entry (white) to be set. (Design choice)
    if (IsRainbowColorMode() && cIndex == 0) {
        cIndex = 1;
    }

    SetColor();
}

/*!
 * Sets the next color in the default color options based on the current mode.
 **/
void LedColor::SetNextColor() {
    if(++cIndex > num_of_colors - 1) {
        cIndex = 0;

        // For rainbow modes, do not allow the first entry (white) to be set. (Design choice)
        if (IsRainbowColorMode() && cIndex == 0) {
            cIndex = 1;
        }
    }

    SetColor();
}

/*!
 * Sets the prev color mode in the default color mode options
 * 10 -> last valid color mode (11 total color modes)
 **/
void LedColor::SetPrevColorMode() {
    mode_ = static_cast<ColorMode>((mode_ - 1) < 0 ? 10 : mode_ - 1);
    cIndex = IsRainbowColorMode() ? 2 : 0;
}


/*!
 * Sets the next color mode in the default color mode options
 * 11 -> # of color modes available
 **/
void LedColor::SetNextColorMode() {
    mode_ = static_cast<ColorMode>((mode_ + 1) % 11);
    cIndex = IsRainbowColorMode() ? 2 : 0;

    SetColor(cIndex);
}

/*!
 * Sets the next color if in rainbow mode
 **/
void LedColor::NextRainbowColor() {
    if (IsRainbowColorMode()) {
        SetNextColor();
    }
}

/*!
 * Convert color into it's fully fleged name for the display
 * <MODE>:RRR|GGG|BBB
 **/
String LedColor::ToColorName() {
    return ToDisplayModeValue() + ":" + currColor_.r + "|" + currColor_.g + "|" + currColor_.b;
}

/*!
 * Converts RGB888 to RGB565 value
 **/
uint16_t LedColor::ToDisplayColorValue() {
    return (((currColor_.r & 0xf8) << 8) + ((currColor_.g & 0xfc) << 3) + (currColor_.b >> 3));
}


/*!
 * Return the string representing the current color mode
 **/
String LedColor::ToDisplayModeValue() {
    switch(mode_) {
        case ColorMode::Blink: 
            return "B_";
        case ColorMode::BlinkRainbow: 
            return "Br";
        case ColorMode::AlwaysOnWhiteBlink:
            return "AOwB_";
        case ColorMode::AlwaysOnWhiteBlinkRainbow:
            return "AOwBr";
        case ColorMode::AlwaysOnBlinkWhite: 
            return "AO_Bw";
        case ColorMode::Fade: 
            return "F";
        case ColorMode::FadeRainbow: 
            return "Fr";
        case ColorMode::Chase: 
            return "C";
        case ColorMode::ChaseRainbow: 
            return "Cr";
        case ColorMode::AlwaysOnWhiteChase: 
            return "AOWC_";
        case ColorMode::AlwaysOnWhiteChaseRainbow: 
            return "AOWCr";
        default: 
            return "XX";
    }
}

/*!
 * Return the string representing the current color mode
 **/
CRGB LedColor::GetWhiteColor() {
    return colors[0];
}


/*!
 * Initializes values and default color modes
 **/
void LedColor::InitColorObjects() {
    colors[0] = CRGB::White;
    colors[1] = CRGB::Pink;
    colors[2] = CRGB::Red;
    colors[3] = CRGB::Orange;
    colors[4] = CRGB::Yellow;
    colors[5] = CRGB::Lime;
    colors[6] = CRGB::Green;
    colors[7] = CRGB::SeaGreen;
    colors[8] = CRGB::Cyan;
    colors[9] = CRGB::SkyBlue;
    colors[10] = CRGB::Blue;
    colors[11] = CRGB::Purple;
    colors[12] = CRGB::Lavender;

    cIndex = 0;
}

/*!
 * Set the current color values to the specified color index based off the default colors
 **/
void LedColor::SetColor(int index) {
    currColor_ = colors[index];
}

/*!
 * Set the current color values to the current color index based off the default colors
 **/
void LedColor::SetColor() {
    SetColor(cIndex);
}

bool LedColor::IsRainbowColorMode() {
    return mode_ == ColorMode::BlinkRainbow
        || mode_ == ColorMode::AlwaysOnWhiteBlinkRainbow
        || mode_ == ColorMode::FadeRainbow
        || mode_ == ColorMode::ChaseRainbow
        || mode_ == ColorMode::AlwaysOnWhiteChaseRainbow;
}

bool LedColor::IsStaticWhiteColorMode() {
    return mode_ == ColorMode::AlwaysOnWhiteBlink
        || mode_ == ColorMode::AlwaysOnWhiteBlinkRainbow
        || mode_ == ColorMode::AlwaysOnWhiteChase
        || mode_ == ColorMode::AlwaysOnWhiteChaseRainbow;
}

bool LedColor::IsFadeColorMode() {
    return mode_ == ColorMode::Fade
        || mode_ == ColorMode::FadeRainbow;
}