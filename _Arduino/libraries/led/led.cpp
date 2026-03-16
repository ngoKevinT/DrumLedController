/**
 * led.cpp
 * 
 * 
 * 
 **/ 
#include "led.h"


/*!
 * Initialize Led Object. Controls turning on and off the led strip based on the address via params.
 * 
 * TODO: Add ability to add custom duration time (ms). Currently default is 100ms (DURATION variable)
 */
Led::Led(Adafruit_PWMServoDriver *pwm, Drum *drum, int rChannel, int gChannel, int bChannel) {
    pwm_ = pwm;
    drum_ = drum;
    r_pwm_channel_ = rChannel;
    g_pwm_channel_ = gChannel;
    b_pwm_channel_ = bChannel;
    is_triggered_ = false;
}

/*!
 *
 *
 * TODO: For static rainbow mode (white then flash color on hit),
 * we're going to need to change the timer logic. Maybe new functionality to get it working.
 **/
void Led::TriggerLights() {
    // Pull in color and mode from the drum object
    // Turn on the color
    // Turn off the color depending on the length
    // Should have access to the pwm object created in main

    // Whoever calls this function, up the chain needs to update the color of the drum if necessary

    // On first called and triggered, turn on led and trigger timer.
    if (!is_triggered_) {
        TurnOn();
        trigger_start_time_ = millis();
    }

    is_triggered_ = true;

    // Timer has reached the limit, turn off led - replaces delay and need for parallelism
    if ((millis() - trigger_start_time_) > DURATION) {
     is_triggered_ = false;
     TurnOff();
   }
}


/*!
 *
 *
 **/
int Led::CalculatePwmValue(int pwmVal) {
    // NOTE: Thinking about it, each drum might need to run in parallel/"concurrently"
    // Given the val, which represents for sensitivity for the time being:
    // calculate the percentage out of 255 to get the percentage for what the pwm value should be.
    return (pwmVal / MAX_PWM_VALUE) * MAX_PWM_VALUE;
}

/*!
 * Calculate the pwm result based off of the sensitivity of the drum
 * Show r,g,b colors via pwm
 * pwm.setPWM(pin, 4096, 0)
 * 
 * TODO: For "fade" mode, we're going to have to manage the pwm value more
 * Turn on, once it hits a threshold start to fade out. Depends on freq? osc? of pwm board?
 * Would need to step each color channel down gradually and/or adjust freq/osc
 **/
void Led::TurnOn() {
    // &drum_.ShowHitIndicator(true);
    // &pwm_.setPWM(r_pwm_channel_, CalculatePwmValue(drum.color_.r_value_), 0);
    // &pwm_.setPWM(g_pwm_channel_, CalculatePwmValue(drum.color_.g_value_), 0);
    // &pwm_.setPWM(b_pwm_channel_, CalculatePwmValue(drum.color_.b_value_), 0);
    drum_ -> ShowHitIndicator(true);
    pwm_ -> setPWM(r_pwm_channel_, CalculatePwmValue(drum.color_.r_value_), 0);
    pwm_ -> setPWM(g_pwm_channel_, CalculatePwmValue(drum.color_.g_value_), 0);
    pwm_ -> setPWM(b_pwm_channel_, CalculatePwmValue(drum.color_.b_value_), 0);
}

/*!
 * Calculate the pwm result based off of the sensitivity of the drum
 * Turn off r,g,b colors via pwm
 * pwm.setPWM(pin, 0, 4096);
 *
 * TODO: For "fade" mode, we're going to have to manage the pwm value more
 * Turn on, once it hits a threshold start to fade out. Depends on freq? osc? of pwm board?
 * Would need to step each color channel down gradually and/or adjust freq/osc
 **/
void Led::TurnOff() {
    // &drum_.ShowHitIndicator(false);
    // &pwm_.setPWM(r_pwm_channel_, 0, CalculatePwmValue(drum.color_.r_value_));
    // &pwm_.setPWM(g_pwm_channel_, 0, CalculatePwmValue(drum.color_.g_value_));
    // &pwm_.setPWM(b_pwm_channel_, 0, CalculatePwmValue(drum.color_.b_value_));
    drum_ -> ShowHitIndicator(false);
    pwm_ -> setPWM(r_pwm_channel_, 0, CalculatePwmValue(drum.color_.r_value_));
    pwm_ -> setPWM(g_pwm_channel_, 0, CalculatePwmValue(drum.color_.g_value_));
    pwm_setPWM(b_pwm_channel_, 0, CalculatePwmValue(drum.color_.b_value_));
}