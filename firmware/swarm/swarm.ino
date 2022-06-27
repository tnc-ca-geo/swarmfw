/*
 * Firmware for SWARM evaluation kit with Adafruit ESP32 Huzzah Feather and
 * SDI-12 sensor
 *
 * TODOs:
 * - investigate use of modules accross different projects
 *
 * Current goals:
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
 * Pins used (TODO: Complete!)
 *
 *  13   battery Voltage measurement (used)
 *  14   BUTTON C (used)
 *  15   BUTTON A (used)
 *  16   SWARM connection, RX (used)
 *  17   SWARM connection, TX (used)
 *  21   SDI data (used)
 *  32   BUTTON B (used)
 *
 * Falk Schuetzenmeister, falk.schuetzenmeister@tnc.org
 * October 2021
 *
 */
/* used for watchdog functionality
 * esp32 seems to have a good default watchdog functionality
 * TODO: explore further
#include <esp_task_wdt.h> */
#include <WiFi.h>
#include "esp_wifi.h"

// my libraries
#include "src/displayWrapper.h"
#include "src/serialWrapper.h"
#include "src/swarmNode.h"
#include "src/sdi12Wrapper.h"
#include "src/messages.h"
#include "src/memory.h"
#include "src/setup.h"

#define BATTERY_PIN A13
#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
#define DEFAULT_SEND_FREQUENCY 3600 // use an hour as default

// Wrapper around the OLED display
DisplayWrapper dspl = DisplayWrapper();
// serial interface communicationg with the SWARM tile, NOT for debugging
SerialWrapper srl = SerialWrapper(&Serial2, 115200);
// third arguments indicates dev mode deleting unsent messages on restart
SwarmNode tile = SwarmNode(&dspl, &srl, false);
// SDI12 communication
SDI12Measurement measurement = SDI12Measurement();
// Configuration storage
PersistentMemory mem = PersistentMemory();
// message types and helpers
MessageHelpers helpers;
SetupHelpers stp;

// Sending every hour (3600s) meets the monthly included rate of 720 message
// arithmetic with millis() needs unsigned long
unsigned long measurementFrequencyS = DEFAULT_SEND_FREQUENCY;
// time polling frequency, set on SWARM tile for unsolicitated time messages
// determines precision of send schedule but also power consumption
const unsigned long tileTimeFrequency = 20;
// watchdog reset time should be a multiple of the tileTimeFrequency since it
// is blocking
const unsigned long watchDogResetTime = 5 * tileTimeFrequency;
// channels available, testing values '0-z' for now using characters
char availableChannels[10] = {0};
int numberOfChannels = 0;
// index of message since last restart to keep track of restarts
int messageCounter = 0;
// by setting nextScheduled = 0 sending will start after restart, schedule
// will start for the next message, good for testing
unsigned long nextScheduled = 0;

/*
 *  Measure battery/system voltage Adafruit Feather HUZZAH
 */
float getBatteryVoltage() {
  return analogRead(BATTERY_PIN) * 2 * 3.3/4096;
}

/*
 * Wait for button for maximal ms.
 */
boolean waitForButtonA(DisplayWrapper &dspl, unsigned long ms) {
  unsigned long start = millis();
  boolean res = false;
  while(start + ms > millis()) {
    if (dspl.buttonDebounced(BUTTON_A)) {
      res = true;
      break;
    }
  }
  return res;
}

/*
 *  Collect data and construct message
 */
