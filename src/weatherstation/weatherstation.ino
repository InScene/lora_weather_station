/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with static payload, 
 * using frequency and encryption settings matching those of
 * the (early prototype version of) The Things Network.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in g1,
 *  0.1% in g2).
 *
 * Send following data via lpp:
 *  1: Temperature
 *  2: Relative humidity
 *  3: Barometric pressure
 *  4: Battery value (Volt)
 *  5: Interpretet rain sensor : 0=dry, 1=light rain, 2=heavy rain, 3=downpour
 *  6: digital rain value (2bytes)
 *  7: rain ammount counter last 5 minutes (mililiter)
 *  
 * ToDo:
 * - set NWKSKEY (value from staging.thethingsnetwork.com)
 * - set APPKSKEY (value from staging.thethingsnetwork.com)
 * - set DEVADDR (value from staging.thethingsnetwork.com)
 * - optionally comment #define ACTIVATE_PRINT
 * - set TX_INTERVAL in seconds
 * - change mydata to another (small) static text
 *
 *******************************************************************************/
#include <lmic.h>
#include <hal/hal.h>


// use low power sleep; comment next line to not use low power sleep
#include "LowPower.h"
bool next = false;

#define ACTIVATE_PRINT 1

#define battery_Adc_Pin A0

/****************** rain gauge sensor ***********************/
#include "raingauge.h"
raingauge::RainGauge RainGauge = raingauge::RainGauge();

/****************** Regensensor Sensor ***********************/
#define rainsense_Adc_Pin A1
#define rainsense_VCC_Pin 8

/****************** BME280 Sensor ***********************/
#include <Adafruit_BME280.h>
Adafruit_BME280 bme; // I2C

#define BME280_SDA_PIN A4
#define BME280_SCL_PIN A5
#define BME280_VCC_PIN 7

/****************** CayenneLPP ***********************/
#include <CayenneLPP.h>

/****************************************** LoRa *******************************/
#include "radio_keys.h"

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static osjob_t sendjob;

// Sende alle 3,5 Minuten eine Nachricht
const unsigned TX_INTERVAL = 190;

// Resette Device jeden Tag
const unsigned RESET_INTERVAL = 86400 / TX_INTERVAL;
unsigned RESET_CNT = 0;

// Resette Device, wenn Loop Cnt max erreicht. Passiert nur im Fehlerfall.
const unsigned long LOOP_MAX_CNT = 2500000; // Das sind ca. 5 Min
unsigned long LOOP_CNT = 0;

// Sicherstellen, dass die WDT Sleep Schleife im Fehlerfall unterbrochen wird
const unsigned long WDT_LOOP_MAX_CNT = 1000;
unsigned long WDT_LOOP_CNT = 0;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {A3, 3, 4},
};


void onEvent (ev_t ev) {
    switch(ev) {
        case EV_JOINING:
            // Do noting, because we react on EV_JOINED
        case EV_JOINED:
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            break;
        case EV_TXCOMPLETE:
            // Schedule next transmission
            next = true;            
            break;
        case EV_RXCOMPLETE:
            // Do nothing
            break;
        default:
            // If this happends, no connection possible. 
            #ifdef ACTIVATE_PRINT
              Serial.print(F("Not Handled event. OnEvent: "));
              Serial.println(ev);
            #endif
            break;
    }    
}


