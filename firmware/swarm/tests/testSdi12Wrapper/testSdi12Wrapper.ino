// this fixes a bug in Aunit.h dependencies
#line 2 "testMessage.ino"

#include <AUnitVerbose.h>
using namespace aunit;

// There is a problem in Arduino; the import from relative paths that
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.

#include "src/sdi12Wrapper.h"


SDI12Measurement sdi12 = SDI12Measurement();


test(parseResponse) {
  char testResponse[] = "510014";
  sdi12.parseResponse(testResponse, 6);
  // assert(sdi12.lastSensor == '5');
  // assert(sdi12.retrievalTime > millis());
  assert(sdi12.numberOfValues == 14);
}


test(countValues) {
  char example[16];
  memcpy(example, "+1+1+2-1+6.677", 15);
  assert(sdi12.countValues(example, 15) == 5);
}


/*
 * Test the behavior of the Meter sensor (extremely hardware dependent)
 */
test(testChannel) {
  char bfr[256];
  size_t len = sdi12.getPayload(bfr, '3');
  Serial.write(bfr, len);
  Serial.println();
  len = sdi12.getPayload(bfr, '4');
  Serial.write(bfr, len);
  Serial.println();
  len = sdi12.getPayload(bfr, '5');
  Serial.write(bfr, len);
  Serial.println();
  //Serial.write(bfr, len);
}


// this is highly hardware dependent
test(nonBlockingSend) {
  sdi12.nonBlockingSend("5C!", 3);
  while (!sdi12.responseReady) {
    sdi12.loop_once();
  }
  Serial.write(sdi12.responseBfr, strlen(sdi12.responseBfr));
}


/*
 *  Test non-existent sensor query bailout
 */
test(takeMeasurementFail) {
  sdi12.takeMeasurement('k');
  while (!sdi12.measurementReady) {
    sdi12.loop_once();
  }
  Serial.print("RESULT: ");
  Serial.write(sdi12.measurementBfr);
  Serial.println();
}


test(takeMeasurement) {
  sdi12.takeMeasurement('3');
  while (!sdi12.measurementReady) {
    sdi12.loop_once();
  }
  Serial.print("RESULT: ");
  Serial.write(sdi12.measurementBfr, strlen(sdi12.measurementBfr));
  Serial.println();
}


void setup() {
  Serial.begin(115200);
  delay(500);
  while(!Serial);
  // TestRunner::exclude("*");
  // TestRunner::include("takeMeasurement");
  // TestRunner::include("takeMeasurementFail");
}


void loop() {
  aunit::TestRunner::run();
}
