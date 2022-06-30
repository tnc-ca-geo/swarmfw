// this fixes a bug in Aunit.h dependencies
#line 2 "testNewSdi12Wrapper.ino"
#include <AUnitVerbose.h> 
using namespace aunit;
// There is a problem in Arduino; the import from relative paths 
// that are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.
#include "src/newSdi12Wrapper.h"


// Hazardly mocking Hardware dependant class
class MockSdi12Wrapper: public Sdi12WrapperBase {
  private:
    bool avail = true;
    char sdi12Bfr[255] = {0};
    char cmdBfr[32] = {};
  public:
    MockSdi12Wrapper() {};
    // --------------- Methods modifying and reading mock state ------ 
    void setAvailable(const bool value) { avail=value; };
    // while testing use this method between calling the tested method
    // and .loopOnce
    void setSdi12Bfr(const char *bfr, const size_t len) {
      memcpy(sdi12Bfr, bfr, len);
      sdi12Bfr[len] = '\0';
      if (len>0) avail = true;
      else avail=false;
    };
     // get the last command issues
    size_t getSentCommand(char *bfr) {
      size_t len = strlen(cmdBfr);
      memcpy(bfr, cmdBfr, len);
      bfr[len] = 0;
      return len;
    // ---------------------------------------------------------------
    };
    // --------------- Methods of the mocked class -------------------
    void begin() {};
    bool available() { return avail; };
    char read() {
      char ret = sdi12Bfr[0];
      if (ret != 0) memmove(sdi12Bfr, sdi12Bfr+1, 254);
      else avail = false;
      return ret;
    };
    void sendCommand(const char *bfr) {
      Serial.print("Mocked sendCommand: "); Serial.println(bfr);
      size_t len = strlen(bfr);
      memcpy(cmdBfr, bfr, len);
      cmdBfr[len] = 0;
    };
    // ---------------------------------------------------------------
};


test(requestInfo_simple) {
  char bfr[32] = {0};
  uint8_t len = 0;
  MockSdi12Wrapper wrapper = MockSdi12Wrapper();
  NewSdi12Measurement measurement = NewSdi12Measurement(&wrapper);
  measurement.requestInfo('3');
  wrapper.setSdi12Bfr("3001Falk\n", 9);
  measurement.loopOnce();
  // check command issued to SDI-12 
  len = wrapper.getSentCommand(bfr);
  for (size_t i=0; i<3; i++) assertEqual("3I!"[i], bfr[i]);
  len = measurement.getResponse(bfr);
  // Not returning \n. We might change that for consistency.
  assertEqual(len, 9);
  for (uint8_t i=0; i<=len; i++) assertEqual("3001Falk\n\0"[i], bfr[i]);
};

// testing with partial Serial return
test(requestInfo_partial) {
  char bfr[32] = {0};
  uint8_t len = 0;
  MockSdi12Wrapper wrapper = MockSdi12Wrapper();
  NewSdi12Measurement measurement = NewSdi12Measurement(&wrapper);
  measurement.requestInfo('3');
  wrapper.setSdi12Bfr("3001F", 5);
  measurement.loopOnce();
  assertEqual((uint8_t) measurement.getResponse(bfr), 0);
  measurement.loopOnce();
  assertEqual((uint8_t) measurement.getResponse(bfr), 0);
  wrapper.setSdi12Bfr("alk\n", 4);
  measurement.loopOnce();
  len = measurement.getResponse(bfr);
  assertEqual(len, 9);
  for (uint8_t i=0; i<=len; i++) assertEqual("3001Falk\n\0"[i], bfr[i]); 
}


test(requestMeasurement_C_simple) {
  char bfr[128] = {0};
  char expected[] = "+10-10+10-10+10-10+10-10+10-10+10-10+10-10+10-10+10-10+10-10\n";
  uint8_t len = 0;
  MockSdi12Wrapper wrapper = MockSdi12Wrapper();
  NewSdi12Measurement measurement = NewSdi12Measurement(&wrapper);
  measurement.requestMeasurement('3', 'C');
  measurement.loopOnce();
  len = wrapper.getSentCommand(bfr);
  for (size_t i=0; i<3; i++) assertEqual("3C!"[i], bfr[i]);
  // measurement.loopOnce();
  // testing with time 0
  wrapper.setSdi12Bfr("300020\n", 7);
  // one loop for interpreting aC! return
  measurement.loopOnce();
  // still requesting all nine pages
  for (char c=48; c<58; c++) {
    wrapper.setSdi12Bfr("3+10-10\r\n", 9);
    // this is currently necessary since we will wait for sensor but we could move on
    // when response is ready
    delay(400);
    char cmd[] = {'3', 'D', c, '!', '\0'};
    wrapper.getSentCommand(bfr);
    for (size_t i=0; i<4; i++) assertEqual(cmd[i], bfr[i]);
    measurement.loopOnce();
  }
  len = measurement.getResponse(bfr);
  for (size_t i=0; i<strlen(bfr); i++) assertEqual(expected[i], bfr[i]);
  measurement.getResponse(bfr);
  assertEqual(bfr[0], (char) 0);
}


test(processCommand) {
  char bfr[128] = {0};
  uint8_t len = 0;
  MockSdi12Wrapper wrapper = MockSdi12Wrapper();
  NewSdi12Measurement measurement = NewSdi12Measurement(&wrapper);
  // this is important for schecking whether sensor is attached.
  measurement.processCommand('3', "");
  measurement.loopOnce();
  len = wrapper.getSentCommand(bfr);
  assertEqual((uint8_t) len, 2); 
  for (size_t i=0; i<len; i++) assertEqual("3!"[i], bfr[i]);
  measurement.processCommand('2', "D0");
  measurement.loopOnce();
  len = wrapper.getSentCommand(bfr);
  assertEqual((uint8_t) len, 4); 
  for (size_t i=0; i<len; i++) assertEqual("2D0!"[i], bfr[i]);
};


test(requestSensors) {
  char bfr[128] = {0};
  uint8_t len = 0;
  MockSdi12Wrapper wrapper = MockSdi12Wrapper();
  NewSdi12Measurement measurement = NewSdi12Measurement(&wrapper);
  // this is important for schecking whether sensor is attached.
  measurement.requestSensors();
  measurement.loopOnce();
  for (char c=48; c<58; c++) {
    // only sensor '1' and '8' exist
    if (c == 49 || c == 56) {
      char bfrContent[] = {c, '\r', '\n'};
      wrapper.setSdi12Bfr(bfrContent, 3);
    } else {
      char bfrContent[] = {0};
      wrapper.setSdi12Bfr(bfrContent, 0);
    }
    delay(400);
    char cmd[] = {c, '!', '\0'};
    wrapper.getSentCommand(bfr);
    for (size_t i=0; i<2; i++) assertEqual(cmd[i], bfr[i]);
    measurement.loopOnce();
  }
  measurement.getResponse(bfr);
  for (size_t i=0; i<2; i++) assertEqual("18"[i], bfr[i]);
};


// even if there are no sensors we need to come to an end and move on
test(requestSensors_noSensors) {
};


// the following sets up the Serial for feedback and starts the test runner
// no need to touch
void setup() {
  Serial.begin(115200);
  delay(500);
  while(!Serial);
   // TestRunner::exclude("*");
   // TestRunner::include("formatMessageLong");
};



void loop() {
  aunit::TestRunner::run();
}
