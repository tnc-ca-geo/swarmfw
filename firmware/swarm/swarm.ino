/*
 * Firmware for SWARM evaluation kit with Adafruit ESP32 Huzzah Feather and
 * SDI-12 sensor
 * 
 * TODOs:
 * - investigate use of modules accross projects
 * 
 * Current goals: 
 * - proof of concept and regular sending of messages
 * - do not wrangle control from SWARM tile
 * - use simple synchronous loop for now
 * 
 * Message format spec:
 * - index: message # after last restart
 * - timeStamp: unix epoch
 * - type:
 *   - SI, SDI 12 sensor information
 *   - SC, SDI 12 message obtained with a C! command
 * - channel: use within message type, e.g. SDI 12 channel 
 * - payload: use within message type, e.g. SDI 12 message
 * - batteryVoltage: battery voltage 
 * 
 * Falk Schuetzenmeister, falk.schuetzenmeister@tnc.org
 * September 2021
 * 
 */
// used for watchdog functionality
// #include <esp_task_wdt.h>

// my libraries
#include "src/displayWrapper.h"
#include "src/serialWrapper.h"
#include "src/swarmNode.h"
#include "src/sdi12Wrapper.h"
#include "src/messages.h"
  

// Wrapper around the OLED display
DisplayWrapper dspl = DisplayWrapper();
// serial interface communicationg with the SWARM tile, NOT for debugging
SerialWrapper srl = SerialWrapper(&Serial2, 115200);
// third arguments indicates dev mode deleting unsent messages on restart
SwarmNode tile = SwarmNode(&dspl, &srl, true);
// SDI12 communication
SDI12Measurement measurement = SDI12Measurement();
// message types and helpers
MessageHelpers helpers;
Message message;


// Sending every hour (3600s) meets the monthly included rate of 720 message
const unsigned long int sendFrequencyInSeconds = 60; // 3600;
// time polling frequency, set on SWARM tile for unsolicitated time messages
// determines precision of send schedule but also power consumption
const unsigned long int tileTimeFrequency = 10;
// watchdog reset time should be a multiple of the tileTimeFrequency since it
// is blocking
const unsigned long int watchDogResetTime = 5 * tileTimeFrequency;
// index of message since last restart to keep track of restarts
unsigned long int messageCounter = 0;
// by setting nextScheduled = 0 sending will start after restart, schedule
// will start for the next message, good for testing
unsigned long int nextScheduled = 0;

/*
 *  Measure battery/system voltage Adafruit Feather HUZZAH
 */
float getBatteryVoltage() {
  return analogRead(A13) * 2 * 3.3/4096;
};


/*
 * Setup
 */
void setup() {
  char bfr[255];
  size_t len;
  // Start Serial for debugging, hope it is ready after a second
  // Keep going if Serial not available
  Serial.begin(115200);
  delay(1000);
  // Initialize display and add some boiler plate
  dspl.begin();
  dspl.printBuffer("SWARM sensor node\n");
  dspl.printBuffer("falk.schuetzenmeister@tnc.org\n");
  dspl.printBuffer("October 2021\n");
  delay(2000);
  // print sensor information
  dspl.clearDisplay();
  dspl.setCursor(0,0);
  len = measurement.getName(bfr);
  dspl.printBuffer(bfr, len);
  dspl.printBuffer("\n");
  len = measurement.getInfo(bfr);
  dspl.printBuffer(bfr, len);
  delay(2000);
  dspl.clearDisplay();
  dspl.setCursor(0, 0);
  // initialize tile and wait until time has been obtained by GPS 
  tile.begin(tileTimeFrequency);
  // off we go
  dspl.printBuffer("\nTILE INIT SUCCESSFUL\n");
}

void loop() {
  size_t len;
  char bfr[32];
  char messageBfr[192];
  // control when loop advances, SWARM tile controls timing
  unsigned long tileTime = tile.waitForTimeStamp();
  // send message if scheduled
  if (tileTime > nextScheduled) {
    message = {0};
    // assemble message
    message.index = messageCounter;   
    message.timeStamp = tileTime;
    memcpy(message.type, "SC\0", 3);
    message.channel = 0;
    // get measurement from SDI-12 device on address 0
    len = measurement.getPayload('0', message.payload);
    // make sure message is \0 terminated
    message.payload[len] = 0;
    message.batteryVoltage = getBatteryVoltage();
    // format message for sending
    len = helpers.formatMessage(message, messageBfr);
    // give some user feedback
    dspl.printBuffer("SENDING AT ");
    sprintf(bfr, "%d", message.timeStamp);
    dspl.printBuffer(bfr);
    if (Serial) { 
      Serial.write(messageBfr, len);
      Serial.println();
    }
    // send to SWARM tile
    tile.sendMessage(messageBfr, len);
    // schedule next message
    nextScheduled = helpers.getNextScheduled(tileTime, sendFrequencyInSeconds);
    // increase counter
    messageCounter++;
  }
}
