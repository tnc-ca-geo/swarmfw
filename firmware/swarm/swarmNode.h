#ifndef display.h
#include "display.h"
#endif
#include <Arduino.h>


class SwarmNode {

  private: 
    SwarmDisplay *_displayRef;
    static int cleanCommand(char *command, int len, char *bffr);
  public:
    SwarmNode();
    
    void begin();

    // return Serial output on line termination
    int getLine(char *bfr);

    // get and format time
    int getTime(char *bfr);

    // from https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf
    // page 34
    static uint8_t nmeaChecksum(const char *sz, size_t len);

    // check whether string is contained in a line, to identify message types
    static boolean parseLine(
      const char *bffr, const int len, 
      const char *searchTerm, const int searchLen);

    // pass display class
    void setDisplay(SwarmDisplay *displayObject);
    
    // send a NMEA command to the tile and load the result to bfr
    // return the length of the used bfr, bfr should be 256 characters long
    int tileCommand(char *command, size_t len, char *bfr);

    // returns true if a certain interval in seconds is met according to tile clock
    // minimum 300, maximum 86400 (5s, 1day)
    // boolean everyInterval(int interval);
};