void do_send(osjob_t* j){
    #ifdef ACTIVATE_PRINT
      Serial.println(F("Enter do_send"));
    #endif
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
      #ifdef ACTIVATE_PRINT
        Serial.println(F("OP_TXRXPEND, not sending"));
      #endif      
    } else {
      /* Initialize CayenneLPP with max buffer size */
      CayenneLPP lpp(30);
      float temp, hum, press;

      #ifdef ACTIVATE_PRINT
        Serial.println(F("Fetch data"));
      #endif
      /**** Lese BME280 Data ****/
      digitalWrite(BME280_VCC_PIN, HIGH);
      delay(1000);
      
      if(readBme280Values(temp, hum, press)) {
        lpp.addTemperature(1, temp);
        lpp.addRelativeHumidity(2, hum);
        lpp.addBarometricPressure(3, press);
      }

      digitalWrite(BME280_VCC_PIN, LOW);

      /**** Lese Batterie Spannung *****/
      lpp.addAnalogInput(4, getBattAdcValue() * 0.004333333 );

      /**** Lese Regensensor Wert *****/
      uint16_t rainSenseVal = getRainSenseAdcValue();
      
      lpp.addDigitalInput(5, interpreteRainSenseAdcValue(rainSenseVal)); 
      lpp.addAnalogInput(6, rainSenseVal); 

      /**** Lese Regenmenge *****/
      lpp.addAnalogInput(7, RainGauge.get1mmRainAmount());
      RainGauge.resetRainCnt();
      
      /**** Sende Daten *****/
      LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);

      #ifdef ACTIVATE_PRINT
        Serial.println(F("Packet queued"));
      #endif
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

bool readBme280Values(float &temp, float &hum, float &press){
  float val;
  uint8_t counterTemp = 0;
  uint8_t counterPress = 0;
  uint8_t counterHum = 0;
  temp = 0;
  hum = 0;
  press = 0;
  
  if(bme.begin(0x76)) {
    for(int x=0; x<3; x++) {
      val = bme.readTemperature();
      if(val != NAN) {
        temp += val;
        counterTemp++;
      }

      val = bme.readPressure();
      if(val != NAN) {
        press += val / 100.0F;
        counterPress++;
      }

      val = bme.readHumidity();
      if(val != NAN) {
        hum += val;
        counterHum++;
      }
    }

    if((counterTemp==0)||(counterPress==0)||(counterHum==0))
      return false;
      
    temp = temp / counterTemp;
    press = press / counterPress;
    hum = hum / counterHum;

    return true;
  }
    
  return false;
}

uint16_t getBattAdcValue() {
  uint32_t val = analogRead(battery_Adc_Pin);
  val += analogRead(battery_Adc_Pin);
  val += analogRead(battery_Adc_Pin);

  return val/3;
}

uint16_t getRainSenseAdcValue() {
  
  digitalWrite(rainsense_VCC_Pin, HIGH);
  delay(2000);
  uint32_t val = analogRead(rainsense_Adc_Pin);
  val += analogRead(rainsense_Adc_Pin);
  val += analogRead(rainsense_Adc_Pin);
  digitalWrite(rainsense_VCC_Pin, LOW);
  return val/3;
}

