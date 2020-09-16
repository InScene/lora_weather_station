#include "rainsense.h"
#include <Arduino.h>

using namespace rainsense;

RainSense::RainSense() : adcValue(0) {
  
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
  adcValue = val/3;
}

uint16_t RainSense::getAdcValue() {
  return adcValue;
}

uint8_t RainSense::getInterpreteValue() {
  #ifdef ACTIVATE_PRINT
    Serial.print(F("rain sense data: "));
    Serial.println(adcValue);
  #endif
  
  if(adcValue < 256) {
    #ifdef ACTIVATE_PRINT
      Serial.println("cloudburst");
    #endif
    return 3;
  }
  else
  if(adcValue < 400) {
    #ifdef ACTIVATE_PRINT
      Serial.println("heavy rain");
    #endif
    return 2;
  }
  else
  if(adcValue < 668) {
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
