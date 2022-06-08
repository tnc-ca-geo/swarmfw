// this fixes a bug in Aunit.h dependencies
#line 2 "testNewSwarmNode.ino"
#include <AUnitVerbose.h> 
using namespace aunit;
// There is a problem in Arduino; the import from relative paths that
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.
#include "src/newSwarmNode.h"


test(checkNmeaChecksum) {
  char bfr[] = "$test*16";
  assertTrue(checkNmeaChecksum(bfr, sizeof(bfr)));
  char incorrectBfr[] = "$test*22";
  assertFalse(checkNmeaChecksum(incorrectBfr, sizeof(incorrectBfr)));
  char noNmeaBfr[] = "nonsense";
  assertFalse(checkNmeaChecksum(noNmeaBfr, sizeof(noNmeaBfr)));
};


test(cleanCommand) {
  char bfr[8];
  char expected[] = "$RS*01\n";
  size_t len = sizeof(expected);
  assertEqual(cleanCommand("$RS", 3, bfr), len-1);
  for (size_t i=0; i<len; i++) assertEqual(bfr[i], expected[i]);
  // check null termination, I think that is already implied in the above
  assertEqual(expected[7], '\0');
};


test(formatMessage) {
  char expected[] = "$TD HD=86400,68656c6c6f";
  char res[255] = {};
  size_t len = formatMessage("hello", 5, res);
  for (int i; i<len; i++) assertEqual(expected[i], res[i]);
  assertEqual(res[len], '\0');
};


test(nmeaChecksum) {
  assertEqual(nmeaChecksum("test", 4), 0x16);
};


test(parseTime) {
  // example from SWARM Tile Manual
  char validTimeResponse[] = "$DT 20190408195123,V*41\n";
  char invalidTimeResponse[] = "Fuchsteufelswild\n";
  // return 0 if input cannot be interpreted as time response
  int res = parseTime(invalidTimeResponse, sizeof(invalidTimeResponse));
  assertEqual(res, 0);
  res = parseTime(validTimeResponse, sizeof(validTimeResponse));
  assertEqual(res, 1554753083);
};


test(parseTime_TimeFlag) {
  char invalidTimeResponse[] = "$DT 20190408195123,N*59\n";
  int res = parseTime(invalidTimeResponse, sizeof(invalidTimeResponse));
  assertEqual(res, 0);
};


test(popFromStack) {
  char bfr[] = "test1\ntesten2\ntest3";
  char outBfr[8];
  int bfrSize = strlen(bfr);
  int res = popFromStack(outBfr, bfr, bfrSize);
  int i = 0;
  assertEqual(res, 5);
  for(i=0; i<5; i++) { assertEqual("test1"[i], outBfr[i]); }
  assertEqual(outBfr[i], '\0');
  for(i=0; i<bfrSize-5-1-1; i++) { assertEqual("testen2\ntest3"[i], bfr[i]); };
  assertEqual(bfr[i], '3');
  assertEqual(bfr[i+1], '\0');
  res = popFromStack(outBfr, bfr, bfrSize);
  assertEqual(res, 7);
  for(i=0; i<7; i++) { assertEqual("testen2"[i], outBfr[i]); }
  assertEqual(outBfr[i], '\0');
  res = popFromStack(outBfr, bfr, bfrSize);
  assertEqual(res, 0);
  assertEqual(outBfr[0], '\0');
  for(i=0; i<bfrSize-5-7-2-1; i++) { assertEqual("test3"[i], bfr[i]); };
  assertEqual(bfr[i], '3');
  assertEqual(bfr[i+1], '\0');
  // test empty input stack
  char empty[255] = {0};
  res = popFromStack(outBfr, empty, 255);
  assertEqual(res, 0);
  assertEqual(outBfr[0], '\0');
};


test(pushToStack) {
  char testStack[32] =  {0};
  char testString[] = "first\nsecond\nthird\n";
  char testBfr[] = "fourth";
  char expected[] = "first\nsecond\nthird\nfourth\n";
  int i = 0;
  memcpy(testStack, testString, strlen(testString));
  assertTrue(pushToStack(testStack, testBfr, strlen(testBfr)));
  for (i=0; i<strlen(expected); i++) { assertEqual(expected[i], testStack[i]); }
  assertEqual(testStack[i-1], '\n');
  assertEqual(testStack[i], '\0');
};

// test with empty stack;
test(pushToStack_empty) {
  char testStack[32] =  {0};
  char testBfr[] = "fourth";
  char expected[] = "fourth\n";
  int i = 0;
  assertTrue(pushToStack(testStack, testBfr, strlen(testBfr)));
  for (i=0; i<strlen(expected); i++) { assertEqual(expected[i], testStack[i]); }
  assertEqual(testStack[i], '\0');
};


// test with leading \n;
test(pushToStack_leadingLineBreak) {
  char testStack[32] =  {0};
  char testString[] = "\nfirstsecondthird";
  char testBfr[] = "fourth";
  char expected[] = "\nfourth\nfirstsecondthird";
  int i = 0;
  memcpy(testStack, testString, strlen(testString));
  assertTrue(pushToStack(testStack, testBfr, strlen(testBfr)));
  for (i=0; i<strlen(expected); i++) { assertEqual(expected[i], testStack[i]); }
  assertEqual(testStack[i], '\0');
};


