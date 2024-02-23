#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "LEDControl.hpp"

class AlarmControl
{
public:
  typedef enum {
    ALARMMODE_FORCE_ON = 0,
    ALARMMODE_ALARM_ON = 1,
    ALARMMODE_ALARM_OFF = 2,
    ALARMMODE_LIMIT
  } AlarmMode_t;

  explicit AlarmControl(LEDControl *led_control);
  AlarmControl(AlarmControl const&) = delete;
  void operator=(AlarmControl const&)  = delete;

  String getAlarmTime(void);
  bool getAlarmWeekend(void);
  uint32_t getFadeMinutes(void);
  float getDutyMax(void);
  float getDutyLightsOn(void);
  AlarmMode_t getMode(void);
  float getCurrentDuty(void);
  uint32_t getSnoozeMinutes(void);
  void setOnMode(void);
  void setMode(AlarmControl::AlarmMode_t mode);
  void setAlarmOFF(void);
  void setAlarmON(void);
  void setAlarmTime(String alarm_time);
  void setAlarmWeekend(bool alarm_weekend);
  void setFadeMinutes(uint32_t fade_minutes);
  void setDutyMax(float duty);
  void setDutyLightsOn(float duty);
  void setSnoozeMinutes(uint32_t snooze_minutes);
  struct tm getCurrentTime(void);
  struct timeval getLastNTPSync(void);

  uint32_t sunrise_minutes;  // minutes after midnight
  uint32_t sunset_minutes;  // minutes after midnight

private:
  static void task_alarm_wrapper(void *arg);
  void task_alarm();
  LEDControl *led_control_;
  Preferences *preferences_;
  struct tm timeinfo_;
  char alarm_time_[10];
  char alarm_time_nvm_[10];
  bool alarm_weekend_;
  bool alarm_weekend_nvm_;
  uint32_t fade_minutes_;
  uint32_t fade_minutes_nvm_;
  float duty_max_;
  uint32_t snooze_minutes_;
  uint32_t snooze_minutes_nvm_;
  float duty_lights_on_;
  float duty_lights_on_nvm_;
  float current_duty_;
  AlarmMode_t mode_;
  AlarmMode_t mode_nvm_;

  TaskHandle_t task_handle_alarm_;
};
