#include "newSdi12Wrapper.h"
#include <SDI12.h>
#include "myHelpers.h"


SDI12 mySdi12(NEW_DATA_PIN);


sdi12Interface::sdi12Interface() {
  mySdi12.begin();
  delay(1000);
};


void sdi12Interface::loopOnce() {
  readSdi12Buffer();
};

/*
 *  Read the SDI-12 buffer character by character since SDI12 is rather slow
 */
void sdi12Interface::readSdi12Buffer() {
  size_t pos=0;
  char character = 0;
  // terminate return for the case that there is no return
  // this is important for \0 terminated strings
  if (mySdi12.available()) {
    character = mySdi12.read();
    pos = strlen(responseStack);
    if (pos < NEW_SDI12_BUFFER_SIZE-2) { responseStack[pos] = character; }
    pos = strlen(outputStack);
    if (pos < NEW_SDI12_BUFFER_SIZE-2) { outputStack[pos] = character; }
  }
};

/*
 *  Send an SDI-12 command
 */
void sdi12Interface::sendSdi12(char *bfr) {
  helpers::pushToStack(outputStack, bfr, strlen(bfr), NEW_SDI12_BUFFER_SIZE);
  mySdi12.sendCommand((char*) bfr);
};

/*
 * Read a line from the outputStack
 */
size_t sdi12Interface::readLine(char *bfr) {
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
 * A class holding complex workflows
 */
newSdi12Measurement::newSdi12Measurement() {};


void newSdi12Measurement::loopOnce() {
  interface.loopOnce();
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
    responseBfr, interface.responseStack, NEW_SDI12_BUFFER_SIZE);
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
  interface.sendSdi12(cmd);
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
