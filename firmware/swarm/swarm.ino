/*
 * Firmware for SWARM evaluation kit with Adafruit ESP32 Huzzah Feather and
 * SDI-12 sensor (eventually)
 * 
 * TODOs:
 * - investigate use of modules accross projects
 * 
 * Current goals: 
 * - proof of concept and regular sending of messages
 * - simple as possible
 * - do not wrangle control from the SWARM tile
 * - we can use a very simple synchronous loop for now
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
  

// a wrapper around the display handling
DisplayWrapper dspl = DisplayWrapper();
// this is the serial interface communicationg with the tile, not the 
// one for debugging
SerialWrapper srl = SerialWrapper(&Serial2, 115200);
// last value indicates dev mode deleting all old messages on restart
// TODO: set to false for production
SwarmNode tile = SwarmNode(&dspl, &srl, true);
// deals with SDI communication
SDI12Measurement measurement = SDI12Measurement();
// message types and helpers
MessageHelpers helpers;
Message message;

// Sending every hour (3600s) meets the monthly included rate of 720 message
const unsigned long int sendFrequencyInSeconds = 3600;
// time polling frequency, set on the tile for unsolicitated time messages
// this determines the precision of the send schedule but also power consumption
const unsigned long int tileTimeFrequency = 120;
// the watchdog reset time should be a multiple of the tileTimeFrequency since it
// is blocking
const unsigned long int watchDogResetTime = 5 * tileTimeFrequency;
// index of message since last restart to keep track of restarts by the
// watchdog
unsigned long int messageCounter = 0;
// by setting nextScheduled = 0 sending will start right after restart, schedule
// will start for the next message, good for testing
unsigned long int nextScheduled = 0;

/*
 *  Measure battery/system voltage Adafruit Feather HUZZAH
 */
float getBatteryVoltage() {
  return analogRead(A13) * 2 * 3.3/4096;
};

/* 
 * Calculate the next scheduled time (in unix epoch time)
 */
unsigned long int getNextScheduled(unsigned long int timeStamp, int interval) {
  return static_cast<int>(
    static_cast<double>(
      timeStamp + interval)/static_cast<double>(interval)) * interval;
};

/*
 * Setup routine
 */
void setup() {
  char bfr[255];
  size_t len;
  // Wait for Serial and hope that it is ready after a second
  // Keep going if Serial is not available for debugging
  Serial.begin(115200);
  delay(1000);
  // Initialize display and add some boiler plate
  dspl.begin();
  dspl.printBuffer("SWARM sensor node\n");
  dspl.printBuffer("falk.schuetzenmeister@tnc.org\n");
  dspl.printBuffer("September 2021\n");
  delay(5000);
  // print sensor information
  dspl.clearDisplay();
  dspl.setCursor(0,0);
  len = measurement.getName(bfr);
  dspl.printBuffer(bfr, len);
  dspl.printBuffer("\n");
  len = measurement.getInfo(bfr);
  dspl.printBuffer(bfr, len);
  delay(5000);
  dspl.clearDisplay();
  dspl.setCursor(0, 0);
  // initialize tile and wait until time has been obtained successfully 
  tile.begin(tileTimeFrequency);
  // and off we go
  dspl.printBuffer("\nTILE INIT SUCCESSFUL\n");
}

void loop() {
  size_t len;
  char bfr[192];
  // control when loop advances, SWARM tile controls timing
  unsigned long tileTime = tile.waitForTimeStamp();
  // send message if scheduled
  if (tileTime > nextScheduled) {
    message = {0};
    // assemble message
    message.index = messageCounter;   
    message.timeStamp = tileTime;
    memcpy(message.type, "CS\0", 3);
    message.channel = 0;
    // get measurement from SDI-12 device on address 0
    len = measurement.getPayload('0', message.payload);
    // make sure message is \0 terminated
    message.payload[len] = 0;
    message.batteryVoltage = getBatteryVoltage();
    // format message for sending
    len = helpers.formatMessage(message, bfr);
    // give some user feedback
    dspl.clearDisplay();
    dspl.setCursor(0, 0);
    dspl.printBuffer("SENDING AT ");
    dspl.print(message.timeStamp);
    if (Serial) { 
      Serial.write(bfr, len);
      Serial.println();
    }
    // send to SWARM tile
    tile.sendMessage(bfr, len);
    // schedule next message
    nextScheduled = getNextScheduled(tileTime, sendFrequencyInSeconds);
    // increase counter
    messageCounter++;
  }
}
