#include "newSwarmNode.h"
#include "myHelpers.h"

/*
 * Check whether bfr contains valid NMEA check sum
 */
bool checkNmeaChecksum(const char *bfr, const size_t len) {
  const uint16_t pos = helpers::searchBfr(bfr, len, "*", 1);
  char sum[3] = {0};
  memcpy(sum, bfr+pos+1, 2);
  // see https://stackoverflow.com/questions/1070497/
  // c-convert-hex-string-to-signed-integer
  return (nmeaChecksum(bfr, pos) == strtol(sum, NULL, 16));
};

/*
 * Clean SWARM coomand for sending, add NMEA checksum
 */
size_t cleanCommand(char *bfr, const char *cmd, const size_t len) {
  // sprintf returns trailing \0 terminator
  char hexbfr[5];
  memcpy(bfr, cmd, len);
  bfr[len] = '*';
  sprintf(hexbfr, "%02x\n\0", nmeaChecksum(cmd, len));
  // don't use \0 terminators at this point
  memcpy(bfr+len+1, hexbfr, 4);
  return len + 4;
}

/*
 * Format a tile message for sending
 */
size_t formatMessage(const char *message, const size_t len, char *bfr) {
  // limit by Tile spec
  char convertedMessage[256];
  char prefix[] = "$TD HD=86400,";
  size_t commandIdx = sizeof(prefix) - 1;
  memcpy(bfr, prefix, commandIdx);
  size_t convertedLen = toHexString(message, len, convertedMessage);
  memcpy(bfr + commandIdx, convertedMessage, convertedLen);
  return commandIdx + convertedLen;
}

/*
 * Create NMEA checksum
 *
 * TODO: it might be more convenient if we return as string? See sprintf
 * command in cleanCommand
 */
uint8_t nmeaChecksum(const char *bfr, const size_t len) {
  size_t i = 0;
  uint8_t cs;
  if (bfr [0] == '$') i++;
  for (cs = 0; (i < len) && bfr [i]; i++) cs ^= ((uint8_t) bfr [i]);
  return cs;
};

/*
 * Create UNIX epoch from SWARM time message, perform validity checks
 */
uint64_t parseTime(const char *bfr, const size_t len) {
  char part[5];
  // see https://newbedev.com/
  // shell-arduino-esp32-getlocaltime-time-h-struct-tm-code-example
  struct tm time = {0};
  // check NMEA checksum whether response is complete
  if (helpers::searchBfr(bfr, len, ",V*", 3) > -1 ) {
    // this is a little bit lazy way to determine whether we have the
    // right message type but it will work for the next 979 years
    memcpy(part, bfr + 4, 4);
    part[4] = '\0';
    // time format stores years since 1900
    time.tm_year = strtol(part, NULL, 10) - 1900;
    // check whether valid year: 2010 to 2037
    memcpy(part, bfr + 8, 2);
    // we have to do that only once since we copy
    // 2 bytes in every of the following steps
    part[2] = '\0';
    // struct works with 0-indexed month
    time.tm_mon = strtol(part, NULL, 10) - 1;
    memcpy(part, bfr + 10, 2);
    time.tm_mday = strtol(part, NULL, 10);
    memcpy(part, bfr + 12, 2);
    time.tm_hour = strtol(part, NULL, 10);
    memcpy(part, bfr + 14, 2);
    time.tm_min = strtol(part, NULL, 10);
    memcpy(part, bfr + 16, 2);
    time.tm_sec = strtol(part, NULL, 10);
    if (validateTime(time)) { return mktime(&time); };
  }
  return 0;
}

/*
 * Convert to ASCII representation of HEX values; because of special
 * characters that cannot be transmitted as double-quoted string
 * See TD section in SWARM manual
 */
 size_t toHexString(const char *inputBfr, const size_t len, char *bfr) {
   // sprintf outputs a terminating \0 character for this reason length 3
   // see https://stackoverflow.com/questions/3706086/
   // using-sprintf-will-change-the-specified-variable
   char smallBfr[3];
   for (size_t i=0; i<len; i++) {
     sprintf(smallBfr, "%02x", inputBfr[i]);
     // use without the terminating \0 character
     memcpy(bfr+i*2, smallBfr, 2);
   }
   return len * 2;
 }

/*
 * Validate whether a line is a complete and valid response
 */
bool validateResponse(const char *bfr, const size_t len) {
  // check whether first character is '$'
  if (bfr[0] != '$') { return false; }
  // check whether check sum is correct
  return checkNmeaChecksum(bfr, len);
};

/*
 * very crude date evaluation that does not deal with leap years or months with
 * less than 31 days yet.
 */
bool validateTime(struct tm tme) {
  // going back to 2019 to make SWARM examples work
  if (119 > tme.tm_year || tme.tm_year > 137 ) { return false; }
  if (0 > tme.tm_mon || tme.tm_mon > 11) { return false; }
  if (1 > tme.tm_mday || tme.tm_mday > 31) { return false; }
  if (0 > tme.tm_hour || tme.tm_hour > 23) { return false; }
  if (0 > tme.tm_min || tme.tm_min > 59) { return false; }
  // 61 could theoretically happen IF leap seconds are part of the time
  // however GPS does not do leap seconds
  if (0 > tme.tm_sec || tme.tm_sec > 61) { return false; }
  return true;
};


/* -----------------------------------------------------------------------------
 * Main class
 * -----------------------------------------------------------------------------
 */
NewSwarmNode::NewSwarmNode(
  SerialWrapperBase *wrappedSerialObject, const bool devMode)
{
  _wrappedSerialRef = wrappedSerialObject;
  dev = devMode;
};

