#ifndef display.h
#include "display.h"
#endif
#include <Arduino.h>


class SwarmNode {

  private: 
    SwarmDisplay *_displayRef;
  public:
    SwarmNode();
    
    void begin();

    // return Serial output on line termination
    int getLine(char *bfr);
    
    // send a NMEA command to the tile and load the result to bfr
    // return the length of the used bfr, bfr should be 256 characters long
    int tileCommand(char *command, size_t len, char *bfr);
    
    // check whether tile is ready IMPLEMENT
    boolean deviceReady();
    
    // check whether GPS is ready
    boolean gpsReady();
    
    // from https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf
    // page 34
    static uint8_t nmeaChecksum(const char *sz, size_t len);

    // this is a little bit overspecific
    // TODO: abstract and make work with Serial as well
    void setDisplay(SwarmDisplay *streamObject);
};