size_t getMessage(
  char *bfr, const char *channels, const int idx, const unsigned long tme
) {
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
  // payloads, read only the first five channels
  for (size_t i=0; i<5; i++) {
    char channel = availableChannels[i];
    if (channel == 0) break;
    message.payloads[i].channel = channel;
    // get measurement from SDI-12 device on address channel
    len = measurement.getPayload(message.payloads[i].payload, channel);
    // Serial.print(len);
    // Serial.print(", ");
    // Serial.write(message.payloads[i].payload, len);
    // Serial.println();
  }
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
  // We doon't use Wifi or Bluetooth, might save a lot of power
  esp_wifi_set_mode(WIFI_MODE_NULL);
  btStop();
  // Serial.begin(115200);
  // Initialize display and add some boiler plate
  dspl.begin();
  dspl.printBuffer(
    "SWARM node v0.0.5\nfalk.schuetzenmeister@tnc.org\nJune 2022");
  // we can use buttons to advance
  waitForButtonA(dspl, 3000);
  dspl.resetDisplay();
  measurementFrequencyS = mem.getMeasurementFrequency(DEFAULT_SEND_FREQUENCY);
  sprintf(
    bfr, "Reporting frequency:\n\n%d seconds", measurementFrequencyS);
  dspl.printBuffer(bfr);
  dspl.printBuffer("\n\nPUSH BUTTON (A) TO CHANGE\n");
  // wait for input to get into setup routine
  if (waitForButtonA(dspl, 3000)) {
    // this will not exit and requires a reset
    stp.setupFrequency(dspl);
  }
  // print sensor information
  dspl.resetDisplay();
  len = measurement.getName(bfr);
  dspl.printBuffer(bfr, len);
  dspl.printBuffer("\n");
  numberOfChannels = measurement.getChannels(availableChannels);
  sprintf(bfr, "%d SDI12 channel(s) detected\n", numberOfChannels);
  dspl.printBuffer(bfr);
  waitForButtonA(dspl, 2000);
  dspl.resetDisplay();
  for (size_t i=0; i<numberOfChannels; i++) {
    len = measurement.getInfo(bfr, availableChannels[i]);
    dspl.print(availableChannels[i]);
    dspl.print(':');
    dspl.printBuffer(bfr, len);
    dspl.print('\n');
    waitForButtonA(dspl, 2000);
  };
  dspl.printBuffer("\nPUSH BUTTON (A) TO CHANGE ADDRESSES\n");
  // wait for input to get into setup routine
  if (waitForButtonA(dspl, 3000)) {
    // this will not exit and requires a reset
    stp.setupSDI12Addresses(measurement, dspl);
  }
  dspl.resetDisplay();
  // initialize tile and wait until time has been obtained by GPS
  tile.begin(tileTimeFrequency);
  // off we go
  dspl.printBuffer("TILE INIT SUCCESSFUL\n");
  //Serial.print("Channels ");
  //for(size_t i=0; i<10; i++) {
  //  Serial.print(availableChannels[i], DEC);
  //  Serial.print(" ");
  //}
  //Serial.println();
  waitForButtonA(dspl, 3000);
}

void loop() {
  size_t len;
  char bfr[32];
  char commandBfr[16];
  char messageBfr[192];
  // control when loop advances, SWARM tile controls timing
  unsigned long tileTime = tile.waitForTimeStamp();
  /*
   *  1. check and process incoming messages
   */
  processIncoming();
  /*
   *  2. send messages according schedule
   */
  if (tileTime > nextScheduled) {
    len = getMessage(messageBfr, availableChannels, messageCounter, tileTime);
    // Serial.write(messageBfr, len);
    // Serial.println();
    sprintf(bfr, "SENDING AT %d", tileTime);
    dspl.printBuffer(bfr);
    // send to SWARM tile
    tile.sendMessage(messageBfr, len);
    // schedule next message
    nextScheduled = helpers.getNextScheduled(tileTime, measurementFrequencyS);
    // increase counter
    messageCounter++;
  }
  /*
   *  3. power management
   */
   // len = sprintf(commandBfr, "$SL S=%d", tileTimeFrequency);
   // tile.tileCommand(commandBfr, len, bfr);
   esp_sleep_enable_timer_wakeup(tileTimeFrequency * uS_TO_S_FACTOR);
   esp_light_sleep_start();
}
