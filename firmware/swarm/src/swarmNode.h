/*
 *  Header file for SWARM tile interaction
 *
 *  Naming and type use conventions:
 *
 *  - Use *bfr for output
 *  - Make input buffers constants to not manipulate them within functions
 *  - Name input buffers according to their expected content
 *  - Use size_t len for object length throughout
 *  - Use other numerical type of return is used to be as a number
 *  - Make helper methods static if they don't relay on instantiation
 */
#include <Arduino.h>
#ifndef _DISPLAY_WRAPPER_H_
#include "displayWrapper.h"
#endif
#ifndef _SERIAL_WRAPPER_H_
#include "serialWrapper.h"
#endif

class SwarmNode {

  private:
    DisplayWrapperBase *_wrappedDisplayRef;
    SerialWrapperBase *_wrappedSerialRef;
    boolean dev;
    // unsigned long messageCounter;

  public:
    SwarmNode(
      DisplayWrapperBase *wrappedDisplayObject,
      SerialWrapperBase *wrappedSerialObject, const boolean dev=true);
    void begin(const int timeReportingFrequency=600);
    // TODO: remove
    size_t cleanCommand(const char *command, const size_t len, char *bfr);
    void emptySerialBuffer();
    size_t formatMessage(const char *message, const size_t len, char *bfr);
    size_t getLine(char *bfr);
    int getTime(char *bfr);
    unsigned long int waitForTimeStamp();
    unsigned long int getTimeStamp();
    // from https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf
    // page 34
    uint8_t nmeaChecksum(const char *sz, const size_t len);
    boolean parseLine(
      const char *line, const size_t len, const char *searchTerm,
      const size_t searchLen);
    unsigned long int parseTime(const char *timeResponse, const size_t len);
    void sendMessage(const char *message, const size_t len);
    size_t tileCommand(const char *command, const size_t len, char *bfr);
};