/*
 * LOOP
 *
 * Define workflow here
 *
 */
void NewSwarmNode::loopOnce() {
  checkTimeAndLocationAge();
  readSerial();
  parseResponse();
  // issue reset command if not initialized
  if (!swarmInitialized && !checkCommandBlock()) { initialize(); }
  // run commands until in commandStack until empty
  if (swarmInitialized) { sendCommands(); }
}

/*
 * Determine whether we can load the message register. Checking for time here is
 * a little bit crude but we cannot send messages before we obtain time
 * TODO: be a more subtle here
 */
bool NewSwarmNode::tileReady() {
  return swarmInitialized && !commandStack[0] && (timestamp != 0);
}

/*
 * Set the command buffer only if tile is ready. We can buffer a series of
 * commands but they must separated by '\n'. Return false when command could
 * not be accepted.
 */
bool NewSwarmNode::setCommandStack(const char *bfr, size_t len) {
  bool ready = tileReady();
  if (ready) {
    ready = helpers::pushToStack(commandStack, bfr, len, STACK_SIZE);
  }
  return ready;
}

/*
 * Format message and store in command stack
 */
bool NewSwarmNode::sendMessage(const char *bfr, size_t len) {
  char messageBfr[255] = {0};
  size_t newLen = formatMessage(bfr, len, messageBfr);
  return setCommandStack(messageBfr, newLen);
};

/*
 * Execute a stack of commands
 */
void NewSwarmNode::sendCommands() {
  char command[255] = {0};
  char expected[32] = {0};
  if (!checkCommandBlock()) {
    int16_t pos = helpers::popFromStack(command, commandStack, STACK_SIZE);
    if (pos) {
      memcpy(expected, command, 3);
      memcpy(expected+3, " OK", 3);
      issueTileCommand(command, pos, expected, 6, 2);
    }
  }
};


void NewSwarmNode::initialize() {
  // extending the timeout since boot up takes awhile
  issueTileCommand("$RS", 3, "BOOT,RUNNING", 12, 30);
};


void NewSwarmNode::setCommandBlock(const uint64_t timeout) {
  commandTimeOut = esp_timer_get_time() + timeout;
};


void NewSwarmNode::removeCommandBlock() {
  commandTimeOut = 0;
};


bool NewSwarmNode::checkCommandBlock() {
  return commandTimeOut > esp_timer_get_time();
};

/*
 * Read all characters available on _wrappedSerialRef and put them into the
 * response and output stacks. Response stack messages will be parsed.
 */
void NewSwarmNode::readSerial() {
  uint16_t pos = 0;
  while (_wrappedSerialRef->available()) {
    char character = _wrappedSerialRef->read();
    pos = strlen(responseStack);
    if (pos < STACK_SIZE-2) {
      responseStack[pos] = character;
      responseStack[pos+1] = 0;
    } else { break; }
    pos = strlen(outputStack);
    if (pos < STACK_SIZE-2) {
      outputStack[pos] = character;
      responseStack[pos+1] = 0;
    }
    // Release after every complete line.
    if (character == 10) { break; }
  }
};

/*
 * Access Serial line through this method for display in application. This way
 * we don't need to pass in a display reference.
 */
size_t NewSwarmNode::readLine(char *bfr) {
  size_t res = helpers::popFromStack(bfr, outputStack, STACK_SIZE);
  bfr[res] ='\n';
  if (res) { res++; }
  bfr[res] = 0;
  return res;
};

/*
 * Send a command to SWARM tile
 */
void NewSwarmNode::issueTileCommand(
  const char *command, size_t len, const char *release,
  const size_t releaseLen, const uint16_t blockTime)
{
  size_t cleanedCommandLen = 0;
  char cleanedCommand[255] = {0};
  char bfr[len+64] = {0};
  // blockTime is currently at least 1s which is a lot
  setCommandBlock(blockTime * 1000 * 1000);
  // store string that would signal successful execution, limit length
  // will remove commandBlock in parseLine
  expectedBfrLen = releaseLen;
  if (releaseLen > 30) { expectedBfrLen = 30; }
  memcpy(expectedBfr, release, expectedBfrLen);
  expectedBfr[expectedBfrLen+1] = 0;
  // clean command, remove trailing characters, add checksum
  cleanedCommandLen = cleanCommand(bfr, command, len);
  // push cleaned command to output Stack
  helpers::pushToStack(outputStack, bfr, cleanedCommandLen, STACK_SIZE);
  _wrappedSerialRef->write(bfr, cleanedCommandLen);
};

/*
 * Invalidate time and location when stale, currently set to a minute
 */
void NewSwarmNode::checkTimeAndLocationAge() {
  if (timestampObtained < esp_timer_get_time() - 6000000) { timestamp = 0; }
};

/*
 * Parse a line from reponseStack and update state. Only implement what needed
 * sofar.
 */
void NewSwarmNode::parseResponse() {
  char line[255] = {0};
  size_t len = helpers::popFromStack(line, responseStack, STACK_SIZE);
  if (!validateResponse(line, len)) { return; }
  // process unsolicated messages first, don't remove command block
  if (helpers::searchBfr(line, len, "$DT 2", 5) > -1) {
    timestamp = parseTime(line, len);
    timestampObtained = esp_timer_get_time();
  };
  // check expected response; remove command block if match
  if (helpers::searchBfr(line, len, expectedBfr, expectedBfrLen) > -1) {
    removeCommandBlock();
    expectedBfr[0] = 0;
    expectedBfrLen = 0;
  };
  // check whether initialized; set flag
  if (helpers::searchBfr(line, len, "BOOT,RUNNING", 12) > -1) {
    swarmInitialized = true;
  };
};
