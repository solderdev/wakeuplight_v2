#pragma once

#include <Arduino.h>
#include "WebServer.h"  // https://github.com/me-no-dev/ESPAsyncWebServer/issues/418
#include <ESPAsyncWebServer.h>
#include <esp_http_client.h>

#include "LEDControl.hpp"
#include "AlarmControl.hpp"

class WebInterface
{
public:
  explicit WebInterface(AlarmControl *alarm_control);
  WebInterface(WebInterface const&) = delete;
  void operator=(WebInterface const&)  = delete;

  AlarmControl* getAlarmControl();

private:
  String build_parameter_string(void);
  static void task_http_wrapper(void *arg);
  void task_http();
  static void task_ota_wrapper(void *arg);
  void task_ota();
  void wifiReconnect();
  void wifiCheckConnectionOrReconnect();

  AsyncWebServer server_;
  AsyncEventSource events_;
  esp_http_client_config_t http_client_config_;
  esp_http_client_handle_t http_client_;
  
  TaskHandle_t task_handle_http_;
  TaskHandle_t task_handle_ota_;

  AlarmControl *alarm_control_;

  uint8_t wifi_select_idx_;
};
