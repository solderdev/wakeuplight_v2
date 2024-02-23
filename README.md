# wakeuplight_v2
ESP32 LED-strip wakeuplight with audio stream, NTP, etc.

Second version of my wakeuplight.

Uses an ESP32, N-channel MOSFETs with gate-drivers PWM-controlling multiple warm-white 24V-LED-strips.

Additionally an internet radio may wake you up.

Time is synchronized via NTP.

## Features

* PWA webinterface for easy control and live-feedback
* manually activate light and regulate intensity
* set fade start time and duration
* over-complicated fade algorithm for added smoothness
* store multiple WiFi credentials and connect / reconnect
* NTP time sync
* sunrise / sunset calculation and default intesity regulation
* store settings in NVM
* logging to serial out


## Build / run

* Build using PlatformIO
* F1 -> Tasks: Run task -> PlatformIO: Build filesystem image (esp32dev)
* F1 -> Tasks: Run task -> PlatformIO: Upload filesystem image (esp32dev)
* run program:
* PlatformIO: Build
* PlatformIO: Upload and Monitor
