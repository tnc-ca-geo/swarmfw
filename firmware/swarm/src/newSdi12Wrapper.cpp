#include "newSdi12Wrapper.h"
#include <SDI12.h>
#include "myHelpers.h"


/*
 * This is a wrapper around the SDI12 library build on an abstract base class
 * allowing for mock creation in test.
 *
 * TODO: We could also create a base class implementing all methods that are
 * not hardware dependant and make the hardware methods abstract. To be
 * implemented later. Then we could run tests on a test implementation of the
 * base class.
 */
Sdi12Wrapper::Sdi12Wrapper(int datapin): _sdi12(SDI12(datapin)) {};
void Sdi12Wrapper::begin() { _sdi12.begin(); };
bool Sdi12Wrapper::available() { return _sdi12.available(); };
char Sdi12Wrapper::read() { return _sdi12.read(); };
void Sdi12Wrapper::sendCommand(const char *bfr) {
  return _sdi12.sendCommand(bfr);
};

/*
 * Class implementing basic interaction with SDI-12 as well as complex
 * measurement workflows
 */
NewSdi12Measurement::NewSdi12Measurement(Sdi12WrapperBase *wrapper):
  // this is a member initializer list (with one item), see
  // https://stackoverflow.com/questions/31211319/
  // no-matching-function-for-call-to-class-constructor
  wrapper(wrapper)
{
  wrapper->begin();
};

/*
 * Loop, call once per main loop.
 * - reads the SDI12 buffer
 * - parses the response if a line is completed
 * - executes scheduled tasks
 */
void NewSdi12Measurement::loopOnce() {
  // reset when timeOut
  if (timeOutTime < esp_timer_get_time()) {
    reset();
    // indicate that measurement came to an end
    ready = true;
  }
  // read incoming data
  readSdi12Buffer();
  // parse response
  parseResponse();
  // run scheduled, order matters here
  runScheduled();
};

/*
 * Getter method for finished measurement result. Empty buffer once read. We end
 * every response with \n so that even empty responses have a length of 1
 */
size_t NewSdi12Measurement::getResponse(char *bfr) {
  if (ready) {
    size_t len = outputBfrLen;
    memcpy(bfr, outputBfr, len);
    memcpy(bfr+len, "\n\0", 2);
    outputBfr[0] = 0;
    outputBfrLen = 0;
    ready = false;
    return len+1;
  }
  bfr[0] = 0;
  return 0;
};

/*
 * Getter method for private ready variable, we need this since the response can
 * be empty
 */
bool NewSdi12Measurement::getResponseReady() { return ready; };

/*
 * run scheduled commands, TODO: convert to a real schedule
 */
void NewSdi12Measurement::runScheduled() {
  // maximum retrieval idx reached; abort and yield
  if (retrievalIdx > 57) {
    retrieving = false;
    sensorCheck = false,
    ready = true;
    retrievalIdx = 48;
  };
  if (retrieving && measurementReadyTime < esp_timer_get_time()) {
    retrieveReadings(lastCommand[0], retrievalIdx);
    // give it some time, 30ms seems to be safe according to SDI spec
    retrievalIdx++;
    retrieving = false;
  };
  if (sensorCheck && measurementReadyTime < esp_timer_get_time()) {
    checkSensor(retrievalIdx);
    retrievalIdx++;
  };
}

/*
 * parse command respone,
 * TODO: restructure into events and actions, create new smaller methods
 */
