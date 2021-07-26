// this fixes a bug in Aunit.h dependencies
#line 2 "testSwarmNode.ino"

#include <AUnitVerbose.h>
using namespace aunit;

// this is a little bit a problem in Arduino, the import from relative paths that 
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.

#include "src/swarmNode.h"

// By using the base classes we do not rely on the presence of particular Hardware
DisplayWrapperBase displ = DisplayWrapperBase();
SerialWrapperBase wrapper = SerialWrapperBase();
SwarmNode testObj = SwarmNode(&displ, &wrapper);

test(cleanCommand) {
  char bfr[32];
  int len = 0;
  char expected[] = "$RS*01\n\0";
  len = testObj.cleanCommand("$RS", 3, bfr);
  assertEqual(len, 8);
  for (int i=0; i<sizeof(expected); i++) {
    assertEqual(bfr[i], expected[i]);
  }
}

test(parseLine) {
  char haystack[] = "This is a haystack and we would like to find a needle in here.\n\0";
  assertTrue(testObj.parseLine(haystack, sizeof(haystack), "needle", 5));
  assertFalse(testObj.parseLine(haystack, sizeof(haystack), "gold", 4));
  assertFalse(testObj.parseLine("short", 5, "to long", 7));
}

test(formatMessage) {
  char message[256];
  int len = testObj.formatMessage("hallo", 5, message); 
  assertEqual(len, 20);
  for (int i=0; i<len; i++) assertEqual(message[i], "$TD HD=86400,\"hallo\""[i]);
}

class MockedSerialWrapper: public SerialWrapperBase {
  private:
    char bfr[256];
    size_t idx;
    size_t sizeTestData;
  public:
    char outBfr[256];
    size_t outIdx;
    MockedSerialWrapper() {
      idx = -1;
      outIdx = 0;
    };
    void loadMockedSerialBuffer(char *testData, size_t len) {
      idx = -1;
      sizeTestData = len;
      memcpy(bfr, testData, sizeTestData);
      for (int i=len; i<256; i++) bfr[i] = 255;
    };
    char read() {
      idx++;
      if (idx < sizeTestData) return bfr[idx];
      else return -1;
    };
    size_t write(char *bfr, size_t len) {
      memcpy(outBfr, bfr, len);
      outIdx = len; 
    } 
};

test(sendCommand) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char bfr[32];
  testNode.sendMessage("hello", 5);
  for (int i=0; i<wrapper.outIdx; i++) {
    assertEqual(wrapper.outBfr[i], "$TD HD=86400,\"hello\"*75\n");
  }
}

test(getLine) {
  char testData[] = "A line\nAnother line\nRubbish";
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  wrapper.loadMockedSerialBuffer(testData, sizeof(testData));
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char bfr[256];
  // read the first line
  int len = testNode.getLine(bfr);
  assertEqual(len, 7);
  for (int i=0; i<len; i++) assertEqual(bfr[i], "A line\n"[i]);
  // read the second line
  len = testNode.getLine(bfr);
  assertEqual(len, 13);
  for (int i=0; i<len; i++) assertEqual(bfr[i], "Another line\n"[i]); 
  // make sure third, unterminated line is not blocking
  // len = testNode.getLine(bfr);
  // Serial.print("LEN: ");
  // assertEqual(len, 8);
  // for (int i=0; i<len; i++) assertEqual(bfr[i], "Another line\n"[i]); 
};

test(parseTime) {
  // example from SWARM Tile Manual 
  char validTimeResponse[] = "$DT 20190408195123,V*41\n";
  char invalidTimeResponse[] = "Fuchsteufelswild\n";
  // return -1 if input cannot be interpreted as time response
  assertEqual(
    testObj.parseTime(invalidTimeResponse, sizeof(invalidTimeResponse)), -1);
  assertEqual(
    testObj.parseTime(validTimeResponse, sizeof(validTimeResponse)), 1554753083);
}

// the following sets up the Serial for feedback and starts the test runner
// no need to touch
void setup() {
  Serial.begin(9600);
  delay(500);
  while(!Serial);
}

void loop() {
  aunit::TestRunner::run();
}
