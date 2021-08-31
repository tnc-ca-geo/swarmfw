/*
 *  Wrapping the SDI12 Arduino library
 *
 */
 #ifndef _SDI12_WRAPPER_H_
 #define _SDI12_WRAPPER_H_
 #endif

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
     size_t readSDI12Buffer(char *bfr);
     size_t sendSDI12(char *cmd, char *bfr);
   public:
     SDI12Measurement();
     void debug();
     size_t getName(char *bfr);
     size_t getInfo(char *bfr);
     size_t getPayload(char addr, char *bfr);
 };
