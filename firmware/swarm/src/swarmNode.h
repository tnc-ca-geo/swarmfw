#ifndef display.h
#include "display.h"
#endif
#include <Arduino.h>


class SwarmNode {

  private:
    SwarmDisplay *_displayRef;
    unsigned long messageCounter;
    static int cleanCommand(char *command, int len, char *bffr);

  public:
    SwarmNode();
    void begin();
    int getLine(char *bfr);
    int getTime(char *bfr);
    // from https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf
    // page 34
    static uint8_t nmeaChecksum(const char *sz, size_t len);
    static boolean parseLine(
      const char *bffr, const int len,
      const char *searchTerm, const int searchLen);
    void sendMessage(char *bfr, size_t len);
    void setDisplay(SwarmDisplay *displayObject);
    int tileCommand(char *command, size_t len, char *bfr);
};
