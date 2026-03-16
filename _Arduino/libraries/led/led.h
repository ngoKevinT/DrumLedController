/**
 * led.h
 * 
 * 
 **/

#ifndef Led_h
#define Led_h

#include <Adafruit_PWMServoDriver.h>
// #include <drum.h>

const int DURATION = 100;

const int MAX_PWM_VALUE = 4096;
const int MIN_PWM_VALUE = 0;

const int MAX_SENSITIVITY = 255;
const int MIN_SENSITIVITY = 0;

class Led {
  public: 
    Led(Adafruit_PWMServoDriver *pwm, Drum *drum, int rChannel, int gChannel, int bChannel);
    void TriggerLights();

    Adafruit_PWMServoDriver *pwm_;
    Drum *drum_;
    int r_pwm_channel_;
    int g_pwm_channel_;
    int b_pwm_channel_;
  private:
    int CalculatePwmValue(int pwmVal);
    void TurnOn();
    void TurnOff();

    bool is_triggered_;
    unsigned long trigger_start_time_;
    unsigned long elapsed_time_;

};

#endif