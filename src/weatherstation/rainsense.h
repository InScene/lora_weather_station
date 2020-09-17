#ifndef RAINSENSE_H
#define RAINSENSE_H

#include <stdint.h>

namespace rainsense {
  
#define rainsense_Adc_Pin A1
#define rainsense_VCC_Pin 8
#define ACTIVATE_PRINT 1

class RainSense {
  public:
    RainSense();
    void init();
    void fetchData();
    uint16_t getAdcValue();
    uint8_t getInterpreteValue();

    void set_cloudburst(uint16_t val);
    void set_heavyRain(uint16_t val);
    void set_lightRain(uint16_t val);

  private:
    uint16_t _cloudburst;
    uint16_t _heavyRain;
    uint16_t _lightRain;
    uint16_t _adcValue;
};
}


#endif
