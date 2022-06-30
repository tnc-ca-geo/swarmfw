#include "src/newSdi12Wrapper.h"


Sdi12Wrapper wrapper = Sdi12Wrapper();
NewSdi12Measurement measurement = NewSdi12Measurement(&wrapper);
boolean sensorFlag = false;
boolean infoFlag = false;
boolean measurementFlag = false;
boolean block = false;


bool innerLoop() {
  char bfr[255] = {0};
  size_t len = 0;
  measurement.loopOnce();
  len = measurement.getResponse(bfr);
  if (len) {
    block = false;
    Serial.print(bfr);
  }
  if (!sensorFlag && !block) {
    Serial.println("\nrequest sensors");
    measurement.requestSensors();
    sensorFlag = true;
    block = true;
  }
  if (!infoFlag && !block) {
    Serial.println("\nrequest info");
    measurement.requestInfo('3');
    infoFlag = true;
    block = true;
  }
  if (!measurementFlag && !block) {
    Serial.println("\nrequest measurement");
    measurement.requestMeasurement('3');
    measurementFlag = true;
    block = true;
  }
  if (measurementFlag && infoFlag && sensorFlag && !block) return false; 
  else return true;
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  while(!Serial) {};
  while (innerLoop());
  Serial.println("End of program");
}


void loop() {}
