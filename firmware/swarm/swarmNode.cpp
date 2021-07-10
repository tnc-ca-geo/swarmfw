#include "swarmNode.h"
#include <Adafruit_GFX.h>
// this library is driving the OLED display
#include <Adafruit_SH110X.h>

// initialization
SwarmNode::SwarmNode() {
};

/* Initialize the SWARM tile 
 */
void SwarmNode::begin() {
  Serial2.begin(115200);
  // TODO: change this to a timeout with error message
  // while (Serial2.available() > 0);
  while (true) {
    size_t len = Serial2.available();
    uint8_t sbuf[len];
    Serial2.readBytes(sbuf, len);
    if (len > 0) {
      if (_streamRef->getCursorY() > 60) {
        _streamRef->clearDisplay();
        _streamRef->setCursor(0, 0);
      }
      for (int i=0; i<len; i++) {
        _streamRef->print((char) sbuf[i]);
      }
      _streamRef->display();
    }
  } 
};

void SwarmNode::setDisplay(Adafruit_SH1107 *streamObject) {
  _streamRef=streamObject;
}

// waiting for the initialization message
boolean SwarmNode::deviceReady() {
}

// just a placeholder for now
boolean SwarmNode::gpsReady() {
  return true;
}

/* get the nmeaChecksum for a command
/  see https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf
/  page 34 */ 
uint8_t SwarmNode::nmeaChecksum(const char *sz, size_t len) {
  size_t i = 0;
  uint8_t cs;
  if (sz [0] == '$') i++;
  for (cs = 0; (i < len) && sz [i]; i++) cs ^= ((uint8_t) sz [i]);
  return cs;
}
