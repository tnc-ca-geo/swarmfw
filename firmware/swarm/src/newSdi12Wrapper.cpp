#include "newSdi12Wrapper.h"
#include <SDI12.h>
#include "myHelpers.h"


Sdi12Wrapper::Sdi12Wrapper():
  _sdi12(SDI12(NEW_DATA_PIN)) {};
void Sdi12Wrapper::begin() { _sdi12.begin(); };
bool Sdi12Wrapper::available() { return _sdi12.available(); };
char Sdi12Wrapper::read() { return _sdi12.read(); };
void Sdi12Wrapper::sendCommand(const char *cmd) {
  return _sdi12.sendCommand(cmd);
};


Sdi12Interface::Sdi12Interface(Sdi12WrapperBase *sdiRef):
  _sdi12(sdiRef)
{
  _sdi12->begin();
};


void Sdi12Interface::loopOnce() {
  readSdi12Buffer();
};

/*
 *  Read SDI-12 buffer character by character since SDI12 is slow.
 */
void Sdi12Interface::readSdi12Buffer() {
  size_t pos=0;
  char character = 0;
  while (_sdi12->available()) {
    character = _sdi12->read();
    pos = strlen(responseStack);
    if (pos < NEW_SDI12_BUFFER_SIZE-2) { responseStack[pos] = character; }
    pos = strlen(outputStack);
    if (pos < NEW_SDI12_BUFFER_SIZE-2) { outputStack[pos] = character; }
  }
};

/*
 *  Send a SDI-12 command
 */
void Sdi12Interface::sendSdi12(char *bfr) {
  helpers::pushToStack(outputStack, bfr, strlen(bfr), NEW_SDI12_BUFFER_SIZE);
  _sdi12->sendCommand((char*) bfr);
};

/*
 * Read line from outputStack
 * TODO: we might not need this
 */
size_t Sdi12Interface::readLine(char *bfr) {
  size_t ret=helpers::popFromStack(bfr, outputStack, NEW_SDI12_BUFFER_SIZE);
  if (ret) {
    bfr[ret] = '\n';
    return ret+1;
  } else {
    bfr[0] = '\0';
    return 0;
  }
};

/*
 * A class holding complex workflows for measurement retrieval
 */
newSdi12Measurement::newSdi12Measurement(Sdi12Interface *interface):
  // this is a member initializer list, see
  // https://stackoverflow.com/questions/31211319/
  // no-matching-function-for-call-to-class-constructor
  interface(interface)
{};


void newSdi12Measurement::loopOnce() {
  interface->loopOnce();
  parseResponse();
};

/*
 * parse command respone
 */
void newSdi12Measurement::parseResponse() {
  size_t len = 0;
  // initiate the retrieval process if scheduled
  if (
    retrieving && measurementReadyTime < esp_timer_get_time() &&
    retrievalIdx==48)
  {
    retrieveReadings(lastCommand[0], retrievalIdx);
    // give it some time; TODO: improve
    measurementReadyTime = measurementReadyTime + 3000000;
  };
  // maximum retrieval idx reached; abort and yield
  if (retrievalIdx > 57) {
    retrievalIdx = 48;
    retrieving = false;
    ready = true;
  };
  // interpret returns from stack
  len = helpers::popFromStack(
    responseBfr, interface->responseStack, NEW_SDI12_BUFFER_SIZE);
  if (len > 0) {
    if (lastCommand[1] == 'D' && retrieving) {
      memcpy(outputBfr+outputBfrLen, responseBfr+1, len-1);
      outputBfr[outputBfrLen + len-1] = 0;
      outputBfrLen = outputBfrLen+len-1;
      retrievalIdx++;
      retrieveReadings(lastCommand[0], retrievalIdx);
    };
    if (lastCommand[1] == 'I') {
      memcpy(outputBfr, responseBfr, len);
      outputBfrLen = len;
      ready = true;
    };
    if (lastCommand[1] == 'C' or lastCommand[1] == 'M') {
      parseMeasurementResponse(responseBfr);
      retrieving = true;
    };
  };
};

/*
 * assemble and send a command to the SDI-12 interface
 */
void newSdi12Measurement::processCommand(const char addr, const char *command) {
  ready = false;
  char cmd[6] = {0};
  cmd[0] = addr;
  memcpy(cmd+1, command, strlen(command));
  memcpy(cmd+strlen(cmd), "!", 1);
  memcpy(lastCommand, cmd, 6);
  interface->sendSdi12(cmd);
};


void newSdi12Measurement::parseMeasurementResponse(const char *bfr) {
  char seconds[4] = {bfr[1], bfr[2], bfr[3], 0};
  measurementReadyTime = esp_timer_get_time() + atoi(seconds) * 1000000;
  char variables[3] = {bfr[4], bfr[5], 0};
  variableCount = atoi(variables);
};


void newSdi12Measurement::retrieveReadings(const char addr, const char idx) {
  char cmd[] = {'D', idx, '\0'};
  processCommand(addr, cmd);
};


void newSdi12Measurement::getInfo(const char addr) {
  char cmd[] = {'I', '\0'};
  outputBfr[0] = '\0';
  outputBfrLen = 0;
  processCommand(addr, cmd);
};


void newSdi12Measurement::getMeasurement(const char addr, const char command) {
  char cmd[] = {command, '\0'};
  outputBfr[0] = '\0';
  outputBfrLen = 0;
  processCommand(addr, cmd);
};


size_t newSdi12Measurement::readResponse(char *bfr) {
  if (ready) {
    memcpy(bfr, outputBfr, outputBfrLen);
    bfr[outputBfrLen] = '\0';
    outputBfrLen = 0;
    return outputBfrLen;
  }
  bfr[0] = 0;
  return 0;
};


bool newSdi12Measurement::responseReady() {
  return ready;
};
