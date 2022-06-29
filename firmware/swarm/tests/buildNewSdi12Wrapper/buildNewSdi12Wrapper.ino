#include "src/newSdi12Wrapper.h"


Sdi12Wrapper wrapper = Sdi12Wrapper();
NewSdi12Measurement measurement = NewSdi12Measurement(&wrapper);
boolean infoFlag = false;
boolean measurementFlag = false;
boolean block = false;


void setup() {
  Serial.begin(115200);
  delay(1000);
  while(!Serial) {};
}


void loop() {
  char bfr[255] = {0};
  measurement.loopOnce();
  measurement.getResponse(bfr);
  Serial.print(bfr);
  if(bfr[0] != 0) {
    Serial.println();
    block = false;
  };
  if (!infoFlag && !block) {
    Serial.println("\nrequest info");
    measurement.requestInfo('3');
    infoFlag = true;
    block = true;
  };
  if (!measurementFlag && !block) {
    Serial.println("\nrequest measurement");
    measurement.requestMeasurement('3');
    measurementFlag = true;
    block = true;
  }
}
