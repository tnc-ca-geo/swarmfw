#include "src/newSdi12Wrapper.h"


newSdi12Measurement measurement = newSdi12Measurement();
boolean infoFlag = false;
boolean measurementFlag = false;
boolean block = false;


void setup() {
  Serial.begin(115200);
  delay(1000);
  while(!Serial) {};
}


void loop() {
  char command[] = "3I!";
  char bfr[255] = {0};
  measurement.loopOnce();
  measurement.readResponse(bfr);
  Serial.print(bfr);
  if(bfr[0] != 0) { 
    Serial.println(); 
    block = false;    
  };
  if (!infoFlag && !block) {
    Serial.println("\nrequest info");
    measurement.getInfo('3');
    infoFlag = true;
    block = true;
  }
  if (!measurementFlag && !block) {
    Serial.println("\nrequest measurement");
    measurement.getMeasurement('3');
    measurementFlag = true;
    block = true;
  }
}