// test with unterminated stack containing \n somewhere;
test(pushToStack_notTerminated) {
  char testStack[32] =  {0};
  char testString[] = "first\nsecond\nthird";
  char testBfr[] = "fourth";
  char expected[] = "first\nsecond\nfourth\nthird";
  int i = 0;
  memcpy(testStack, testString, strlen(testString));
  assertTrue(pushToStack(testStack, testBfr, strlen(testBfr)));
  for (i=0; i<strlen(expected); i++) { assertEqual(expected[i], testStack[i]); }
  assertEqual(testStack[i], '\0');
};


// test with no \n anywhere;
test(pushToStack_notTerminatedNoLineBreak) {
  char testStack[32] =  {0};
  char testString[] = "firstsecondthird";
  char testBfr[] = "fourth";
  char expected[] = "fourth\nfirstsecondthird";
  int i = 0;
  memcpy(testStack, testString, strlen(testString));
  assertTrue(pushToStack(testStack, testBfr, strlen(testBfr)));
  for (i=0; i<strlen(expected); i++) { assertEqual(expected[i], testStack[i]); }
  assertEqual(testStack[i], '\0');
};


// test overflow;
test(pushToStack_overflow) {
  char testStack[32] =  {0};
  char testString[] = "firstsecondthird";
  char testBfr[] = "fourthfourthfourthfourth";
  char expected[] = "firstsecondthird";
  int i = 0;
  memcpy(testStack, testString, strlen(testString));
  // overflow argument required
  assertFalse(pushToStack(testStack, testBfr, strlen(testBfr), 32));
  for (i=0; i<strlen(expected); i++) { assertEqual(expected[i], testStack[i]); }
  assertEqual(testStack[i], '\0');
};


// test trailing line break;
test(pushToStack_trailingLineBreakInBuffer) {
  char testStack[32] =  {0};
  char testString[] = "first\nsecond";
  char testBfr[] = "fourth\n\n";
  char expected[] = "first\nfourth\nsecond";
  int i = 0;
  memcpy(testStack, testString, strlen(testString));
  // overflow argument required
  assertTrue(pushToStack(testStack, testBfr, strlen(testBfr), 32));
  for (i=0; i<strlen(expected); i++) { assertEqual(expected[i], testStack[i]); }
  assertEqual(testStack[i], '\0');
};


test(searchBfr) {
  char haystack[] = "Haystack with a needle in here.\n\0";
  assertEqual(searchBfr(haystack, sizeof(haystack), "needle", 5), 16);
  assertEqual(searchBfr(haystack, sizeof(haystack), "gold", 4), -1);
  assertEqual(searchBfr("short", 5, "to long", 7), -1);
  assertEqual(
   searchBfr("$TILE BOOT,RUNNING*49\n", 22, "$TILE BOOT,RUNNING", 18), 0);
  // Return -1 if a null pointer passed
  assertEqual(searchBfr(haystack, sizeof(haystack), NULL, 0), -1);
};


test(toHexString) {
  char testBfr[] = "exciting,test";
  char expected[] = "6578636974696e672c74657374";
  char outBfr[255];
  size_t len = toHexString(testBfr, 13, outBfr);
  assertEqual(static_cast<uint16_t>(len), 26);
  for (size_t i=0; i<len; i++) assertEqual(outBfr[i], expected[i]);
}


test(validateResponse) {
  char valid[] = "$DT OK*34\n";
  assertTrue(validateResponse(valid, sizeof(valid)));
  char invalid[] = "$DT OK*54\n";
  assertFalse(validateResponse(invalid, sizeof(invalid)));
  char incomplete[] = "test";
  assertFalse(validateResponse(incomplete, sizeof(incomplete)));
};


test(validateTime) {
  struct tm tt{0};
  strptime("2037-12-31 23:59:59", "%Y-%m-%dT %H:%M:%S", &tt);
  assertTrue(validateTime(tt));
  tt.tm_year = 10;
  assertFalse(validateTime(tt));
  strptime("2037-12-31 23:59:59", "%Y-%m-%dT %H:%M:%S", &tt);
  tt.tm_mon = 12;
  assertFalse(validateTime(tt));
  strptime("2037-12-31 23:59:59", "%Y-%m-%dT %H:%M:%S", &tt);
  tt.tm_mday = 0;
  assertFalse(validateTime(tt));
  strptime("2037-12-31 23:59:59", "%Y-%m-%dT %H:%M:%S", &tt);
  tt.tm_mday = 32;
  assertFalse(validateTime(tt));
  strptime("2037-12-31 23:59:59", "%Y-%m-%dT %H:%M:%S", &tt);
  tt.tm_hour = 24;
  assertFalse(validateTime(tt));
  strptime("2037-12-31 23:59:59", "%Y-%m-%dT %H:%M:%S", &tt);
  tt.tm_min = 61;
  assertFalse(validateTime(tt));
  strptime("2037-12-31 23:59:59", "%Y-%m-%dT %H:%M:%S", &tt);
  tt.tm_sec = 62;
  assertFalse(validateTime(tt));
};


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


test(NewSwarmNode_getLastCommand) {
  MockedSerialWrapper wrapper = MockedSerialWrapper();
  NewSwarmNode testNode = NewSwarmNode(&wrapper);
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
