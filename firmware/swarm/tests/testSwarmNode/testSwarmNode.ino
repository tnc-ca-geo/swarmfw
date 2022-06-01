// this fixes a bug in Aunit.h dependencies
#line 2 "testSwarmNode.ino"

#include <AUnitVerbose.h>
using namespace aunit;

// There is a problem in Arduino; the import from relative paths that
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.
#include "src/swarmNode.h"


class MockedSerialWrapper: public SerialWrapperBase {
  private:
    char bfr[256];
    size_t idx;
    size_t sizeTestData;
  public:
    char outBfr[512];
    size_t outIdx;
    MockedSerialWrapper() {
      idx = 0;
      outIdx = 0;
    };
    void loadMockedSerialBuffer(const char *testData, const size_t len) {
      // set properties for use in mocked methods
      idx = 0;
      sizeTestData = len;
      memcpy(bfr, testData, len);
      for (size_t i=len; i<256; i++) bfr[i] = 255;
    };
    char read() {
      Serial.println("MOCK read called");
      idx++;
      if (idx-1 < sizeTestData) {
        return bfr[idx-1];
      }
      else return 255;
    };
    size_t write(char *bfr, size_t len) {
      memcpy(outBfr, bfr, len);
      outIdx = len;
      return len;
    };
    boolean available() {
      boolean res;
      if (idx < sizeTestData) res = true;
      else res = false;
      return res;
    };
};


// There seems to be no need to mock this
DisplayWrapperBase displ = DisplayWrapperBase();


test(cleanCommand) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char bfr[32];
  size_t len = 0;
  char expected[] = "$RS*01\n";
  uint16_t expectedLen = 7;
  len = testNode.cleanCommand("$RS", 3, bfr);
  assertEqual(static_cast<uint16_t>(len), expectedLen);
  for (size_t i=0; i<expectedLen; i++) assertEqual(bfr[i], expected[i]);
}


test(parseLine) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char haystack[] = "This is a haystack and we would like to find a needle in here.\n\0";
  assertTrue(testNode.parseLine(haystack, sizeof(haystack), "needle", 5));
  assertFalse(testNode.parseLine(haystack, sizeof(haystack), "gold", 4));
  assertFalse(testNode.parseLine("short", 5, "to long", 7));
  assertTrue(
    testNode.parseLine("$TILE BOOT,RUNNING*49\n", 22, "$TILE BOOT,RUNNING", 18));
}


test(formatMessage) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char message[256];
  size_t len = testNode.formatMessage("hello", 5, message);
  assertEqual(static_cast<uint16_t>(len), 23);
  for (size_t i=0; i<len; i++) assertEqual(message[i], "$TD HD=86400,68656c6c6f"[i]);
}


test(toHexString) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char testBfr[] = "exciting,test";
  char expected[] = "6578636974696e672c74657374";
  char outBfr[255];
  size_t len = testNode.toHexString(testBfr, 13, outBfr);
  assertEqual(static_cast<uint16_t>(len), 26);
  for (size_t i=0; i<len; i++) assertEqual(outBfr[i], expected[i]);
}


test(toHexStringLong) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char bfr[512];
  char testBfr[] = "We are using a very, very long buffer because there have been trouble"
   "The old breaking value was 54 for input length.";
  size_t len = sizeof(testBfr);
  len = testNode.toHexString(testBfr, len, bfr);
  assertEqual(static_cast<uint16_t>(len), 234);
}


test(formatMessageLong) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char outBfr[241];
  char testBfr[] =
    "000000,1630598125,0+0+0.000+0+0+0.13+111.3+0.16+19.8+1.50+101.22+0.650+19.7+0.1+1.1"
    "+0-0.05+0.12+0.16,4.03";
  char expected[] =
    "$TD HD=86400,3030303030302c313633303539383132352c302b302b302e3030302b302b302b302e3"
    "1332b3131312e332b302e31362b31392e382b312e35302b3130312e32322b302e3635302b31392e372"
    "b302e312b312e312b302d302e30352b302e31322b302e31362c342e3033";
  size_t len = testNode.formatMessage(testBfr, sizeof(testBfr)-1, outBfr);
  assertEqual(static_cast<uint16_t>(len), static_cast<uint16_t>(sizeof(expected))-1);
  for (size_t i=0; i<len; i++) assertEqual(outBfr[i], expected[i]);
}


// using the same message as above
test(cleanCommandLong) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char outBfr[512];
  char testBfr[] =
    "$TD HD=86400,3030303030302c313633303539383132352c302b302b302e3030302b302b302b302e3"
    "1332b3131312e332b302e31362b31392e382b312e35302b3130312e32322b302e3635302b31392e372"
    "b302e312b312e312b302d302e30352b302e31322b302e31362c342e3033";
  char expected[] =
    "$TD HD=86400,3030303030302c313633303539383132352c302b302b302e3030302b302b302b302e3"
    "1332b3131312e332b302e31362b31392e382b312e35302b3130312e32322b302e3635302b31392e372"
    "b302e312b312e312b302d302e30352b302e31322b302e31362c342e3033*1a\n";
  size_t len = testNode.cleanCommand(testBfr, sizeof(testBfr)-1, outBfr);
  for (size_t i=0; i<len; i++) assertEqual(outBfr[i], expected[i]);
  assertEqual(static_cast<uint16_t>(len), static_cast<uint16_t>(sizeof(expected))-1);
};


