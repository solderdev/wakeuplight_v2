#include "WebInterface.hpp"
#include "TaskConfig.hpp"
#include <SPIFFS.h>
#include <FS.h>  // needed to fix asyncwebserver compile issues
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <cstring>
#include <string>
#include "Pins.hpp"
#include <sstream>
#include "sunset.h"

#include "Private.hpp"

// Docu:
// https://registry.platformio.org/libraries/me-no-dev/ESP%20Async%20WebServer
// https://realfavicongenerator.net

// picture send as data-URI: https://www.thetawelle.de/?p=6362&cpage=1

// from https://techtutorialsx.com/2017/12/16/esp32-arduino-async-http-server-serving-a-html-page-from-flash-memory/

static const char HOSTNAME[] = "wakeuplight";

static WebInterface *instance = nullptr;

WebInterface::WebInterface(AlarmControl *alarm_control) :
  server_(80),
  events_("/events"),
  task_handle_http_(nullptr),
  task_handle_ota_(nullptr),
  alarm_control_(alarm_control),
  wifi_select_idx_(0)
{
  if (instance)
  {
    log_e("ERROR: more than one WebInterface generated");
    ESP.restart();
    return;
  }

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    log_e("An Error has occurred while mounting SPIFFS");
    return;
  }

  // disable Bluetooth
  btStop();

  if (xTaskCreate(&WebInterface::task_http_wrapper, "task_http", TaskConfig::WiFi_http_stacksize, this, TaskConfig::WiFi_http_priority, &task_handle_http_) != pdPASS)
    log_e("WebInterface ERROR init failed, task_http");

  instance = this;
}

AlarmControl* WebInterface::getAlarmControl()
{
  return this->alarm_control_;
}

void WebInterface::wifiReconnect()
{
  WiFi.disconnect();
  vTaskDelay(pdMS_TO_TICKS(1000));
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  vTaskDelay(pdMS_TO_TICKS(1000));

  log_d("trying wifi %u with: %s / %s", this->wifi_select_idx_, WIFICREDS[this->wifi_select_idx_].ssid, WIFICREDS[this->wifi_select_idx_].pw);
  WiFi.begin(WIFICREDS[this->wifi_select_idx_].ssid, WIFICREDS[this->wifi_select_idx_].pw);
  this->wifi_select_idx_ += 1;
  this->wifi_select_idx_ %= sizeof(WIFICREDS) / sizeof(WIFICREDS[0]);
  
  WiFi.setSleep(false);  // "true" makes mDNS and request extremely slow because packets are lost
  WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);  // WIFI_POWER_19_5dBm
}

