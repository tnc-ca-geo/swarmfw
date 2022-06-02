#include "swarmNode.h"
// #include "serialWrapper.h"
#include <Adafruit_GFX.h>
// this library is driving the OLED display
#include <Adafruit_SH110X.h>
#include <time.h>


// time after which Serial2 is considered inactive and empty
const unsigned long READ_TIMEOUT = 100; // ms
// time after which we don't wait any longer for a command response
const unsigned long COMMAND_TIMEOUT = 5000; // ms


/*
 * very crude date evaluation that does not deal with leap years or months with
 * less than 31 days yet.
 */
boolean validateTimeStruct(struct tm tme) {
  // going back to 2019 to make SWARM examples work
  if (tme.tm_year < 119 || tme.tm_year > 137 ) { return false; }
  if (tme.tm_mon < 0 || tme.tm_mon > 11) { return false; }
  if (tme.tm_mday < 1 || tme.tm_mday > 31) { return false; }
  if (tme.tm_hour < 0 || tme.tm_hour > 23) { return false; }
  if (tme.tm_min < 0 || tme.tm_min > 59) { return false; }
  // 61 could theoretically happen IF leap seconds are part of the time
  // however GPS does not do leap seconds
  if (tme.tm_sec < 0 || tme.tm_sec > 61) { return false; }
  return true;
}


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
 *  process this command if starting up but it will restart if already
 *  running. This is just a simply way to get to a known state.
 *
 *  NOT INCLUDED IN TESTS
 */
void SwarmNode::begin(const unsigned long timeReportingFrequency) {
  // We can limit the buffer size since we exactly know how long the buffers
  // within the begin method will be
  char bfr[256];
  char timeFrequencyBfr[16];
  size_t len=0;
  // issue tile reset
  len = tileCommand("$RS", 3, bfr);
  // BLOCKING: wait for indication that tile is running
  while (true) {
    len = getLine(bfr);
    if (len) _wrappedDisplayRef->printBuffer(bfr, len);
    delay(500);
    if (parseLine(bfr, len, "BOOT,RUNNING", 12) > -1) break;
  }
  // TODO: use compile flags instead of program flow for this
  // IF dev=true delete all unsent messages to not use up 720 monthly included
  // messages while developing and testing
  if (dev) {
    _wrappedDisplayRef->printBuffer("DEV MODE:\nDELETING OLD MESSAGES");
    len = tileCommand("$MT D=U", 7, bfr);
  }
  // configure the frequency at which a time report is issued
  _wrappedDisplayRef->printBuffer("CONFIGURE");
  len = sprintf(
    timeFrequencyBfr, "$DT %i", timeReportingFrequency);
  // drastically reduce the number of unsolicited messages
  len = tileCommand("$RT 3600", 8, bfr);
  len = tileCommand("$GN 3600", 8, bfr);
  len = tileCommand("$GS 3600", 8, bfr);
  _wrappedDisplayRef->printBuffer(bfr, len);
  // TODO: this is blocking which is nasty
  // wait until we obtain a valid time message
  // BLOCKING
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
  char bfr[256];
  while (getLine(bfr) > 0);
}

/*
 *  get a line from _serialRef
 */
size_t SwarmNode::getLine(char *bfr) {
  size_t idx = 0;
  char character;
  unsigned long startMillis = millis();
  if (_wrappedSerialRef->available()) {
    do {
      character = _wrappedSerialRef->read();
      // if no character is in the Serial cache it will return 255
      // this is currently blocking until a newline character appears on
      // the Serial
      if (character != 255) {
        bfr[idx] = character;
        idx ++;
        startMillis = millis();
      }
    // return if
    // - EOL
    // - timed out
    // - terminate also after 255 characters
    } while (character != 10 && startMillis + READ_TIMEOUT > millis() && idx < 255);
  }
  return idx;
}

/*
 * issue a time command and read the result
 */
int SwarmNode::getTime(char *bfr) {
  size_t len = tileCommand("$DT @", 5, bfr);
  return len;
}

/*
 * get time stamp for sensor reading
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
 *
 * BLOCKING
 */
unsigned long int SwarmNode::waitForTimeStamp() {
  size_t bfrLen = 0;
  char messageBfr[256];
  // we need to keep that for a check
  unsigned long ret = 0;
  while (true) {
    bfrLen = getLine(messageBfr);
    ret = parseTime(messageBfr, bfrLen);
    if (ret > 0) return ret;
  }
}

/*
 *  TODO: might be more useful as a buffer
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
 * Check weather a buffer contains a valid NMEA check sum
 */
boolean SwarmNode::checkNmeaChecksum(const char *bffr, const size_t len) {
  const uint16_t pos = parseLine(bffr, len, "*", 1);
  char sum[3] = {0};
  memcpy(sum, bffr+pos+1, 2);
  // see https://stackoverflow.com/questions/1070497/c-convert-hex-string-to-signed-integer
  return (nmeaChecksum(bffr, pos) == strtol(sum, NULL, 16));
}

/*
 * Parse a line and return index of find, returns -1 if not found
 * TODO: There might be fancier algorithms to do that such as Boyer-Moore
 * or even a standard function
 */
int16_t SwarmNode::parseLine(
  const char *line, const size_t len, const char *searchTerm,
  const size_t searchLen
) {
  int16_t ret = -1;
  if (searchLen <= len) {
    // Note: we need to make sure that two buffers with the same length can be
    // found, so next loop needs to run at least once
    for (size_t i=0; i <= len-searchLen; i++) {
      ret = i;
      for (size_t j=0; j < searchLen; j++) {
        if (searchTerm[j] != line[i+j]) {
          ret = -1;
          break;
        }
      }
      if (ret != -1) break;
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
  // check NMEA checksum whether response is complete
  if (!checkNmeaChecksum(timeResponse, len)) { return 0; }
  // check valid flag on time reading
  if (parseLine(timeResponse, len, ",V*", 3) < 0 ) { return 0; }
  // this is a little bit lazy way to determine whether we have the
  // right message type but it will work for the next 979 years
  if (parseLine(timeResponse, len, "$DT 2", 5) > -1) {
    memcpy(part, timeResponse + 4, 4);
    part[4] = '\0';
    // time format stores years since 1900
    time.tm_year = strtol(part, NULL, 10) - 1900;
    // check whether valid year: 2010 to 2037
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
    if (validateTimeStruct(time)) { return mktime(&time); };
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
 *
 *
 * BLOCKING
 */
 size_t SwarmNode::tileCommand(
   const char *command, const size_t len, char *bfr
 ) {
   char commandBfr[len+4];
   size_t retLen = 0;
   unsigned long startMillis = millis();
   cleanCommand(command, len, commandBfr);
   _wrappedDisplayRef->shortPrintBuffer(commandBfr, len+4);
   _wrappedSerialRef->write(commandBfr, len+4);
   // discard unsolicated messages if they arrive between a command and the
   // command response
   do {
     retLen = getLine(bfr);
   } while (
     parseLine(bfr, 3, commandBfr, 3) < 0
     && startMillis + COMMAND_TIMEOUT > millis()
   );
   _wrappedDisplayRef->shortPrintBuffer(bfr, retLen);
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
