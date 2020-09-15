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
    void fetchData();
    uint16_t getAdcValue();
    uint8_t getInterpreteValue();

   private:
    uint16_t adcValue;
};
}


#endif