void WebInterface::wifiCheckConnectionOrReconnect()
{
  if (WiFi.isConnected())
    return;
  
  log_w("WIFI down .. attempting connect!");
  wifiReconnect();
  
  log_i("WIFI reconnecting .. waiting for network");
  uint32_t trycount = 0;
  while (!WiFi.isConnected())
  {
    trycount++;
    if (trycount > 100)  // 10s
    {
      trycount = 0;
      log_d("WIFI still down .. attempting reconnect!");
      wifiReconnect();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  log_i("WIFI up again!");
}

String WebInterface::build_parameter_string(void)
{
  String al_we = (this->alarm_control_->getAlarmWeekend()) ? String("true") : String("false");

  String params = 
    this->alarm_control_->getAlarmTime() + " " +
    al_we + " " +
    String(this->alarm_control_->getFadeMinutes()) + " " + 
    String(this->alarm_control_->getMode()) + " " + 
    String(this->alarm_control_->getDutyMax(), 3) + " " +
    String(this->alarm_control_->getDutyLightsOn(), 3) + " " +
    String(this->alarm_control_->getCurrentDuty(), 3) + " " +
    String(this->alarm_control_->getSnoozeMinutes());

  return params;
}

void WebInterface::task_http_wrapper(void *arg)
{
  WebInterface* web_interface = static_cast<WebInterface *>(arg);
  web_interface->task_http();
}
void WebInterface::task_http()
{
  wifiCheckConnectionOrReconnect();

  IPAddress ip = WiFi.localIP();
  log_i("WiFi connected - IP address: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

  // be reachable under hostname.local
  if (!MDNS.begin(HOSTNAME))
    log_e("Error setting up MDNS responder!");
  else
    log_i("mDNS responder started");

  // serve static files: images, html, js, css
  server_.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  // routes to buttons
  server_.on("/lights_on", HTTP_POST, [this](AsyncWebServerRequest *request) {
    log_d("Received lights_on");
    this->alarm_control_->setOnMode();
    request->send(200, "text/plain", this->build_parameter_string());
  });

  server_.on("/alarm_off", HTTP_POST, [this](AsyncWebServerRequest *request) {
    log_d("Received alarm_off");
    this->alarm_control_->setAlarmOFF();
    request->send(200, "text/plain", this->build_parameter_string());
  });

  server_.on("/alarm_on", HTTP_POST, [this](AsyncWebServerRequest *request) {
    log_d("Received alarm_on");
    this->alarm_control_->setAlarmON();
    request->send(200, "text/plain", this->build_parameter_string());
  });

  // routes to setters
  server_.on("/set_alarm_time", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
    [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      String alarm_time = String(data, len);
      this->alarm_control_->setAlarmTime(alarm_time);
      request->send(200, "text/plain", this->build_parameter_string());
  });

  server_.on("/set_alarm_weekend", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
    [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      bool alarm_weekend = strncmp((const char*)(data), "true", len) == 0;
      this->alarm_control_->setAlarmWeekend(alarm_weekend);
      request->send(200, "text/plain", this->build_parameter_string());
  });

  server_.on("/set_fade_minutes", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
    [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      std::stringstream ss;
      ss << (char*)data;
      float fade_minutes;
      ss >> fade_minutes;
      this->alarm_control_->setFadeMinutes(fade_minutes);
      request->send(200, "text/plain", this->build_parameter_string());
  });

  server_.on("/set_snooze_minutes", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
    [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      std::stringstream ss;
      ss << (char*)data;
      float snooze_minutes;
      ss >> snooze_minutes;
      this->alarm_control_->setSnoozeMinutes(snooze_minutes);
      request->send(200, "text/plain", this->build_parameter_string());
  });

  server_.on("/set_duty_lights_on", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
    [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      std::stringstream ss;
      ss << (char*)data;
      float duty;
      ss >> duty;
      this->alarm_control_->setDutyLightsOn(duty);
      request->send(200, "text/plain", this->build_parameter_string());
  });

  // route to fetch current parameters
  server_.on("/parameters", HTTP_GET, [this](AsyncWebServerRequest *request) {
    log_d("Received parameters req");
    String params = this->build_parameter_string();
    request->send(200, "text/plain", params);
    log_d("send parameters: %s", params);
  });

  // route to restart ESP
  server_.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200);
    ESP.restart();
  });

  // route to get heap usage
  server_.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Free heap: " + String(ESP.getFreeHeap()));
  });
  
  // add handler for Server-Sent Events
  server_.addHandler(&events_);
  // start webserver
  server_.begin();
  MDNS.addService("http", "tcp", 80);

  // start OTA task when WIFI is available
  if (task_handle_ota_ == nullptr)
    xTaskCreate(task_ota_wrapper, "task_ota", TaskConfig::WiFi_ota_stacksize, this, TaskConfig::WiFi_ota_priority, &task_handle_ota_);
  
  while (1)
  {
    wifiCheckConnectionOrReconnect();

    // live stream current percent every second
    events_.send(String(this->alarm_control_->getCurrentDuty(), 3).c_str(), "currentpercent", millis());

    // send current time and time of last NTP sync
    struct tm timeinfo = this->alarm_control_->getCurrentTime();
    struct timeval lastntpsync = this->alarm_control_->getLastNTPSync();
    struct tm ntpsync;
    localtime_r(&lastntpsync.tv_sec, &ntpsync);

    // do not indent! -> will lead to spaces being sent in string
    char currenttime[200] = {0};
    snprintf(currenttime, sizeof(currenttime), 
"<b>%02u:%02u:%02u</b>\
<p>last NTP sync:<br />\
%u.%u.%u %02u:%02u:%02u<br />\
sunrise: %02u:%02u<br />\
sunset: %02u:%02u\
</p>", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
             ntpsync.tm_mday, 1 + ntpsync.tm_mon, 1900 + ntpsync.tm_year, ntpsync.tm_hour, ntpsync.tm_min, ntpsync.tm_sec,
             this->alarm_control_->sunrise_minutes / 60, this->alarm_control_->sunrise_minutes % 60,
             this->alarm_control_->sunset_minutes / 60, this->alarm_control_->sunset_minutes % 60
            );
    events_.send(currenttime, "currenttime", millis());

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}


void WebInterface::task_ota_wrapper(void *arg)
{
  static_cast<WebInterface *>(arg)->task_ota();
}
void WebInterface::task_ota()
{
  // over-the-air update:
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // stop everything (esp. webserver)
      instance->server_.end();
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      SPIFFS.end();

      log_i("Start updating %s", type);
    })
    .onEnd([]() {
      log_i("\nEnd of OTA ..");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      log_d("Progress: %u%%", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      log_e("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) log_e("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) log_e("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) log_e("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) log_e("Receive Failed");
      else if (error == OTA_END_ERROR) log_e("End Failed");
    });

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();
  
  while(1)
  {
    vTaskDelay(pdMS_TO_TICKS(5));
    
    if (WiFi.isConnected())
      ArduinoOTA.handle();
    else
      vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
