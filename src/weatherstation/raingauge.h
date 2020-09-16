#ifndef RAINGAUGE_H
#define RAINGAUGE_H

#define raingouge_int_Pin 2

#include <stdint.h>

namespace raingauge {
class RainGauge {

  public:

    RainGauge();
    static void rain_signal();
    void init();
    float get1mmRainAmount();
    void resetRainCnt();
    void printCnt();

  private:
  
    static uint8_t rainPulseCnt;
    static unsigned long last_interrupt_time;

    /* Basis for the calculation of the rain amount in mm
        5cm x 11cm, and 6 pulses per 10mL => 10/6 = 1.67mL per pulse.
    
        1mm rain = 0.1*5*11cm = 5.5 mL
        1" rain = 2.54*5*11cm = 139.70 mL
    
        @ 6 pulses per 10mL => 10/6 = 1.67mL per pulse
        
        pulses / mm = 5.5 / (10/6) = 3.3
        pulses / in = 139.7/(10/6) = 83.82
        
        mm / pulse = 0.30303
        in / pulse = 0.01193
    */
    const float mm_rain_per_pulse = 0.30303; // 0.30303mm rain per pulse
};
}
#endif
