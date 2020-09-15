#include "raingauge.h"
#include <Arduino.h>

using namespace raingauge;

uint8_t RainGauge::rainCnt_5min = 0;
unsigned long RainGauge::last_interrupt_time = 0;

void RainGauge::rain_signal() {
  cli();
  unsigned long interrupt_time = micros();  // If interrupts come faster than 50ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 50) 
  {
    rainCnt_5min++;
    last_interrupt_time = interrupt_time;
  }
  sei();
}

float RainGauge::get1mmRainAmount() {
  return mm_rain_per_pulse * rainCnt_5min;
}

void RainGauge::resetRainCnt() {
  rainCnt_5min = 0;
}