void NewSdi12Measurement::parseResponse() {
  size_t len = 0;
  char bfr[255] = {0};
  // interpret returns from stack
  len = helpers::popFromStack(bfr, responseStack, NEW_SDI12_BUFFER_SIZE);
  // This relies on the fact that the command receipt in SDI-12 is <CR><LF>
  // Since the \n (<LF>) is removed, we will still parse when there is a <CR>
  // which is particularly important for the aDn! command since it can reproduce
  // empty responses. TODO: make sure that it will not block even if everything
  // else fails
  if (len > 0) {
    if (lastCommand[1] == 'D') {
      // removing address byte from output
      memcpy(outputBfr+outputBfrLen, bfr+1, len-2);
      outputBfrLen = outputBfrLen + len - 2;
      outputBfr[outputBfrLen] = 0;
      retrieving = true;
    };
    if (lastCommand[1] == '!') {
      memcpy(outputBfr+outputBfrLen, bfr, len - 1);
      outputBfrLen = outputBfrLen + len - 1;
      outputBfr[outputBfrLen] = 0;
    };
    if (lastCommand[1] == 'I') {
      memcpy(outputBfr, bfr, len);
      outputBfrLen = len;
      ready = true;
    };
    if (lastCommand[1] == 'C' or lastCommand[1] == 'M') {
      parseMeasurementResponse(bfr);
      retrieving = true;
    };
  };
};

/*
 * assemble and send a command to the SDI-12 wrapper
 */
void NewSdi12Measurement::processCommand(const char addr, const char *command) {
  ready = false;
  char cmd[6] = {0};
  cmd[0] = addr;
  memcpy(cmd+1, command, strlen(command));
  memcpy(cmd+strlen(cmd),  "!", 1);
  memcpy(lastCommand, cmd, 6);
  wrapper->sendCommand((char*) cmd);
  measurementReadyTime = esp_timer_get_time() + SDI12_DEFAULT_COMMAND_TIMEOUT;
};

/*
 * Parses the response to a C! or M! measurement, set attributes that will
 * direct the retrieval process
 */
void NewSdi12Measurement::parseMeasurementResponse(const char *bfr) {
  char seconds[4] = {bfr[1], bfr[2], bfr[3], 0};
  measurementReadyTime = esp_timer_get_time() + atoi(seconds) * 1000000;
  char variables[3] = {bfr[4], bfr[5], 0};
  variableCount = atoi(variables);
};

/*
 *  Read SDI-12 buffer
 */
void NewSdi12Measurement::readSdi12Buffer() {
  // This is potentially dangerous. Since we could flood the Serial with
  // input that does not contain \n and it would be blocking. TODO: add
  // timeout. However, given the slowliness of SDI-12 this should not happen.
  while (wrapper->available()) {
    char character = wrapper->read();
    size_t pos = strlen(responseStack);
    // TODO: add handling if this condition is not met, like pop the oldest
    // message
    if (pos < NEW_SDI12_BUFFER_SIZE-2) { responseStack[pos] = character; }
    if (character == '\n') { break; }
  }
};

/*
 * Trigger aI! command, will delete results, and cancel ongoing retrieval
 */
void NewSdi12Measurement::requestInfo(const char addr) {
  char cmd[] = {'I', '\0'};
  reset();
  processCommand(addr, cmd);
};

/*
 * trigger new measurement, will delete older results, and cancel ongoing
 * retrieval
 */
void NewSdi12Measurement::requestMeasurement(
  const char addr, const char command
) {
  char cmd[] = {command, '\0'};
  reset();
  processCommand(addr, cmd);
};

/*
 * initiate Sensor check
 */
void NewSdi12Measurement::requestSensors() {
  reset();
  sensorCheck = true;
}

/*
 * Reset and terminate all ongoing processes
 */
void NewSdi12Measurement::reset() {
  ready = false;
  retrieving = false;
  sensorCheck = false;
  retrievalIdx = 48;
  outputBfr[0] = 0;
  outputBfrLen = 0;
  timeOutTime = esp_timer_get_time() + SDI12_TIMEOUT;
}

/*
 * Issue aDx! commands to retrieve and assemble complete sensor readings
 */
void NewSdi12Measurement::retrieveReadings(const char addr, const char idx) {
  char cmd[] = {'D', idx, '\0'};
  processCommand(addr, cmd);
};

/*
 * Issue a! command to check whether sensor is attached
 */
void NewSdi12Measurement::checkSensor(const char idx) {
  char cmd[] = "";
  processCommand(idx, cmd);
}
