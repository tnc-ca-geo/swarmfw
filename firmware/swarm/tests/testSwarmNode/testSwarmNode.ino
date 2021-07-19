// this fixes a bug in Aunit.h dependencies
#line 2 "testSwarmNode.ino"

#include <AUnitVerbose.h>
using namespace aunit;

// this is a little bit a problem in Arduino, the import from relative paths that 
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.

#include "src/swarmNode.h"
// #include "src/serialWrapper.h"

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

class MockedSerialWrapper1: public SerialWrapperBase {
  private:
    uint8_t bfr[255];
    int idx;
    size_t sizeTestData;
  public:
    MockedSerialWrapper1() {
      uint8_t testData[] = "$DT 20190408195123,V*41\nrubbish";
      idx = -1;
      sizeTestData = sizeof(testData);
      memcpy(bfr, testData, sizeTestData);
    };
    char read() {
      idx++;
      if (idx < sizeTestData) return bfr[idx];
      else return -1;
    }
};

test(getTime) {
  MockedSerialWrapper1 wrapper = MockedSerialWrapper1();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char bfr[255];
  assertEqual(wrapper.read(), '$');
  assertEqual(wrapper.read(), 'D');
  testNode.tileCommand("$DT ", 4, bfr);
};

test(parseTime) {
  // example from SWARM Tile Manual 
  char validTimeResponse[] = "$DT 20190408195123,V*41\n";
  char invalidTimeResponse[] = "Fuchsteufelswild\n";
  // return -1 if input cannot be interpreted as time response
  assertEqual(testObj.parseTime(invalidTimeResponse, sizeof(invalidTimeResponse)), -1);
  assertEqual(testObj.parseTime(validTimeResponse, sizeof(validTimeResponse)), 1554753083);
}

// 
// this just tests whether we can actually pass a mocked class
// TODO: remove when we are confident about our tests 
//
class MockedSerialWrapper: public SerialWrapperBase {
  public:
    MockedSerialWrapper() {}; 
    char read() { return 'a'; }
};
test(testMockSerial) {
  MockedSerialWrapper mockedSerialWrapper = MockedSerialWrapper();
  SwarmNode patchedNode = SwarmNode(&displ, &mockedSerialWrapper);
  assertEqual(mockedSerialWrapper.read(), 'a');
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
