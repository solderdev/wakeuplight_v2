#include "AlarmControl.hpp"
#include "Storage.hpp"
#include "TaskConfig.hpp"
#include "esp_sntp.h"
#include "sunset.h"


#define LATITUDE        47.073540
#define LONGITUDE       15.437691


static struct timeval last_ntp_sync_;
SunSet sun;

void cbSyncTime(struct timeval *tv);


AlarmControl::AlarmControl(LEDControl *led_control) :
  led_control_(led_control),
  task_handle_alarm_(nullptr),
  current_duty_(98.765f),
  duty_max_(95.0f)
{
  this->preferences_ = new Preferences();
  
  // load saved values
  preferences_lock.lock();
  this->preferences_->begin("alarmctrl");

  this->preferences_->getString("alarmtime", this->alarm_time_, sizeof(this->alarm_time_));
  this->alarm_time_[5] = '\0';  // safety
  if (strchr(this->alarm_time_, ':') != this->alarm_time_ + 2)
  {
    strcpy(this->alarm_time_, "06:45");
    this->preferences_->putString("alarmtime", this->alarm_time_);
    log_d("Setting default value for alarmtime: %s", this->alarm_time_);
  }
  memcpy(&this->alarm_time_nvm_, &this->alarm_time_, sizeof(this->alarm_time_nvm_));

  this->alarm_weekend_ = this->preferences_->getBool("alarmwe", false);
  this->alarm_weekend_nvm_ = this->alarm_weekend_;

  this->fade_minutes_ = this->preferences_->getUInt("fade_min", 0);
  if (this->fade_minutes_ == 0 || this->fade_minutes_ >= 150)
  {
    this->fade_minutes_ = 30;
    this->preferences_->putUInt("fade_min", this->fade_minutes_);
    log_d("Setting default value for fade_minutes: %u", this->fade_minutes_);
  }
  this->fade_minutes_nvm_ = this->fade_minutes_;

  this->snooze_minutes_ = this->preferences_->getUInt("snooze_min", 0);
  if (this->snooze_minutes_ > 200)
  {
    this->snooze_minutes_ = 30;
    this->preferences_->putUInt("snooze_min", this->snooze_minutes_);
    log_d("Setting default value for snooze_min: %u", this->snooze_minutes_);
  }
  this->snooze_minutes_nvm_ = this->snooze_minutes_;

  this->duty_lights_on_ = this->preferences_->getFloat("duty_lights_on", 0.0f);
  if (this->duty_lights_on_<= 0.0f || this->duty_lights_on_ > 100.0f)
  {
    this->duty_lights_on_ = 60.0f;
    this->preferences_->putFloat("duty_lights_on", this->duty_lights_on_);
    log_d("Setting default value for duty_lights_on: %.2f", this->duty_lights_on_);
  }
  this->duty_lights_on_nvm_ = this->duty_lights_on_;

  this->mode_ = (AlarmMode_t)(this->preferences_->getUChar("mode", ALARMMODE_ALARM_ON));
  if ((uint8_t)(this->mode_) >= ALARMMODE_LIMIT)
  {
    this->mode_ = ALARMMODE_ALARM_ON;
    this->preferences_->putUChar("mode", (uint8_t)(this->mode_));
    log_d("Setting default value for mode: %u", this->mode_);
  }
  this->mode_nvm_ = this->mode_;

  this->preferences_->end();
  preferences_lock.unlock();

  last_ntp_sync_.tv_sec = 100812;  // set it to something other than 1.1.1970
  last_ntp_sync_.tv_usec = 0;
  sntp_set_time_sync_notification_cb(cbSyncTime);  // set a Callback function for time synchronization notification

  sun.setPosition(LATITUDE, LONGITUDE, 0);  // set timezone later to account for DST

  this->sunrise_minutes = 8 * 60;
  this->sunset_minutes = 20 * 60;

  log_i("Initialized Alarm with: alarm_time_=%s alarm_weekend_=%s fade_minutes_=%u duty_max_=%.2f duty_lights_on=%.2f snooze_minutes=%u", 
        this->alarm_time_, (this->alarm_weekend_)?"true":"false", this->fade_minutes_, this->duty_max_, this->duty_lights_on_, this->snooze_minutes_);

  if (xTaskCreate(&AlarmControl::task_alarm_wrapper, "task_alarm", TaskConfig::Alarm_stacksize, this, TaskConfig::Alarm_priority, &task_handle_alarm_) != pdPASS)
    log_e("Alarm ERROR task init failed");
}

