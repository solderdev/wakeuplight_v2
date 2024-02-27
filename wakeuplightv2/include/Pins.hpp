#pragma once

#include <Arduino.h>

class Pins
{
public:
  static constexpr uint32_t pwm1 = 27;
  static constexpr uint32_t en_bottom = 21;
  static constexpr uint32_t en_top = 19;
  static constexpr uint32_t button_1 = 17;
  static constexpr uint32_t button_2 = 16;
  static constexpr uint32_t disable_audio_pwr = 14;
  static constexpr uint32_t i2s_dout = 18;
  static constexpr uint32_t i2s_bclk = 4;
  static constexpr uint32_t i2s_lrc = 5;
};