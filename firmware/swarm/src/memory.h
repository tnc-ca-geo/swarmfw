#ifndef _NODE_PERSISTENT_MEMORY_
#define _NODE_PERSISTENT_MEMORY_
#endif

#include <EEPROM.h>

#define EEPROM_SIZE 16
#define FREQUENCY_ADDRESS 0


/*
 * Configuration storage
 * - this is rather static and project-specific but good enough for now
 */
class PersistentMemory {
  public:
    static void writeFrequency(uint32_t value) {
      // for some reason it does not work if we do that
      // in the constructor
      EEPROM.begin(EEPROM_SIZE);
      for (size_t i=0; i<sizeof(value); i++) {
        byte bte = (value >> 8 * i) & 0xFF;
        EEPROM.write(FREQUENCY_ADDRESS+i, bte);
      }
      EEPROM.commit();
    };

    /*
     *  Get measurement Frequency or use default
     *  - not necessarily the best place but we are re-using it
     *  - however it gives us the chance to generalize
     */
    static uint32_t getMeasurementFrequency(unsigned long dflt) {
      uint32_t ret = readFrequency();
      if (ret < 60 && ret > 86400) ret = dflt;
      return ret;
    };

    static uint32_t readFrequency() {
      uint32_t ret = 0;
      EEPROM.begin(EEPROM_SIZE);
      for (size_t i=0; i < sizeof(ret); i++) {
        byte bte = EEPROM.read(FREQUENCY_ADDRESS+i);
        ret = ret + bte * pow(256, i);
      }
      return ret;
    }
};
