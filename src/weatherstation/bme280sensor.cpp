#include "bme280sensor.h"
#include <Arduino.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme; // I2C

using namespace bme280_sensor;

BME280Sensor::BME280Sensor() :
  temperature(0),
  humidity(0),
  pressure(0){
  
}

void BME280Sensor::init() {
  pinMode(BME280_VCC_PIN, OUTPUT);
}

bool BME280Sensor::fetchData(){
  bool success = false;
  temperature = 0;
  humidity = 0;
  pressure = 0;
  
  digitalWrite(BME280_VCC_PIN, HIGH);
  delay(1000);

  if(startReading()) {
    if(readTemperature() &&
       readHumidity() &&
       readPressure()) {
      success = true;
      
    }
  }

  digitalWrite(BME280_VCC_PIN, LOW); 
  return success;
}

float BME280Sensor::getTemperature() {
  return temperature;
}

float BME280Sensor::getHumidity() {
  return humidity;
}

float BME280Sensor::getPressure() {
  return pressure;
}
 
bool BME280Sensor::startReading() {
  if(bme.begin(0x76)) {
    return true;
  }

  return false;
}

void BME280Sensor::print() {
  Serial.print(F("BME280 temp: "));
  Serial.print(temperature);
  Serial.print(F(", hum: "));
  Serial.print(humidity);
  Serial.print(F(", press: "));
  Serial.println(pressure);
}

bool BME280Sensor::readTemperature() {
  uint8_t counter = 0;
  float val;
  float data = 0;
  
  for(int x=0; x<3; x++) {
    val = bme.readTemperature();
    if(val != NAN) {
      data += val;
      counter++;
    }
  }

  if(counter != 0) {
    temperature = data / counter;
    return true;
  }

  return false;
}

bool BME280Sensor::readHumidity() {
  uint8_t counter = 0;
  float val;
  float data = 0;
  
  for(int x=0; x<3; x++) {
    val = bme.readHumidity();
    if(val != NAN) {
      data += val;
      counter++;
    }
  }

  if(counter != 0) {
    humidity = data / counter;
    return true;
  }

  return false;
}

bool BME280Sensor::readPressure() {
  uint8_t counter = 0;
  float val;
  float data = 0;
  
  for(int x=0; x<3; x++) {
    val = bme.readPressure();
    if(val != NAN) {
      data += val;
      counter++;
    }
  }

  if(counter != 0) {
    pressure = data / counter;
    return true;
  }

  return false;
}
