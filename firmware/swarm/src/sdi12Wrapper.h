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
     // state variables
     char lastSensor;
     uint16_t numberOfValues;
     uint16_t valuesReceived;
     uint16_t waitTime;
     unsigned long startWaitTime;
     SDI12Measurement();
     void debug();
     // count values in a response string obtained with the aD! command
     uint16_t countValues(char *bfr, const size_t len);
     // get name of the measurement method, returns just an identifying
     // message to identify which measurement class is used
     size_t getName(char *bfr);
     // get info about a sensor at addr; ? means all of them
     size_t getInfo(char *bfr, const char addr='?');
     // return available channels/address
     size_t getChannels(char *bfr, const char maxChannel='9');
     // read measurements from a channel/address
     size_t getPayload(char *bfr, const char addr=0);
     // parse response for time
     void parseResponse(char *response, size_t len);
     // update channel, return success 1 or failure 0
     boolean setChannel(char oldAddr, char newAddr);
 };
