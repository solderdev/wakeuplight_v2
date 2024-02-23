#pragma once

class TaskConfig
{
public:
  static constexpr uint32_t WiFi_http_stacksize = 5000u;  // words
  static constexpr uint32_t WiFi_http_priority = 3;

  static constexpr uint32_t WiFi_ota_stacksize = 5000u;  // words
  static constexpr uint32_t WiFi_ota_priority = 1;

  static constexpr uint32_t Alarm_stacksize = 5000u;  // words
  static constexpr uint32_t Alarm_priority = 2;
};
