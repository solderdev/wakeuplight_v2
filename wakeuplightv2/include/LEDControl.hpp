#pragma once

#include <Arduino.h>
#include "driver/mcpwm.h"

class LEDControl
{
public:
  typedef enum {
    LEDMODE_ON = 0,
    LEDMODE_OFF = 1,
    LEDMODE_PWM = 2
  } LEDMode_t;

  LEDControl(uint8_t pwm_pin, mcpwm_unit_t mcpwm_unit_id, uint8_t en_top_pin, uint8_t en_bottom_pin);
  LEDControl(LEDControl const&) = delete;
  void operator=(LEDControl const&)  = delete;

  void updateTiming(uint32_t frequency_hz, float duty_percent);
  uint32_t getFrequencyHz(void);
  float getDutyPercent(void);
  LEDMode_t getMode(void);
  void setOnMode(void);
  void setOffMode(void);
  void setPwmMode(void);
  void setFrequency(uint32_t frequency);
  void setDutyCycle(float duty_cycle);

private:
  uint8_t pwm_pin;
  mcpwm_unit_t mcpwm_unit_;
  uint8_t en_top_pin;
  uint8_t en_bottom_pin;
  uint32_t frequency_hz_;
  float duty_percent_;
  LEDMode_t mode = LEDMODE_PWM; //on == 0, off == 1, pwm == 2
};
