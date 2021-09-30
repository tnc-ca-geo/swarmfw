// this fixes a bug in Aunit.h dependencies
#line 2 "testMessage.ino"

#include <AUnitVerbose.h>
using namespace aunit;

// There is a problem in Arduino; the import from relative paths that
// are not children of the sketch path is not supported.
// I am HACKING this with a symlink to the src directory for now.

#include "src/messages.h"

test(formatMessage) {
  Message message;
  MessageHelpers helpers;
  size_t len;
  char bfr[256];
  // test \0 terminated strings
  message.index = 5;
  message.timeStamp = 1632506435;
  memcpy(message.type, "SC\0", 3);
  message.channel = 230;
  memcpy(message.payload, "Test\0", 5);
  message.batteryVoltage = 3.5;
  len = helpers.formatMessage(message, bfr);
  assertEqual(static_cast<uint16_t>(len), 34);
  for (size_t i=0; i<len; i++) {
    assertEqual(bfr[i], "000005,1632506435,SC,230,Test,3.50"[i]);
  }
  // test not \0 terminated strings, that are limited in length
  message = {0};
  message.index = 5;
  message.timeStamp = 1632506435;
  memcpy(message.type, "SC", 2);
  message.channel = 1;
  memcpy(message.payload, "TestTest", 8);
  message.batteryVoltage = 1;
  len = helpers.formatMessage(message, bfr);
  assertEqual(static_cast<uint16_t>(len), 36);
  for (size_t i=0; i<len; i++) {
    assertEqual(bfr[i], "000005,1632506435,SC,1,TestTest,1.00"[i]);
  }
}

/*
 *  Make sure CV 50 output fits into struct
 */
test(testFormatCV50) {
  Message message;
  MessageHelpers helpers;
  char testMessage[] = "0+0+0.000+0+0+0.13+111.3+0.16+19.8+1.50+101.22+0.650"
    "+19.7+0.1+1.1+0-0.05+0.12+0.16";
  size_t len;
  char bfr[256];
  message.index = 3001;
  message.timeStamp = 1632506435;
  memcpy(message.type, "SC", 2);
  message.channel = 1;
  memcpy(message.payload, testMessage, sizeof(testMessage));
  message.batteryVoltage = .1;
  len = helpers.formatMessage(message, bfr);
  assertEqual(static_cast<uint16_t>(len), 110);
  for (size_t i=0; i<len; i++) {
    assertEqual(bfr[i],
      "003001,1632506435,SC,1,0+0+0.000+0+0+0.13+111.3+0.16+19.8+1.50+101.22"
      "+0.650+19.7+0.1+1.1+0-0.05+0.12+0.16,0.10"[i]);
  }
}

test(testNextScheduled) {
  MessageHelpers helpers;
  // current time 9/29/2021 17:55:58 GMT
  unsigned long int res = helpers.getNextScheduled(1632938158, 3600);
  // nextScheduled time will be 9/29/ 18:00:00 GMT
  // type explicite to satisfy overloaded assertEqual function
  unsigned long int expected = 1632938400;
  assertEqual(res, expected);
};

void setup() {
  Serial.begin(115200);
  delay(500);
  while(!Serial);
  // TestRunner::exclude("*");
  // TestRunner::include("emptySerialBuffer");
}

void loop() {
  aunit::TestRunner::run();
}