String AlarmControl::getAlarmTime(void)
{
  return String(this->alarm_time_);
}

bool AlarmControl::getAlarmWeekend(void)
{
  return this->alarm_weekend_;
}

uint32_t AlarmControl::getFadeMinutes(void)
{
  return this->fade_minutes_;
}

float AlarmControl::getDutyMax(void)
{
  return this->duty_max_;
}

float AlarmControl::getDutyLightsOn(void)
{
  return this->duty_lights_on_;
}

AlarmControl::AlarmMode_t AlarmControl::getMode(void)
{
  return this->mode_;
}

float AlarmControl::getCurrentDuty(void)
{
  return this->current_duty_;
}

uint32_t AlarmControl::getSnoozeMinutes(void)
{
  return this->snooze_minutes_;
}

void AlarmControl::setMode(AlarmControl::AlarmMode_t mode)
{
  log_d("setMode %u", mode);
  this->mode_ = mode;
}

void AlarmControl::setOnMode(void)
{
  float new_duty;
  log_d("setOnMode");
  // pre-set intensity depending on time vs. sunrise / sunset
  if (this->timeinfo_.tm_hour * 60 + this->timeinfo_.tm_min > this->sunrise_minutes && 
      this->timeinfo_.tm_hour * 60 + this->timeinfo_.tm_min < this->sunset_minutes)
    new_duty = this->duty_max_;
  else
    new_duty = 20;
  this->current_duty_ = new_duty;
  this->duty_lights_on_ = new_duty;
  this->setMode(ALARMMODE_FORCE_ON);
}

void AlarmControl::setAlarmOFF(void)
{
  log_d("setAlarmOFF");
  this->setMode(ALARMMODE_ALARM_OFF);
  this->current_duty_ = 0.0f;
}

void AlarmControl::setAlarmON(void)
{
  log_d("setAlarmON");
  this->setMode(ALARMMODE_ALARM_ON);
  this->current_duty_ = 0.0f;
}

void AlarmControl::setAlarmTime(String alarm_time)
{
  log_d("setAlarmTime %s", alarm_time);
  strcpy(this->alarm_time_, alarm_time.c_str());
}

void AlarmControl::setAlarmWeekend(bool alarm_weekend)
{
  log_d("setAlarmWeekend %d", alarm_weekend);
  this->alarm_weekend_ = alarm_weekend;
}

void AlarmControl::setFadeMinutes(uint32_t fade_minutes)
{
  log_d("setFadeMinutes %u", fade_minutes);
  this->fade_minutes_ = fade_minutes;
}

void AlarmControl::setDutyMax(float duty)
{
  if (duty >= 0.0 && duty <= 100.0)
  {
    log_d("setDutyMax %.2f", duty);
    this->duty_max_ = duty;
  }
  else
    log_w("setDutyMax failed: %.2f", duty);
}

void AlarmControl::setDutyLightsOn(float duty)
{
  // duty parameter is 0 --> 100 and will be scaled to actual max duty

  if (duty >= 0.0 && duty <= 100.0)
  {
    log_d("setDutyLightsOn %.2f", duty);
    this->duty_lights_on_ = duty / 100.0f * this->duty_max_;
    if (this->mode_ == ALARMMODE_FORCE_ON)
      this->current_duty_ = this->duty_lights_on_;
  }
  else
    log_w("setDutyLightsOn failed: %.2f", duty);
}

void AlarmControl::setSnoozeMinutes(uint32_t snooze_minutes)
{
  log_d("setSnoozeMinutes %u", snooze_minutes);
  this->snooze_minutes_ = snooze_minutes;
}

struct tm AlarmControl::getCurrentTime(void)
{
  return this->timeinfo_;
}