uint8_t interpreteRainSenseAdcValue(uint16_t val) {
  #ifdef ACTIVATE_PRINT
    Serial.println(F("Regensensor daten: "));
    Serial.println(val);
  #endif
  
  if(val < 256) {
    #ifdef ACTIVATE_PRINT
      Serial.println("Wolkenbruch");
    #endif
    return 3;
  }
  else
  if(val < 400) {
    #ifdef ACTIVATE_PRINT
      Serial.println("Starkregen");
    #endif
    return 2;
  }
  else
  if(val < 668) {
    #ifdef ACTIVATE_PRINT
      Serial.println("Leichtregen");
    #endif
    return 1;
  }
  else { // messwert in [768, 1024[
    #ifdef ACTIVATE_PRINT
      Serial.println("Trocken => Kein Regen");
    #endif
    return 0;    
  }
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void setup() {
    #ifdef ACTIVATE_PRINT
      Serial.begin(9600);
      Serial.println(F("Enter setup"));
    #endif

    /* Regenmengen Sensor */
    // Lege den Interruptpin als Inputpin mit Pullupwiderstand fest
    pinMode(raingouge_int_Pin, INPUT_PULLUP);
    // Lege die ISR 'blink' auf den Interruptpin mit Modus 'CHANGE':
    // "Bei wechselnder Flanke auf dem Interruptpin" --> "Führe die ISR aus"
    attachInterrupt(digitalPinToInterrupt(raingouge_int_Pin), RainGauge.rain_signal, CHANGE);

    /* Regen Sensor */
    pinMode(rainsense_Adc_Pin,   INPUT);
    pinMode(rainsense_VCC_Pin, OUTPUT);
    pinMode(BME280_VCC_PIN, OUTPUT);

    /* Batterie */
    // Fuer A0 Battery
    analogReference(INTERNAL);
    delay(1000);
    // Lese Batterie Daten drei mal ein, um ADC einpendeln zu lassen
    analogRead(battery_Adc_Pin);
    analogRead(battery_Adc_Pin);
    analogRead(battery_Adc_Pin);

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // Aktiviere Interrupts
    interrupts();
    
    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // got this fix from forum: https://www.thethingsnetwork.org/forum/t/over-the-air-activation-otaa-with-lmic/1921/36
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

    // Start job
    do_send(&sendjob);
    
    #ifdef ACTIVATE_PRINT
      Serial.println(F("Leave setup"));
    #endif
}

void resetWhenCounterMaxReached() {
  RESET_CNT++;
  if (RESET_CNT >= RESET_INTERVAL) {
    RESET_CNT = 0;
    #ifdef ACTIVATE_PRINT
      Serial.println(F("Reset device"));
      Serial.flush(); // give the serial print chance to complete
    #endif
    resetFunc();  //call reset
  } else {
    #ifdef ACTIVATE_PRINT
      Serial.print(F("Reset counter: "));
      Serial.print(RESET_CNT);
      Serial.print(F(", from:"));
      Serial.println(RESET_INTERVAL);
      Serial.flush(); // give the serial print chance to complete
    #endif
  }
}

void resetWhenLoopCounterMaxReached() {
  LOOP_CNT++;
  if(LOOP_CNT % LOOP_MAX_CNT == 0 && LOOP_CNT != 0) {
    #ifdef ACTIVATE_PRINT
      Serial.println(F("Reset device because of loop overrun"));
      Serial.flush(); // give the serial print chance to complete
    #endif
    LOOP_CNT = 0;
    resetFunc();  //call reset
  }
}

bool failsaveWdtEndlessLoopInterrupter(){
    WDT_LOOP_CNT++;
    if(WDT_LOOP_CNT >= WDT_LOOP_MAX_CNT) {
      #ifdef ACTIVATE_PRINT
        Serial.print(F("WDT error. Loop counter max reached!"));
        Serial.println(WDT_LOOP_CNT);
        Serial.flush(); // give the serial print chance to complete
      #endif
      
      return true;
    }
    return false;
}

void sleepForATime() {
  const int sleepcycles = TX_INTERVAL / 8;  // calculate the number of sleepcycles (8s) given the TX_INTERVAL
  #ifdef ACTIVATE_PRINT
    Serial.print(F("Enter sleeping for "));
    Serial.print(sleepcycles);
    Serial.println(F(" cycles of 8 seconds"));
    Serial.flush(); // give the serial print chance to complete
  #endif
  
  for (int i=0; i<sleepcycles; i++) {
    WDT_LOOP_CNT = 0;
    // Enter power down state for 8 s with ADC and BOD module disabled
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    // If watchdog not triggered, go back to sleep. End sleep by other interrupt 
    while(!LowPower.isWdtTriggered() || failsaveWdtEndlessLoopInterrupter())
    {
      LowPower.returnToSleep();
    }
  }
   
  #ifdef ACTIVATE_PRINT
    Serial.println("******************* Sleep complete *******************");
  #endif
}

void loop() {
  extern volatile unsigned long timer0_overflow_count;

  if (next == false) {
    os_runloop_once();
    resetWhenLoopCounterMaxReached();

  } else {
    LOOP_CNT = 0;
    resetWhenCounterMaxReached();

    sleepForATime();
    next = false;
  
    // Start job
    do_send(&sendjob);
  }
}
