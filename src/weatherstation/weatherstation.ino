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
#include <CayenneLPP.h>
#include "rainsense.h"
#include "bme280sensor.h"
#include "battery.h"
#include "raingauge.h"
#include "failsafe.h"

// Sende alle 3,5 Minuten eine Nachricht
const unsigned TX_INTERVAL = 190;

raingauge::RainGauge g_rainGauge;
rainsense::RainSense g_rainSense;
bme280_sensor::BME280Sensor g_bmeSensor;
battery::Battery g_battery;
failsafe::FailSafe g_resetDaily(86400 / TX_INTERVAL);
failsafe::FailSafe g_wdtFailSafe(1000);
failsafe::FailSafe g_loopFailSafe(2500000);

bool next = false;
#define ACTIVATE_PRINT 1

/****************************************** LoRa *******************************/
#include "radio_keys.h"

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static osjob_t sendjob;

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
            break;
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
      #ifdef ACTIVATE_PRINT
        Serial.println(F("Fetch data"));
      #endif
      CayenneLPP lpp = getCayenneFormatedData();
      
      /**** send data *****/
      LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);

      #ifdef ACTIVATE_PRINT
        Serial.println(F("Packet queued"));
      #endif
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

CayenneLPP getCayenneFormatedData() {
  /* Initialize CayenneLPP with max buffer size */
  CayenneLPP lpp(30);

  /**** get BME280 Data ****/      
  if(g_bmeSensor.fetchData()) {
    lpp.addTemperature(1, g_bmeSensor.getTemperature());
    lpp.addRelativeHumidity(2, g_bmeSensor.getHumidity());
    lpp.addBarometricPressure(3, g_bmeSensor.getPressure());
    #ifdef ACTIVATE_PRINT
      g_bmeSensor.print();
    #endif
    
  } else {
    #ifdef ACTIVATE_PRINT
      Serial.println(F("Error getting bme280 data!"));
    #endif
  }

  /**** get battery voltage *****/
  g_battery.fetchData();
  lpp.addAnalogInput(4, g_battery.getVoltage() );
  #ifdef ACTIVATE_PRINT
    g_battery.print();
  #endif
  
  /**** get rain sense value *****/
  g_rainSense.fetchData();
  
  lpp.addDigitalInput(5, g_rainSense.getInterpreteValue()); 
  lpp.addAnalogInput(6, g_rainSense.getAdcValue()); 

  /**** get rain amount *****/
  lpp.addAnalogInput(7, g_rainGauge.get1mmRainAmount());
  g_rainGauge.removeReadedRainCnt();
  #ifdef ACTIVATE_PRINT
    g_rainGauge.printCnt();
  #endif

  return lpp;
}


void setup() {
    #ifdef ACTIVATE_PRINT
      Serial.begin(9600);
      Serial.println(F("Enter setup"));
    #endif

    g_rainGauge.init();
    g_rainSense.init();
    g_bmeSensor.init();
    g_battery.init();    

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



void sleepForATime() {
  const int sleepcycles = TX_INTERVAL / 8;  // calculate the number of sleepcycles (8s) given the TX_INTERVAL
  #ifdef ACTIVATE_PRINT
    Serial.print(F("Enter sleeping for "));
    Serial.print(sleepcycles);
    Serial.println(F(" cycles of 8 seconds"));
    Serial.flush(); // give the serial print chance to complete
  #endif
  
  for (int i=0; i<sleepcycles; i++) {
    g_wdtFailSafe.resetCnt();
    // Enter power down state for 8 s with ADC and BOD module disabled
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    // If watchdog not triggered, go back to sleep. End sleep by other interrupt 
     while(!LowPower.isWdtTriggered())
    {
      g_wdtFailSafe.increaseCnt();
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

  } else {
    g_resetDaily.increaseCnt();
    g_loopFailSafe.resetCnt();

    sleepForATime();
    next = false;
  
    // Start job
    do_send(&sendjob);
  }

  g_loopFailSafe.increaseCnt();
}
