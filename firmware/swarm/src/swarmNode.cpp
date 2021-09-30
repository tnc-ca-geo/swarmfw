#include "swarmNode.h"
// #include "serialWrapper.h"
#include <Adafruit_GFX.h>
// this library is driving the OLED display
#include <Adafruit_SH110X.h>
#include <time.h>

/*
 *  Constructor
 *
 *  - pass wrappers for hardware dependant functionality or mocks for testing
 *  - use dev=true for dev specific functionality, e.g. deleting unsent messages
 */
SwarmNode::SwarmNode(
  DisplayWrapperBase *wrappedDisplayObject,
  SerialWrapperBase *wrappedSerialObject, const boolean devMode)
{
  _wrappedDisplayRef = wrappedDisplayObject;
  _wrappedSerialRef = wrappedSerialObject;
  dev = devMode;
};

/*
 *  Initialize SWARM tile
 *
 *  - Issue a reset to the SWARM tile immediately. Tile will not be ready to
 *  receive the command if starting up anyways. It will restart if already
 *  running. For now the simpliest way to achieve a known state.
 *
 *  NOT INCLUDED IN TESTS
 */
void SwarmNode::begin(const unsigned long int timeReportingFrequency) {
  // We can limit the buffer size since we exactly know how long the buffers
  // within the begin method will be
  char bfr[128];
  char timeFrequencyBfr[16];
  size_t len=0;
  // issue tile reset
  len = tileCommand("$RS", 3, bfr);
  // see on the very bottom of https://github.com/astuder/SwarmTile
  // UNDOCUMENTED, implement as a reaction to a certain send status
  // len = tileCommand("$RS dbinit", 10, bfr);
  // solved with FW 1.1.0
  // wait for indication that tile is running
  while (true) {
    len = getLine(bfr);
    if (len) _wrappedDisplayRef->printBuffer(bfr, len);
    if (parseLine(bfr, len, "$TILE BOOT,RUNNING", 18)) break;
  }
  // IF dev=true delete all unsent messages to not use up 720 monthly included
  // messages while developing and testing
  if (dev) {
    _wrappedDisplayRef->printBuffer("DEV MODE:\nDELETING OLD MESSAGES");
    len = tileCommand("$MT D=U", 7, bfr);
  }
  // configure the frequency at which a time report is issued
  len = sprintf(
    timeFrequencyBfr, "$DT %i", timeReportingFrequency);
  tileCommand(timeFrequencyBfr, len, bfr);
  // wait until we obtain a valid time message
  while (waitForTimeStamp() == 0);
};

/*
 *  Add NMEA checksum and new line to command
 */
size_t SwarmNode::cleanCommand(
  const char *command, const size_t len, char *bfr
) {
  // sprintf returns trailing \0 terminator
  char hexbfr[3];
  memcpy(bfr, command, len);
  bfr[len] = '*';
  sprintf(hexbfr, "%02x", nmeaChecksum(command, len));
  // don't use \0 terminators
  memcpy(bfr+len+1, hexbfr, 2);
  memcpy(bfr+len+3, "\n", 1);
  return len + 4;
}

/*
 *  make sure there is nothing in the Serial buffer
 *
 *  - very crude solution to the problem of unsolicitated messages and still
 *  prone to weird conditions
 */
void SwarmNode::emptySerialBuffer() {
  char bfr[255];
  while (getLine(bfr) > 0);
}

/*
 *  get a line from _serialRef
 */
size_t SwarmNode::getLine(char *bfr) {
  size_t idx = 0;
  char character;
  if (_wrappedSerialRef->available()) {
    do {
      character = _wrappedSerialRef->read();
      // if no character is in the Serial cache it will return 255
      // this is currently blocking until a newline character appears on
      // the Serial
      if (character != 255) {
        bfr[idx] = character;
        idx ++;
      }
    // return if
    // - EOL
    // - 255 char has been returned
    // - when no characters left
    } while (character != 10 and character != 255 and idx < 256);
  }
  return idx;
}

/*
 * issue a time command and read the result
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
 * Since time reporting is unsolicitated basis we need to wait for a time report
 * to sync the loop.
 * This function is blocking and returns time depending on setting of
 * reporting frequency which determines precision and power consumption
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
 * Parse a line and return whether it contains a specific string
 *
 * TODO: There is probably a native command to do that
 * TODO: return integer first position for more functionality
 */
boolean SwarmNode::parseLine(
  const char *line, const size_t len, const char *searchTerm,
  const size_t searchLen
) {
  boolean ret = false;
  if (searchLen <= len) {
    // Note: we need to make sure that two buffers with the same length can be
    // found, so next loop needs to run at leat once
    for (size_t i=0; i <= len-searchLen; i++) {
      ret = true;
      for (size_t j=0; j < searchLen; j++) {
        if (searchTerm[j] != line[i+j]) {
          ret = false;
          break;
        }
      }
      if (ret) break;
    }
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
  // limit by Tile spec
  char convertedMessage[512];
  char prefix[] = "$TD HD=86400,";
  size_t commandIdx = sizeof(prefix) - 1;
  memcpy(bfr, prefix, commandIdx);
  size_t convertedLen = toHexString(message, len, convertedMessage);
  memcpy(bfr + commandIdx, convertedMessage, convertedLen);
  return commandIdx + convertedLen;
}

/*
 * Format a text message, add metadata, and send to tile
 */
void SwarmNode::sendMessage(const char *message, const size_t len) {
  char commandBfr[512];
  char responseBfr[512];
  size_t responseLen=len;
  responseLen = formatMessage(message, responseLen, commandBfr);
  responseLen = tileCommand(commandBfr, responseLen, responseBfr);
}

/*
 *  Send a command to SWARM tile
 *
 *  - returns the response to a command assuming that the first three characters
 *    of the command match the response pattern
 */
 size_t SwarmNode::tileCommand(
   const char *command, const size_t len, char *bfr
 ) {
   char commandBfr[len+4];
   size_t retLen = 0;
   cleanCommand(command, len, commandBfr);
   _wrappedDisplayRef->printBuffer(commandBfr, len+4);
   _wrappedSerialRef->write(commandBfr, len+4);
   do {
     retLen=getLine(bfr);
   } while (!parseLine(bfr, 3, commandBfr, 3));
   _wrappedDisplayRef->printBuffer(bfr, retLen);
   return retLen;
 }

 /*
  * Convert to ASCII representation of HEX values; because of special
  * characters that cannot be transmitted as double-quoted string
  * See TD section in SWARM manual
  */
  size_t SwarmNode::toHexString(
    const char *inputBfr, const size_t len, char *bfr
  ) {
    // sprintf outputs a terminating \0 character for this reason length 3
    // see https://stackoverflow.com/questions/3706086/using-sprintf-will-change-the-specified-variable
    char smallBfr[3];
    for (size_t i=0; i<len; i++) {
      sprintf(smallBfr, "%02x", inputBfr[i]);
      // use without the terminating \0 character
      memcpy(bfr+i*2, smallBfr, 2);
    }
    return len * 2;
  }
