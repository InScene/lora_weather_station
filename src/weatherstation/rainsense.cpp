#include "rainsense.h"
#include <Arduino.h>

using namespace rainsense;

RainSense::RainSense() : 
  _cloudburst(256),
  _heavyRain(400),
  _lightRain(668),
  _adcValue(0){
  
}

void RainSense::init() {
  pinMode(rainsense_Adc_Pin,   INPUT);
  pinMode(rainsense_VCC_Pin, OUTPUT);
}

void RainSense::fetchData() {
  digitalWrite(rainsense_VCC_Pin, HIGH);
  delay(2000);
  uint32_t val = analogRead(rainsense_Adc_Pin);
  val += analogRead(rainsense_Adc_Pin);
  val += analogRead(rainsense_Adc_Pin);
  digitalWrite(rainsense_VCC_Pin, LOW);
  _adcValue = val/3;
}

uint16_t RainSense::getAdcValue() {
  return _adcValue;
}

uint8_t RainSense::getInterpreteValue() {
  #ifdef ACTIVATE_PRINT
    Serial.print(F("rain sense data: "));
    Serial.println(_adcValue);
  #endif
  
  if(_adcValue < _cloudburst) {
    #ifdef ACTIVATE_PRINT
      Serial.println("cloudburst");
    #endif
    return 3;
  }
  else
  if(_adcValue < _heavyRain) {
    #ifdef ACTIVATE_PRINT
      Serial.println("heavy rain");
    #endif
    return 2;
  }
  else
  if(_adcValue < _lightRain) {
    #ifdef ACTIVATE_PRINT
      Serial.println("light rain");
    #endif
    return 1;
  }
  else { // messwert in [768, 1024[
    #ifdef ACTIVATE_PRINT
      Serial.println("dry => no rain");
    #endif
    return 0;    
  }
}

void RainSense::set_cloudburst(uint16_t val) {
  _cloudburst = val;
}

void RainSense::set_heavyRain(uint16_t val) {
  _heavyRain = val;
}

void RainSense::set_lightRain(uint16_t val) {
  _lightRain = val;
}