extern AlarmControl *alarm_control;
void cbSyncTime(struct timeval *tv)  // callback function to show when NTP was synchronized
{
  static int currentDay = 99;

  log_i("NTP time synced");
  last_ntp_sync_.tv_sec = tv->tv_sec;
  last_ntp_sync_.tv_usec = tv->tv_usec;

  // get time zone offset
  std::time_t epoch_plus_11h = 60 * 60 * 11;
  struct tm local_time, gm_time;
  localtime_r(&epoch_plus_11h, &local_time);
  gmtime_r(&epoch_plus_11h, &gm_time);
  const int tz_diff = difftime(mktime(&local_time), mktime(&gm_time)) / 3600;
  struct tm *ntpsync = localtime(&last_ntp_sync_.tv_sec);

  // check every day after 4am (consider daylight saving time changes)
  if (currentDay != ntpsync->tm_mday && ntpsync->tm_hour >= 4)
  {
    currentDay = ntpsync->tm_mday;

    sun.setCurrentDate(1900 + ntpsync->tm_year, 1 + ntpsync->tm_mon, ntpsync->tm_mday);
    sun.setTZOffset(tz_diff + ((ntpsync->tm_isdst)?1:0));  // adjust for daylight saving time

    alarm_control->sunrise_minutes = sun.calcSunrise();
    alarm_control->sunset_minutes = sun.calcSunset();
  }
}

struct timeval AlarmControl::getLastNTPSync(void)
{
  return last_ntp_sync_;
}

