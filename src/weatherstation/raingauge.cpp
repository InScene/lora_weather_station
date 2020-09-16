#include "raingauge.h"
#include <Arduino.h>

using namespace raingauge;

uint8_t RainGauge::rainPulseCnt = 0;
unsigned long RainGauge::last_interrupt_time = 0;

RainGauge::RainGauge(){
  rainPulseCnt = 0;
  last_interrupt_time = 0;
  lastReadedCntValue = 0;
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

void RainGauge::init() {
  // Lege den Interruptpin als Inputpin mit Pullupwiderstand fest
  pinMode(raingouge_int_Pin, INPUT_PULLUP);
  // Lege die ISR 'blink' auf den Interruptpin mit Modus 'CHANGE':
  // "Bei wechselnder Flanke auf dem Interruptpin" --> "Führe die ISR aus"
  attachInterrupt(digitalPinToInterrupt(raingouge_int_Pin), rain_signal, CHANGE);
}

float RainGauge::get1mmRainAmount() {
  lastReadedCntValue = rainPulseCnt;
  return mm_rain_per_pulse * lastReadedCntValue;
}

void RainGauge::removeReadedRainCnt() {
  if(rainPulseCnt > 0 && 
     lastReadedCntValue > 0 && 
     lastReadedCntValue <= rainPulseCnt) {
      
    rainPulseCnt -= lastReadedCntValue;
  }

  lastReadedCntValue = 0;
}

void RainGauge::printCnt() {
  Serial.print(F("Rain gauge counter: "));
  Serial.println(rainPulseCnt);
}
