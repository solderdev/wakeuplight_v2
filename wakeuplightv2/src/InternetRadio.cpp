#include "InternetRadio.hpp"
#include "Pins.hpp"


InternetRadio::InternetRadio()
{
  log_d("init audio driver");
  audio_.setPinout(Pins::i2s_bclk, Pins::i2s_lrc, Pins::i2s_dout);
  audio_.setVolume(21); // default 0...21
}

void InternetRadio::loop(void)
{
  audio_.loop();
}