test(sendMessage) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  wrapper.loadMockedSerialBuffer("$TD OK,5354468575916*2c\n", 24);
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char bfr[32];
  testNode.sendMessage("hello", 5);
  Serial.print("wrapper.outIdx:");
  Serial.println(wrapper.outIdx);
  for (size_t i=0; i<wrapper.outIdx; i++) {
    assertEqual(wrapper.outBfr[i], "$TD HD=86400,68616c6c6f*4a\n");
  }
};


test(sendMessageLong) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  wrapper.loadMockedSerialBuffer("$TD OK,5354468575916*2c\n", 24);
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char testMessage[] =
    "000000,1630598125,0+0+0.000+0+0+0.13+111.3+0.16+19.8+1.50+101.22+0.650+19.7+0.1+1.1"
    "+0-0.05+0.12+0.16,4.03";
  // this generates a /0 terminated string
  char expected[] =
    "$TD HD=86400,3030303030302c313633303539383132352c302b302b302e3030302b302b302b302e3"
    "1332b3131312e332b302e31362b31392e382b312e35302b3130312e32322b302e3635302b31392e37"
    "2b302e312b312e312b302d302e30352b302e31322b302e31362c342e3033*1a\n";
  // use sizeof-1 as length
  testNode.sendMessage(testMessage, sizeof(testMessage)-1);
  // think about this again but, it is two less than above
  assertEqual(
    static_cast<uint16_t>(wrapper.outIdx), static_cast<uint16_t>(sizeof(expected)-1));
  for (size_t i=0; i<wrapper.outIdx; i++) {
    assertEqual(wrapper.outBfr[i], expected[i]);
  }
}


test(getLine) {
  char testData[] = "A line\nAnother line\nRubbish";
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  wrapper.loadMockedSerialBuffer(testData, sizeof(testData));
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char bfr[32];
  // read the first line
  size_t len = testNode.getLine(bfr);
  assertEqual(static_cast<uint16_t>(len), 7);
  for (size_t i=0; i<len; i++) assertEqual(bfr[i], "A line\n"[i]);
  // read the second line
  len = testNode.getLine(bfr);
  assertEqual(static_cast<uint16_t>(len), 13);
  for (size_t i=0; i<len; i++) assertEqual(bfr[i], "Another line\n"[i]);
  len = testNode.getLine(bfr);
  assertEqual(static_cast<uint16_t>(len), 8);
  for (size_t i=0; i<len; i++) assertEqual(bfr[i], "Rubbish\0"[i]);
};


// test getLine where newline last character in buffer
test(getLineNewLine) {
  char testData[] = "A line\n";
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  wrapper.loadMockedSerialBuffer(testData, sizeof(testData));
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  char bfr[32];
  size_t len = testNode.getLine(bfr);
  assertEqual(static_cast<uint16_t>(len), 7);
  for (size_t i=0; i<len; i++) assertEqual(bfr[i], "A line\n"[i]);
  assertEqual(bfr[6], "\n");
};


test(emptySerialBuffer) {
  char bfr[32];
  char testData[] = "A line\nAnother line\nRubbish";
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  wrapper.loadMockedSerialBuffer(testData, sizeof(testData));
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  // read the first line
  size_t len = testNode.getLine(bfr);
  assertEqual(static_cast<uint16_t>(len), 7);
  // empty buffer
  testNode.emptySerialBuffer();
  // read the next line which should be empty
  len = testNode.getLine(bfr);
  assertEqual(static_cast<uint16_t>(len), 0);
};


test(parseTime) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  SwarmNode testNode = SwarmNode(&displ, &wrapper);
  // example from SWARM Tile Manual
  char validTimeResponse[] = "$DT 20190408195123,V*41\n";
  char invalidTimeResponse[] = "Fuchsteufelswild\n";
  // return 0 if input cannot be interpreted as time response
  int res = testNode.parseTime(
    invalidTimeResponse, sizeof(invalidTimeResponse));
  assertEqual(res, 0);
  res = testNode.parseTime(validTimeResponse, sizeof(validTimeResponse));
  assertEqual(res, 1554753083);
}


// the following sets up the Serial for feedback and starts the test runner
// no need to touch
void setup() {
  Serial.begin(115200);
  delay(500);
  while(!Serial);
   // TestRunner::exclude("*");
   // TestRunner::include("formatMessageLong");
}

void loop() {
  aunit::TestRunner::run();
}
