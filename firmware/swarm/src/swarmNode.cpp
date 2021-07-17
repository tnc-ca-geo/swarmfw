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
 *
 *   NOT INCLUDED IN TESTING
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
size_t SwarmNode::cleanCommand(const char *command, size_t len, char *bfr) {
  memcpy(bfr, command, len);
  bfr[len] = '*';
  char hexbfr[2];
  sprintf(hexbfr, "%02x", nmeaChecksum(command, len));
  for (int i=0; i<2; i++) bfr[len+1+i] = hexbfr[i];
  bfr[len+3] = '\n';
  bfr[len+4] = '\0';
  return len + 5;
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
size_t SwarmNode::getLine(char *bfr) {
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
int SwarmNode::getTime(char *bfr) {
  return tileCommand("$DT @", 5, bfr);
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

/*
 * Parse a line and return whether it contains another starting
 *
 * TODO: There is probably a native command to do that
 * TODO: return integer first position for more functionality
 */
boolean SwarmNode::parseLine(
  const char *line, size_t len, const char *searchTerm, size_t searchLen) {
  boolean ret = false;
  if (searchLen > len) return false;
  for (int i=0; i < len-searchLen; i++) {
    for (int j=0; j < searchLen; j++) {
      if (searchTerm[j] == line[i+j]) ret = true;
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
 * Parse time return from SWARM tile into epoch time_t
 *
 * Returm type could be t_time but that does not play well with the Arduino
 * test framework
 */
int SwarmNode::parseTime(const char *timeResponse, size_t len) {
  char part[5];
  struct tm time = {0};
  // this is a little bit lazy way to determine whether we have the
  // right message type but it will work for the next 979 years
  if (parseLine(timeResponse, len, "$DT 2", 4)) {
    memcpy(part, timeResponse + 4, 4);
    part[4] = '\0';
    time.tm_year = strtol(part, NULL, 10) - 1900;
    memcpy(part, timeResponse + 8, 2);
    // we have to do that only once since we copy
    // nly 2 bytes in the following steps
    part[2] = '\0';
    // struct works with 0-indexed month
    time.tm_mon = strtol(part, NULL, 10) - 1;
    memcpy(part, timeResponse + 10, 2);
    time.tm_mday = strtol(part, NULL, 10);
    memcpy(part, timeResponse + 12, 2);
    time.tm_hour = strtol(part, NULL, 10);
    memcpy(part, timeResponse + 14, 2);
    time.tm_min = strtol(part, NULL, 10);
    memcpy(part, timeResponse + 16, 2);
    time.tm_sec = strtol(part, NULL, 10);
    return mktime(&time);
  }
  return -1;
}

/*
 * Format a text message, add metadata, and send to tile
 */
void SwarmNode::sendMessage(const char *message, size_t len) {
  // buffer for feedback output
  char res[256];
  // buffer for index
  char numberBfr[6];
  // buffer to assemble the command
  char commandBfr[255];
  // index for commandBfr
  unsigned int commandIdx = 0;
  sprintf(numberBfr, "%06u", messageCounter);
  char part1[] = "$TD HD=86400,\"{\"idx\":";
  commandIdx = sizeof(part1) - 1;
  memcpy(commandBfr, part1, commandIdx);
  memcpy(commandBfr + commandIdx, numberBfr, sizeof(numberBfr));
  commandIdx += sizeof(numberBfr);
  char part2[] = ",\"payload\":\"";
  memcpy(commandBfr + commandIdx, part2, sizeof(part2));
  commandIdx += sizeof(part2) - 1;
  for (int i=0; i<len; i++) {
    commandBfr[commandIdx] = message[i];
    commandIdx++;
  }
  char part3[] = "\"}\"";
  memcpy(commandBfr + commandIdx, part3, sizeof(part3));
  commandIdx += sizeof(part3) - 1;
  len = tileCommand(commandBfr, commandIdx, res);
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
size_t SwarmNode::tileCommand(const char *command, size_t len, char *bfr) {
  char commandBuffer[len+4];
  cleanCommand(command, len, commandBuffer);
  _displayRef->printBuffer(commandBuffer, len+4);
  int res = Serial2.write(commandBuffer, len+4);
  return getLine(bfr);
}
