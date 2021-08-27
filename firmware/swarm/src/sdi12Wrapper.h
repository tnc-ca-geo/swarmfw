/* 
 *  Wrapping the SDI12 Arduino library
 *  
 *  The use of the Arduino String class is considered problematic
 *  I am using it here for convenience information on startup but
 *  I have eliminated from any code that runs long term.
 */

#include <Arduino.h>

class SDI12Measurement {
  private:
    void readSDI12Buffer(uint8_t *bfr);
    void sendSDI12(uint8_t *cmd, uint8_t *bfr);
  public:
    SDI12Measurement();
    void getName(uint8_t *bfr);
    void getInfo(uint8_t *bfr);
    uint8_t getPayload(uint8_t addr, uint8_t *bfr);
};
