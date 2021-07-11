#include "swarmNode.h"
#include <Adafruit_GFX.h>
// this library is driving the OLED display
#include <Adafruit_SH110X.h>


// initialization
SwarmNode::SwarmNode() {
};

/*  Initialize the SWARM tile
 *  
 *  The challenge seems to be that we have no way to issue a hardware reset to the 
 *  tile and cannot issue a software restart unless the tile is in receive mode. 
 *  For this reason the status has to be determined by poking on the tile.
 */
void SwarmNode::begin() {
  Serial2.begin(115200);
  // TODO: change this to a timeout with error message
  // while (Serial2.available() > 0);
  deviceReady(); 
};

void SwarmNode::setDisplay(SwarmDisplay *streamObject) {
  _streamRef=streamObject;
}

// waiting for the initialization message
boolean SwarmNode::deviceReady() {
  char command[] = "$RS*01\n";
  int len = sizeof(command);
  char bfr[256];
  tileCommand(command, 7, bfr);
  while (true) {
    if (_streamRef->getCursorY() > 50) {
      _streamRef->clearDisplay();
      _streamRef->setCursor(0, 0);
    }
    size_t len = getLine(bfr);
    for (int i=0; i<len; i++) {
      _streamRef->print(bfr[i]);
    }
    _streamRef->display();
  }
}

/*  get a line from Serial2
 *  
 *  We are currently using Serial2 in global space
 *  TODO: look into passing an instance instead
 */
int SwarmNode::getLine(char *bfr) {
  int idx = -1;
  char character;
  if (Serial2.available()) {
    do {
      character = Serial2.read();
      if (character != -1) {
        idx ++;
        bfr[idx] = character;
      }
    } while (bfr[idx] != 10 and idx < 255);
  }
  return idx + 1;
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

int SwarmNode::tileCommand(char *command, size_t len, char *bfr) {
  for (int i; i<len+1; i++) _streamRef->print(command[i]);
  int res = Serial2.write(command, len);
  _streamRef->display();
  return 0;
}
  
