#include "src/sdi12Wrapper.h"

SDI12Measurement measurement = SDI12Measurement();
char responseBfr[256] = { 0 };

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Hello SDI-12!");
  Serial.println();
};

/*
 * We are testing here that the script does not hangs up
 * when sensor removed.
 */
void loop() {
  delay(1000);
  Serial.println("Reading SDI-12");
  Serial.println("==============");
  size_t len = measurement.getPayload(responseBfr, '3');
  Serial.print("Message length: ");
  Serial.println(len);
  Serial.print("Message payload: ");
  Serial.write(responseBfr);
  Serial.println();
  Serial.println();
  Serial.println("Moving on!");
  Serial.println();
};
