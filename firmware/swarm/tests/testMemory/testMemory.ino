/*
 * Test persistent memory
 * 
 * This test is hardware dependant
 */

// this fixes a bug in Aunit.h dependencies
#line 2 "testMessage.ino"

#include <AUnitVerbose.h>
using namespace aunit;

// There is a problem in Arduino; the import from relative paths that
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.

#include "src/memory.h"

PersistentMemory mem = PersistentMemory();

test(testMemory) {
  mem.writeFrequency(111111);
  assertEqual(static_cast<int>(mem.readFrequency()), 111111);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  while(!Serial);
  // TestRunner::exclude("*");
  // TestRunner::include("formatMessage");
}

void loop() {
  aunit::TestRunner::run();
}
