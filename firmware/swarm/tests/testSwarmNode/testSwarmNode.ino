// this fixes a bug in Aunit.h dependencies
#line 2 "testSwarmNode.ino"

#include <AUnitVerbose.h>
// this is a little bit a problem in Arduino, the import from relative paths that 
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.
#include "src/swarmNode.h"

SwarmNode testObj = SwarmNode();

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
}

test(parseTime) {
  // example from SWARM Tile Manual 
  char validTimeResponse[] = "$DT 20190408195123,V*41\n";
  char invalidTimeResponse[] = "Fuchsteufelswild\n";
  // return -1 if input cannot be interpreted as time response
  assertEqual(testObj.parseTime(invalidTimeResponse, sizeof(invalidTimeResponse)), -1);
  assertEqual(testObj.parseTime(validTimeResponse, sizeof(validTimeResponse)), 1554753083);
}


// the following sets up the Serial for feedback and starts the test runner
// no need to touch
void setup() {
  delay(1000);
  Serial.begin(9600);
  while(!Serial);
}

void loop() {
  aunit::TestRunner::run();
}
