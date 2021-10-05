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
 * - batteryVoltage: battery voltage 
 * - array of up to 5
 *    - channel: use within message type, e.g. SDI 12 channel 
 *    - payload: use within message type, e.g. SDI 12 message
 *  
 * Entire message must fit into 192 characters
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

#define BATTERY_PIN A13

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


// Sending every hour (3600s) meets the monthly included rate of 720 message
const unsigned long int sendFrequencyInSeconds = 3600;
// time polling frequency, set on SWARM tile for unsolicitated time messages
// determines precision of send schedule but also power consumption
const unsigned long int tileTimeFrequency = 10;
// watchdog reset time should be a multiple of the tileTimeFrequency since it
// is blocking
const unsigned long int watchDogResetTime = 5 * tileTimeFrequency;
// channels available, testing values '0-z' for now using characters
char availableChannels[10] = {0};
int numberOfChannels = 0;
// index of message since last restart to keep track of restarts
int messageCounter = 0;
// by setting nextScheduled = 0 sending will start after restart, schedule
// will start for the next message, good for testing
unsigned long int nextScheduled = 0;

/*
 *  Measure battery/system voltage Adafruit Feather HUZZAH
 */
float getBatteryVoltage() {
  return analogRead(BATTERY_PIN) * 2 * 3.3/4096;
};

/*
 *  Collect data and construct message
 */
size_t getMessage(char *bfr, int idx, unsigned long tme) {
  Message message = {0};
  size_t len = 0;
  // message index
  message.index = idx;   
  // time
  message.timeStamp = tme;
   // get battery voltage
  message.batteryVoltage = getBatteryVoltage();
  // message type
  memcpy(message.type, "SC\0", 3);
  // payloads
  message.payloads[0].channel = 48;
  // get measurement from SDI-12 device on address '0'/48
  len = measurement.getPayload(48, message.payloads[0].payload);
  // format message for sending
  return helpers.formatMessage(message, bfr);
}

/*
 *  Deal with incoming messages
 */
void processIncoming() {};

/*
 *  Setup
 */
void setup() {
  // use these as needed
  char bfr[255];
  size_t len;
  // Start Serial for debugging, hope it is ready after a second
  // Keep going if Serial not available
  Serial.begin(115200);
  // Initialize display and add some boiler plate
  dspl.begin();
  dspl.printBuffer(
    "SWARM sensor node\nfalk.schuetzenmeister@tnc.org\nOctober 2021");
  delay(2000);
  dspl.resetDisplay();
  sprintf(
    bfr, "Reporting frequency:\n\n%d seconds", sendFrequencyInSeconds);
  dspl.printBuffer(bfr);
  delay(1000);
  // print sensor information
  dspl.resetDisplay();
  len = measurement.getName(bfr);
  dspl.printBuffer(bfr, len);
  dspl.printBuffer("\n");
  numberOfChannels = measurement.getChannels(availableChannels);
  sprintf(bfr, "%d SDI12 channel(s) detected\n", numberOfChannels);
  dspl.printBuffer(bfr);
  delay(1000);
  dspl.resetDisplay();
  for (size_t i=0; i<numberOfChannels; i++) {
    len = measurement.getInfo(bfr, availableChannels[i]);
    dspl.print(availableChannels[i]);
    dspl.print(':');
    dspl.printBuffer(bfr, len);
  };
  delay(2000);
  dspl.resetDisplay();
  // initialize tile and wait until time has been obtained by GPS
  tile.begin(tileTimeFrequency);
  // off we go
  dspl.printBuffer("TILE INIT SUCCESSFUL\n");
  Serial.write(availableChannels, 10);
  Serial.println();
} 


void loop() {
  size_t len;
  char bfr[32];
  char messageBfr[192];
  // control when loop advances, SWARM tile controls timing
  unsigned long int tileTime = tile.waitForTimeStamp();
  /* 
   *  1. process incoming messages
   */
  processIncoming();
  /* 
   *  2. send messages according schedule
   */
  if (tileTime > nextScheduled) {
    len = getMessage(messageBfr, messageCounter, tileTime);
    sprintf(bfr, "SENDING AT %d", tileTime);
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
  /* 
   *  3. power management
   */
}
