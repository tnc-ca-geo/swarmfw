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

#ifndef display.h
#include "display.h"
#endif
#include <Arduino.h>

class SwarmNode {

  private:
    SwarmDisplay *_displayRef;
    unsigned long messageCounter;

  public:
    SwarmNode(SwarmDisplay *displayObject);
    void begin();
    static size_t cleanCommand(const char *command, size_t len, char *bfr);
    static size_t getLine(char *bfr);
    int getTime(char *bfr);
    // from https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf
    // page 34
    static uint8_t nmeaChecksum(const char *sz, size_t len);
    static boolean parseLine(
      const char *line, size_t len, const char *searchTerm, size_t searchLen);
    static int parseTime(const char *timeResponse, size_t len);
    void sendMessage(const char *message, size_t len);
    virtual size_t tileCommand(const char *command, size_t len, char *bfr);
};
