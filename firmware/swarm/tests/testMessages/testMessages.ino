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
  message.payloads[0].channel = 230;
  memcpy(message.payloads[0].payload, "Test\0", 5);
  message.batteryVoltage = 3.5;
  len = helpers.formatMessage(message, bfr);
  assertEqual(static_cast<uint16_t>(len), 34);
  for (size_t i=0; i<len; i++) {
    assertEqual(bfr[i], "000005,1632506435,3.50,SC,230,Test"[i]);
  }
  // test not \0 terminated strings, that are limited in length
  message = {0};
  message.index = 5;
  message.timeStamp = 1632506435;
  memcpy(message.type, "SC", 2);
  message.payloads[0].channel = 1;
  memcpy(message.payloads[0].payload, "TestTest", 8);
  message.batteryVoltage = 1;
  len = helpers.formatMessage(message, bfr);
  for (size_t i=0; i<len; i++) {
    assertEqual(bfr[i], "000005,1632506435,1.00,SC,1,TestTest"[i]);
  }
}

test(formatMessageMultiple) {
  Message message;
  MessageHelpers helpers;
  size_t len;
  char bfr[256];
  message.index = 7;
  message.timeStamp = 20;
  message.batteryVoltage = 3.5;
  memcpy(message.type, "SC", 2);
  message.payloads[0].channel = 48;
  memcpy(message.payloads[0].payload, "hello world", 12);
  message.payloads[1].channel = 50;
  memcpy(message.payloads[1].payload, "hollow moon", 12);
  len = helpers.formatMessage(message, bfr);
  assertEqual(static_cast<int>(len), 55);
  for (size_t i=0; i<len; i++) {
    assertEqual(bfr[i], "000007,0000000020,3.50,SC,48,hello world,50,hollow moon"[i]);
  }
}

test(formatMessageToLong) {
  Message message;
  MessageHelpers helpers;
  size_t len;
  char bfr[256];
  message.index = 7;
  message.timeStamp = 20;
  message.batteryVoltage = 3.5;
  memcpy(message.type, "SC", 2);
  message.payloads[0].channel = 48;
  memcpy(message.payloads[0].payload, "hello world", 12);
  message.payloads[1].channel = 50;
  memcpy(message.payloads[1].payload, "hollow moon", 12);
  message.payloads[2].channel = 53;
  // copy only the first 100 characters
  memcpy(message.payloads[2].payload,
    "This message is way to long for a SWARM payload and we have to see whether it works. "
    "We also have to make tough decisions. But it still fits into the message.", 150);
  len = helpers.formatMessage(message, bfr);
  assertEqual(static_cast<int>(len), 192);
  for (size_t i=0; i<len; i++) {
    assertEqual(bfr[i], 
      "000007,0000000020,3.50,SC,48,hello world,50,hollow moon,53,This message is way to long "
      "for a SWARM payload and we have to see whether it works. We also have to make tough "
      "decisions. But it s.."[i]);
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
  message.payloads[0].channel = 1;
  memcpy(message.payloads[0].payload, testMessage, sizeof(testMessage));
  message.batteryVoltage = .1;
  len = helpers.formatMessage(message, bfr);
  assertEqual(static_cast<uint16_t>(len), 110);
  for (size_t i=0; i<len; i++) {
    assertEqual(bfr[i],
      "003001,1632506435,0.10,SC,1,0+0+0.000+0+0+0.13+111.3+0.16+19.8+1.50+101.22"
      "+0.650+19.7+0.1+1.1+0-0.05+0.12+0.16"[i]);
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
  // TestRunner::include("formatMessage");
}

void loop() {
  aunit::TestRunner::run();
}
