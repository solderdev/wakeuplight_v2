#include "LEDControl.hpp"
#include "Storage.hpp"

// https://forum.arduino.cc/t/esp32-mcpwm-trigger/907542/9
// https://github.com/espressif/esp-idf/blob/master/examples/peripherals/mcpwm/mcpwm_bdc_speed_control/main/mcpwm_bdc_control_example_main.c
// alternative (not exact): https://github.com/khoih-prog/ESP32_PWM
// alternative to switch off with one-shot: https://demo-dijiudu.readthedocs.io/en/latest/api-reference/system/esp_timer.html


LEDControl::LEDControl(uint8_t pwm_pin, mcpwm_unit_t mcpwm_unit_id) :
  mcpwm_unit_(mcpwm_unit_id), 
  pwm_pin(pwm_pin)
{
  this->frequency_hz_ = 300;
  this->duty_percent_ = 50.0f;
  
  log_d("init mcpwm driver");
  this->updateTiming(this->frequency_hz_, this->duty_percent_);
  this->setOffMode();
}

void LEDControl::updateTiming(uint32_t frequency_hz, float duty_percent)
{
  if (frequency_hz > 100 && frequency_hz < 30000 &&
      duty_percent > 0.0 && duty_percent < 100.0)
  {
    // do not re-apply same values
    if (this->mode == LEDMODE_PWM && this->frequency_hz_ == frequency_hz && this->duty_percent_ == duty_percent)
      return;
  }
  else
  {
    log_e("ERR Invalid frequency: %u  duty-cycle: %f", frequency_hz, duty_percent);
    return;
  }

  log_i("Setting frequency: %u", frequency_hz);
  log_i("Setting duty-cycle: %f", duty_percent);

  vTaskSuspendAll();

  if (this->mode != LEDMODE_PWM || this->frequency_hz_ != frequency_hz)
  {
    mcpwm_set_duty(this->mcpwm_unit_, MCPWM_TIMER_0, MCPWM_GEN_A, duty_percent);  // 100.0f - 
    mcpwm_gpio_init(this->mcpwm_unit_, MCPWM0A, this->pwm_pin);

    mcpwm_config_t pwm_config = {
        .frequency = frequency_hz,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .duty_mode = MCPWM_DUTY_MODE_0,
        .counter_mode = MCPWM_DOWN_COUNTER,
    };
    mcpwm_init(this->mcpwm_unit_, MCPWM_TIMER_0, &pwm_config);

    mcpwm_set_duty_type(this->mcpwm_unit_, MCPWM_TIMER_0, MCPWM_GEN_A, MCPWM_DUTY_MODE_0);

    // activate timer output
    mcpwm_set_timer_sync_output(this->mcpwm_unit_, MCPWM_TIMER_0, MCPWM_SWSYNC_SOURCE_TEZ);
  }

  mcpwm_set_duty(this->mcpwm_unit_, MCPWM_TIMER_0, MCPWM_GEN_A, duty_percent);  // 100.0f - 
  // mcpwm_set_frequency(this->mcpwm_unit_, MCPWM_TIMER_0, frequency_hz);
  
  // set PWM-mode
  this->mode = LEDMODE_PWM;

  this->frequency_hz_ = frequency_hz;
  this->duty_percent_ = duty_percent;
  
  xTaskResumeAll();
}

uint32_t LEDControl::getFrequencyHz(void)
{
  return this->frequency_hz_;
}

float LEDControl::getDutyPercent(void)
{
  return this->duty_percent_;
}

LEDControl::LEDMode_t LEDControl::getMode(void)
{
  return this->mode;
}

void LEDControl::setOnMode(void)
{
  if (this->mode != LEDMODE_ON) {
    this->mode = LEDMODE_ON;
    mcpwm_set_signal_high(this->mcpwm_unit_, MCPWM_TIMER_0, MCPWM_GEN_A);
  }
}

void LEDControl::setOffMode(void)
{
  if (this->mode != LEDMODE_OFF) {
    this->mode = LEDMODE_OFF;
    mcpwm_set_signal_low(this->mcpwm_unit_, MCPWM_TIMER_0, MCPWM_GEN_A);
  }
}

void LEDControl::setPwmMode(void)
{
  this->updateTiming(this->frequency_hz_, this->duty_percent_);
}

void LEDControl::setFrequency(uint32_t frequency)
{
  this->updateTiming(frequency, this->duty_percent_);
}

void LEDControl::setDutyCycle(float duty_cycle)
{
  if (duty_cycle >= 100.0f)
    this->setOnMode();
  else if (duty_cycle > 0.0f)
    this->updateTiming(this->frequency_hz_, duty_cycle);
  else
    this->setOffMode();
}
