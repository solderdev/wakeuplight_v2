#pragma once

#include <Arduino.h>
#include "Audio.h"

class InternetRadio
{
public:
  InternetRadio();
  InternetRadio(InternetRadio const&) = delete;
  void operator=(InternetRadio const&) = delete;

  void loop(void);

private:
  Audio audio_;
};
