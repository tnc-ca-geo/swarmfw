#include "swarmNode.h"
#include <Adafruit_GFX.h>
// this library is driving the OLED display
#include <Adafruit_SH110X.h>

/* 
 *  Constructor
 */
SwarmNode::SwarmNode() {
  messageCounter = 0;  
};

/*  
 *   Initialize the SWARM tile
 * 
 *   Issue a reset to the SWARM tile immediately. The tile will not 
 *   be ready to receive the command if starting up anyways. However it 
 *   will restart if already running.
 */
void SwarmNode::begin() {
  char bfr[256];
  size_t len=0;
  Serial2.begin(115200);
  tileCommand("$RS", 3, bfr);
  while (true) {
    len = getLine(bfr);
    if (len) {
      _displayRef->printBuffer(bfr, len);
      // TODO: this works only on actual start up. Find another way
      // to determine whether tile is ready
      if (parseLine(bfr, len, "$TILE BOOT,RUNNING*49", 21)) break;
    }
  }
  // FOR DEV delete all unsent messages to not use up our 
  // 720 included messages with too many junk messages
  len = tileCommand("$MT D=U", 7, bfr);
  _displayRef->printBuffer(bfr, len); 
  do {
    delay(3000);
    len = getTime(bfr);
    // Serial.println(timeLen);
    _displayRef->printBuffer(bfr, len);
  } while (!parseLine(bfr, len, "$DT 20", 5));
};

/* 
 *  Add NMEA checksum and new line to command
 */
int SwarmNode::cleanCommand(char *command, int len, char *bffr) {
  memcpy(bffr, command, len);
  bffr[len] = '*';
  char hexBffr[2];
  sprintf(hexBffr, "%02x", nmeaChecksum(command, len));
  for (int i=0; i<2; i++) bffr[len+1+i] = hexBffr[i];
  bffr[len+3] = '\n';
}

/*  
 *  get a line from Serial2
 *  
 *  We are currently using Serial2 in global space
 *  TODO: look into passing an instance instead
 *  
 *  The current implementation is rather primitive, entirely relaying
 *  on the UART cache
 */
int SwarmNode::getLine(char *bfr) {
  int idx = -1;
  char character;
  if (Serial2.available()) {
    do {
      character = Serial2.read();
      // if no character is in the Serial cash it will return 255
      // this is currently blocking until a newline character appears on
      // the Serial
      if (character != 255) {
        idx ++;
        bfr[idx] = character;
      }
    } while (bfr[idx] != 10 and idx < 255);
  } 
  return idx+1;
}

/* 
 * read and format time
 */
int SwarmNode::getTime(char *bffr) {
  return tileCommand("$DT @", 5, bffr);
}

/* 
 *  get nmeaChecksum of a command
 *  see https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf
 *  page 34 
 */ 
uint8_t SwarmNode::nmeaChecksum(const char *sz, size_t len) {
  size_t i = 0;
  uint8_t cs;
  if (sz [0] == '$') i++;
  for (cs = 0; (i < len) && sz [i]; i++) cs ^= ((uint8_t) sz [i]);
  return cs;
}


boolean SwarmNode::parseLine(
  const char *bffr, const int len, 
  const char *searchTerm, int searchLen) {
  boolean ret = false;
  if (searchLen > len) return false;
  for (int i=0; i < len-searchLen; i++) {
    for (int j=0; j < searchLen; j++) {
      if (searchTerm[j] == bffr[i+j]) ret = true;
      else {
        ret = false;
        break;
      }
    }
    if (ret) break;
  } 
  return ret;
}

/*
 * Format a text message, add metadata, and send to tile
 */
void SwarmNode::sendMessage(char *bfr, size_t len) {
  // buffer for feedback output
  char res[256];
  // buffer for index
  char numberBffr[6];
  // buffer to assemble the command
  char commandBffr[255];
  // index for commandBffr
  unsigned int commandIdx = 0;
  sprintf(numberBffr, "%06u", messageCounter);
  char part1[] = "$TD HD=86400,\"{\"idx\":";
  commandIdx = sizeof(part1) - 1;
  memcpy(commandBffr, part1, commandIdx);
  memcpy(commandBffr + commandIdx, numberBffr, sizeof(numberBffr));
  commandIdx += sizeof(numberBffr);
  char part2[] = ",\"payload\":\"";
  memcpy(commandBffr + commandIdx, part2, sizeof(part2));
  commandIdx += sizeof(part2) - 1;
  for (int i=0; i<len; i++) { 
    commandBffr[commandIdx] = bfr[i];
    commandIdx++;
  }
  char part3[] = "\"}\"";
  memcpy(commandBffr + commandIdx, part3, sizeof(part3));
  commandIdx += sizeof(part3) - 1;
  len = tileCommand(commandBffr, commandIdx, res); 
  _displayRef->printBuffer(res, len);
  messageCounter++;
}

void SwarmNode::setDisplay(SwarmDisplay *displayObject) {
  _displayRef=displayObject;
}

/*
 * Send a command to the tile
 * TODO: implement NMEA checksum
 * - currently only returning the last line of response, however that creates
 * a race condition since an unsolicitated message could arrive in the meanwhile
 * TODO: implement independant serial buffer later
 */
int SwarmNode::tileCommand(char *command, size_t len, char *bffr) {
  char commandBuffer[len+4];
  cleanCommand(command, len, commandBuffer);
  _displayRef->printBuffer(commandBuffer, len+4);
  int res = Serial2.write(commandBuffer, len+4);
  return getLine(bffr);
}
