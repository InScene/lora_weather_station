#include "raingauge.h"
#include <Arduino.h>

using namespace raingauge;

uint8_t RainGauge::rainPulseCnt = 0;
unsigned long RainGauge::last_interrupt_time = 0;

RainGauge::RainGauge(){
  rainPulseCnt = 0;
  last_interrupt_time = 0;
}
  
void RainGauge::rain_signal() {
  cli();
  unsigned long interrupt_time = micros();  // If interrupts come faster than 50ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 50) 
  {
    rainPulseCnt++;
    last_interrupt_time = interrupt_time;
  }
  sei();
}

float RainGauge::get1mmRainAmount() {
  return mm_rain_per_pulse * rainPulseCnt;
}

void RainGauge::resetRainCnt() {
  rainPulseCnt = 0;
}

void RainGauge::printCnt() {
  Serial.print(F("Rain gauge counter: "));
  Serial.println(rainPulseCnt);
}
