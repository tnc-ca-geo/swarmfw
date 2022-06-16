// this fixes a bug in Aunit.h dependencies
#line 2 "testNewSdi12Wrapper.ino"
#include <AUnitVerbose.h> 
using namespace aunit;
// There is a problem in Arduino; the import from relative paths that
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.
#include "src/newSdi12Wrapper.h"




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
