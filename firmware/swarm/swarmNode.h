#include <Arduino.h>
#include <Adafruit_SH110X.h>


class SwarmNode {

  private: 
    Adafruit_SH1107 *_streamRef;
  public:
    SwarmNode();
    
    void begin();
    
    // this is a little bit overspecific
    // TODO: abstract and make work with Serial as well
    void setDisplay(Adafruit_SH1107 *streamObject);
    
    // check whether tile is ready IMPLEMENT
    static boolean deviceReady();
    // check whether GPS is ready
    
    static boolean gpsReady();
    // from https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf
    // page 34
    
    static uint8_t nmeaChecksum(const char *sz, size_t len);
};
