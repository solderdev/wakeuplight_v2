// #define CORE_DEBUG_LEVEL  ARDUHAL_LOG_LEVEL_DEBUG

#include <Arduino.h>
#include <nvs_flash.h>
#include "WebInterface.hpp"
#include "Pins.hpp"
#include "LEDControl.hpp"
#include "AlarmControl.hpp"

#include "time.h"

#include "esp32-hal-log.h"
#include "esp_sntp.h"

WebInterface *web_interface;
LEDControl *led_control;
AlarmControl *alarm_control;

static uint32_t systime_multiplier = 1;

void systime_init()
{
  systime_multiplier = 80000000 / getApbFrequency();
}

unsigned long IRAM_ATTR systime_ms()
{
  return (unsigned long) ((esp_timer_get_time() * systime_multiplier) / 1000ULL);
}

void setup()
{
  setCpuFrequencyMhz(80);
  systime_init();

  esp_log_level_set("*", ESP_LOG_DEBUG);

  // COMPLETELY ERASE NVS
  // nvs_flash_erase(); // erase the NVS partition and...
  // nvs_flash_init(); // initialize the NVS partition.
  // while(true);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(2000);
  log_d("Hi there! Booting now..");
  log_d("CPU clock: %u MHz / using systime multiplier %u with ApbFrequency %u", getCpuFrequencyMhz(), systime_multiplier, getApbFrequency());
  
  led_control = new LEDControl(Pins::pwm1, MCPWM_UNIT_0);

  alarm_control = new AlarmControl(led_control);

  web_interface = new WebInterface(alarm_control);

  // change NTP update interval
  sntp_set_sync_interval(1 * 60 * 60 * 1000UL); // 1 hours

  // time-zone needs to be set manually, otherwise DST switch date is wrong
  // by https://werner.rothschopf.net/microcontroller/202103_arduino_esp32_ntp_en.htm
  configTime(0, 0, "time.metrologie.at");
  // list of TZ strings: https://github.com/nayarsystems/posix_tz_db/eblob/master/zones.csv
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);           // Set environment variable with your time zone
  tzset();
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(5551));
  // log_d("ON");
  // led_control->setOnMode();
  // vTaskDelay(pdMS_TO_TICKS(3000));
  // log_d("Off");
  // led_control->setOffMode();

  // struct tm timeinfo;
  // if(!getLocalTime(&timeinfo, 0)){
  //   log_w("Failed to obtain time");
  // }
  // else
  //   log_d("%04u-%02u-%02u %02u:%02u:%02u", 
  //         1900 + timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday, 
  //         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // log_d("systime: %u", systime_ms());
}
