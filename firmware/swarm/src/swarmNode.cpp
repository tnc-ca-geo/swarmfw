#include "swarmNode.h"
// #include "serialWrapper.h"
#include <Adafruit_GFX.h>
// this library is driving the OLED display
#include <Adafruit_SH110X.h>
#include <time.h>

/*
 *  Constructor
 *
 *  - pass wrappers for hardware dependant functionality.
 *  - use dev=true for dev specific functionality, e.g. deleting unsent messages
 */
SwarmNode::SwarmNode(
  DisplayWrapperBase *wrappedDisplayObject,
  SerialWrapperBase *wrappedSerialObject, boolean const devMode)
{
  _wrappedDisplayRef = wrappedDisplayObject;
  _wrappedSerialRef = wrappedSerialObject;
  dev = devMode;
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
void SwarmNode::begin(const int timeReportingFrequency) {
  char bfr[256];
  size_t len=0;
  tileCommand("$RS", 3, bfr);
  while (true) {
    len = getLine(bfr);
    if (len) {
      _wrappedDisplayRef->printBuffer(bfr, len);
      // TODO: this works only on actual start up. Find another way
      // to determine whether tile is ready
      if (parseLine(bfr, len, "$TILE BOOT,RUNNING*49", 21)) break;
    }
  }
  // FOR DEV delete all unsent messages to not use up our
  // 720 included messages with too many junk messages
  if (dev) {
    _wrappedDisplayRef->printBuffer("DEV MODE:\nDELETING OLD MESSAGES");
    len = tileCommand("$MT D=U", 7, bfr);
  }
  char timeFrequencyBffr[16];
  size_t timeFrequencyBffrLen = 0;
  timeFrequencyBffrLen = sprintf(
    timeFrequencyBffr, "$DT %i", timeReportingFrequency);
  tileCommand(timeFrequencyBffr, timeFrequencyBffrLen, bfr);
  // wait until we obtain a valid date
  while (waitForTimeStamp() == 0);
};

/*
 *  Add NMEA checksum and new line to command
 */
size_t SwarmNode::cleanCommand(
  const char *command, const size_t len, char *bfr
) {
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
 *  make sure there is nothing in the Serial buffer
 *
 *  - very crude solution to the problem of unsolicitated
 *    messages and still prone to race conditions
 */
void SwarmNode::emptySerialBuffer() {
  char bfr[255];
  while (getLine(bfr) > 0);
}

/*
 *  get a line _serialRef
 */
size_t SwarmNode::getLine(char *bfr) {
  int idx = -1;
  char character;
  if (_wrappedSerialRef->available()) {
    do {
      character = _wrappedSerialRef->read();
      // if no character is in the Serial cache it will return 255
      // this is currently blocking until a newline character appears on
      // the Serial
      if (character != 255) {
        idx ++;
        bfr[idx] = character;
      }
    // return if
    // - EOL
    // - 255 characters have been returned
    // - when no characters left
    } while (character != 10 and idx < 256);
  }
  /* if (idx>2) {
    if (Serial) {
      Serial.print("DEBUG ");
      Serial.write(bfr, idx+1);
      Serial.println();
    }
  } */
  return idx+1;
}

/*
 * read time
 */
int SwarmNode::getTime(char *bfr) {
  size_t len = tileCommand("$DT @", 5, bfr);
  _wrappedDisplayRef->printBuffer(bfr, len);
  return len;
}

/*
 * get time stamp for sensor reading, retry if time not valid
 */
unsigned long int SwarmNode::getTimeStamp() {
  char timBfr[32];
  size_t len = getTime(timBfr);
  return parseTime(timBfr, len);
}

/*
 * Since time reporting works on an unsolicitated basis we need to wait for a
 * time report to exactly sync the loop.
 * This function is blocking and return time depends on the setting of the
 * reporting frequency which also determines precision and power consumption
 * This function should govern the main loop and the measurement timing
 */
unsigned long int SwarmNode::waitForTimeStamp() {
  size_t bfrLen = 0;
  char messageBfr[256];
  // we need to keep that for a check
  unsigned long int ret = 0;
  while (true) {
    bfrLen = getLine(messageBfr);
    // output incoming messages
    if (bfrLen > 0) _wrappedDisplayRef->printBuffer(messageBfr, bfrLen);
    // try to interpret as time
    ret = parseTime(messageBfr, bfrLen);
    if (ret > 0) return ret;
  }
}

/*
 *  get nmeaChecksum of a command
 *  see https://swarm.space/wp-content/uploads/2021/06/Swarm-Tile-Product-Manual.pdf
 *  page 34
 */
uint8_t SwarmNode::nmeaChecksum(const char *sz, const size_t len) {
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
  const char *line, size_t len, const char *searchTerm,
  const size_t searchLen
) {
  boolean ret = false;
  if (searchLen > len) return false;
  for (int i=0; i < len-searchLen; i++) {
    ret = true;
    for (int j=0; j < searchLen; j++) {
      if (searchTerm[j] != line[i+j]) {
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
unsigned long int SwarmNode::parseTime(
  const char *timeResponse, const size_t len
) {
  char part[5];
  // see https://newbedev.com/shell-arduino-esp32-getlocaltime-time-h-struct-tm-code-example
  struct tm time = {0};
  // this is a little bit lazy way to determine whether we have the
  // right message type but it will work for the next 979 years
  if (parseLine(timeResponse, len, "$DT 2", 5)) {
    memcpy(part, timeResponse + 4, 4);
    part[4] = '\0';
    time.tm_year = strtol(part, NULL, 10) - 1900;
    memcpy(part, timeResponse + 8, 2);
    // we have to do that only once since we copy
    // 2 bytes in every of the following steps
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
  return 0;
}

/*
 * Format a message
 */
size_t SwarmNode::formatMessage(
  const char *message, const size_t len, char *bfr
) {
  char prefix[] = "$TD HD=86400,";
  size_t commandIdx = sizeof(prefix) - 1;
  Serial.println("DEBUG 4");
  memcpy(bfr, prefix, commandIdx);
  Serial.println("DEBUG 5");
  // limit by Tile spec
  char convertedMessage[512];
  size_t convertedLen = toHexString(message, len, convertedMessage);
  Serial.println("DEBUG 6");
  memcpy(bfr + commandIdx, convertedMessage, convertedLen);
  Serial.println("DEBUG 7");
  commandIdx = commandIdx + convertedLen;
  return commandIdx;
}

/*
 * Format a text message, add metadata, and send to tile
 */
void SwarmNode::sendMessage(const char *message, const size_t len) {
  char commandBfr[512];
  char responseBfr[512];
  size_t responseLen=len;
  // Serial.println("DEBUG 1");
  responseLen = formatMessage(message, responseLen, commandBfr);
  // Serial.println("DEBUG 2");
  responseLen = tileCommand(commandBfr, responseLen, responseBfr);
  // Serial.println("DEBUG 3");
  // _wrappedDisplayRef->printBuffer(responseBfr, responseLen);
}

/*
 * Send a command to the tile
 * TODO: implement NMEA checksum
 * - currently only returning the last line of response, however that creates
 * a race condition since an unsolicitated message could arrive in the meanwhile
 * TODO: remove serial buffer and give entire control to waitForTimeStamp routine
 */
 size_t SwarmNode::tileCommand(const char *command, size_t len, char *bfr) {
   char commandBfr[len+4];
   cleanCommand(command, len, commandBfr);
   _wrappedDisplayRef->printBuffer(commandBfr, len+4);
   _wrappedSerialRef->write(commandBfr, len+4);
   return getLine(bfr);
 }

 /*
  * Convert to ASCII representation of the HEX values because of special
  * characters
  *
  * - TODO: adapt for \0 terminated strings
  */
  size_t SwarmNode::toHexString(
    const char *inputBfr, const size_t inputLength, char *bfr
  ) {
    size_t res = 2 * inputLength;
    // Serial.println(static_cast<double>(inputLength));
    for (int i=0; i < static_cast<double>(inputLength); i++) {
      char smallBfr[2];
      sprintf(smallBfr, "%02x", inputBfr[i]);
      // Serial.print("OFFSET ");
      // Serial.println(i*2);
      memcpy(bfr+i*2, smallBfr, 2);
    }
    // Serial.println("DEBUG 8");
    return res;
  }
