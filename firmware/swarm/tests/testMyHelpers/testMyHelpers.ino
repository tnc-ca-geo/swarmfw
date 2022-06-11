// this fixes a bug in Aunit.h dependencies
#line 2 "testMyHelpers.ino"
#include <AUnitVerbose.h>
using namespace aunit;
// There is a problem in Arduino; the import from relative paths that
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.
#include "src/myHelpers.h"


test(popFromStack) {
  char bfr[] = "test1\ntesten2\ntest3";
  char outBfr[8];
  int bfrSize = strlen(bfr);
  int res = helpers::popFromStack(outBfr, bfr, bfrSize);
  int i = 0;
  assertEqual(res, 5);
  for(i=0; i<5; i++) { assertEqual("test1"[i], outBfr[i]); }
  assertEqual(outBfr[i], '\0');
  for(i=0; i<bfrSize-5-1-1; i++) { assertEqual("testen2\ntest3"[i], bfr[i]); };
  assertEqual(bfr[i], '3');
  assertEqual(bfr[i+1], '\0');
  res = helpers::popFromStack(outBfr, bfr, bfrSize);
  assertEqual(res, 7);
  for(i=0; i<7; i++) { assertEqual("testen2"[i], outBfr[i]); }
  assertEqual(outBfr[i], '\0');
  res = helpers::popFromStack(outBfr, bfr, bfrSize);
  assertEqual(res, 0);
  assertEqual(outBfr[0], '\0');
  for(i=0; i<bfrSize-5-7-2-1; i++) { assertEqual("test3"[i], bfr[i]); };
  assertEqual(bfr[i], '3');
  assertEqual(bfr[i+1], '\0');
  // test empty input stack
  char empty[255] = {0};
  res = helpers::popFromStack(outBfr, empty, 255);
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
  assertTrue(helpers::pushToStack(testStack, testBfr, strlen(testBfr), 32));
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
  assertTrue(helpers::pushToStack(testStack, testBfr, strlen(testBfr), 32));
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
  assertTrue(helpers::pushToStack(testStack, testBfr, strlen(testBfr), 32));
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
  assertTrue(helpers::pushToStack(testStack, testBfr, strlen(testBfr), 32));
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
  assertTrue(helpers::pushToStack(testStack, testBfr, strlen(testBfr), 32));
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
  assertFalse(helpers::pushToStack(testStack, testBfr, strlen(testBfr), 32));
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
  assertTrue(helpers::pushToStack(testStack, testBfr, strlen(testBfr), 32));
  for (i=0; i<strlen(expected); i++) { assertEqual(expected[i], testStack[i]); }
  assertEqual(testStack[i], '\0');
};


test(searchBfr) {
  char haystack[] = "Haystack with a needle in here.\n\0";
  assertEqual(helpers::searchBfr(haystack, sizeof(haystack), "needle", 5), 16);
  assertEqual(helpers::searchBfr(haystack, sizeof(haystack), "gold", 4), -1);
  assertEqual(helpers::searchBfr("short", 5, "to long", 7), -1);
  assertEqual(
   helpers::searchBfr("$TILE BOOT,RUNNING*49\n", 22, "$TILE BOOT,RUNNING", 18), 0);
  // Return -1 if a null pointer passed
  assertEqual(helpers::searchBfr(haystack, sizeof(haystack), NULL, 0), -1);
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