void AlarmControl::task_alarm_wrapper(void *arg)
{
  AlarmControl* alarm_ctrl = static_cast<AlarmControl *>(arg);
  alarm_ctrl->task_alarm();
}
void AlarmControl::task_alarm()
{
  #define RAMP_EXP_OFFSET 0.8f
  #define RAMP_EXP_WIDTH  5.0f
  struct tm timeinfo_alarm;
  float duty_calc;
  float minutes_diff;
  float exp_calc_max, exp_calc_min;

  this->led_control_->setOffMode();

  exp_calc_min = 1.0f / (RAMP_EXP_OFFSET + expf(RAMP_EXP_WIDTH));
  exp_calc_max = 1.0f / (RAMP_EXP_OFFSET + expf(-RAMP_EXP_WIDTH));
  
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(100));

    // check and save new values
    preferences_lock.lock();
    if (this->alarm_weekend_ != this->alarm_weekend_nvm_)
    {
      this->alarm_weekend_nvm_ = this->alarm_weekend_;
      this->preferences_->begin("alarmctrl");
      this->preferences_->putBool("alarmwe", this->alarm_weekend_);
      this->preferences_->end();
    }
    if (this->snooze_minutes_ != this->snooze_minutes_nvm_)
    {
      this->snooze_minutes_nvm_ = this->snooze_minutes_;
      this->preferences_->begin("alarmctrl");
      this->preferences_->putUInt("snooze_min", this->snooze_minutes_);
      this->preferences_->end();
    }
    if (this->duty_lights_on_ != this->duty_lights_on_nvm_)
    {
      this->duty_lights_on_nvm_ = this->duty_lights_on_;
      this->preferences_->begin("alarmctrl");
      this->preferences_->putFloat("duty_lights_on", this->duty_lights_on_nvm_);
      this->preferences_->end();
    }
    if (this->fade_minutes_ != this->fade_minutes_nvm_)
    {
      this->fade_minutes_nvm_ = this->fade_minutes_;
      this->preferences_->begin("alarmctrl");
      this->preferences_->putUInt("fade_min", this->fade_minutes_);
      this->preferences_->end();
    }
    if (String(this->alarm_time_) != String(this->alarm_time_nvm_))
    {
      memcpy(&this->alarm_time_nvm_, &this->alarm_time_, sizeof(this->alarm_time_nvm_));
      this->preferences_->begin("alarmctrl");
      this->preferences_->putString("alarmtime", this->alarm_time_);
      this->preferences_->end();
    }
    if (this->mode_ != this->mode_nvm_)
    {
      this->mode_nvm_ = this->mode_;
      this->preferences_->begin("alarmctrl");
      this->preferences_->putUChar("mode", (uint8_t)(this->mode_));
      this->preferences_->end();
    }
    preferences_lock.unlock();

    switch (this->mode_)
    {
    case ALARMMODE_FORCE_ON:
      // we are instructed to stay on
      this->led_control_->setDutyCycle(this->duty_lights_on_);
      getLocalTime(&this->timeinfo_, 5);
      break;

    case ALARMMODE_ALARM_ON:
      if (!getLocalTime(&this->timeinfo_, 100))
      {
        log_w("Failed to obtain time");
        // do not change anything
        vTaskDelay(pdMS_TO_TICKS(1000));
        break;
      }
      // struct tm timeinfo:
      // Member    Type  Meaning Range
      // tm_sec    int   seconds after the minute  0-61 (0-59)
      // tm_min    int   minutes after the hour    0-59
      // tm_hour   int   hours since midnight      0-23
      // tm_mday   int   day of the month          1-31
      // tm_mon    int   months since January      0-11
      // tm_year   int   years since 1900
      // tm_wday   int   days since Sunday         0-6
      // tm_yday   int   days since January 1      0-365
      // tm_isdst  int   Daylight Saving Time flag

      // if we are on a weekend and option is off --> disable
      if ((this->timeinfo_.tm_wday == 6 || this->timeinfo_.tm_wday == 0) && this->alarm_weekend_ == false)
      {
        this->led_control_->setOffMode();
        this->current_duty_ = 0.0f;
        break;
      }

      // copy "now" for calculations
      memcpy(&timeinfo_alarm, &this->timeinfo_, sizeof(this->timeinfo_));
      timeinfo_alarm.tm_hour = String(this->alarm_time_).substring(0, 2).toInt();
      timeinfo_alarm.tm_min = String(this->alarm_time_).substring(3, 5).toInt();
      timeinfo_alarm.tm_sec = 0;

      // calculate distance to alarm time
      minutes_diff = difftime(mktime(&this->timeinfo_), mktime(&timeinfo_alarm)) / 60.0f;

      // uint8_t start_h = String(this->alarm_time_).substring(0, 2).toInt();
      // uint8_t start_m = String(this->alarm_time_).substring(3, 5).toInt();
      // uint8_t end_h = (uint8_t)(((uint32_t)(start_h) * 60 + start_m + this->fade_minutes_) / 60) % 24;
      // uint8_t end_m = (start_m + this->fade_minutes_) % 60;

      if (minutes_diff < 0.0f)
      {
        // too early
        log_d("outside of fading time. min diff: %f", minutes_diff);
        this->led_control_->setOffMode();
        this->current_duty_ = 0.0f;
      }
      else if (minutes_diff < this->fade_minutes_)
      {
        // if we are inside alarm time: calculate current fade duty cycle
        // duty_calc = minutes_diff * this->duty_max_ / this->fade_minutes_;

        // calculate sigmoid ramp with top at duty_max
        duty_calc = (this->duty_max_ / (exp_calc_max - exp_calc_min)) * 
                    (1.0f / (RAMP_EXP_OFFSET + expf(-RAMP_EXP_WIDTH * (2.0f * minutes_diff - this->fade_minutes_)/this->fade_minutes_))
                    - exp_calc_min);

        log_d("inside alarm time. min diff: %f duty calc: %f", minutes_diff, duty_calc);
        this->led_control_->setDutyCycle(duty_calc);
        this->current_duty_ = duty_calc;
      }
      else if (minutes_diff < this->fade_minutes_ + this->snooze_minutes_)
      {
        log_d("after alarm time. min diff: %f duty max: %f", minutes_diff, this->duty_max_);
        this->led_control_->setDutyCycle(this->duty_max_);
        this->current_duty_ = this->duty_max_;
      }
      else
      {
        // we are after fading time
        log_d("outside of fading time. min diff: %f", minutes_diff);
        this->led_control_->setOffMode();
        this->current_duty_ = 0.0f;
      }
      break;
    
    case ALARMMODE_ALARM_OFF:
      // alarm is off
      this->led_control_->setOffMode();
      this->current_duty_ = 0.0f;
      getLocalTime(&this->timeinfo_, 5);
      break;
    
    default:
      break;
    }
  }
}
