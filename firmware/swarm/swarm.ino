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
 * July 2021
 * 
 */
// used for watchdg functionality
#include <esp_task_wdt.h>
// my libraries
#include "src/swarmNode.h" 

DisplayWrapper dspl = DisplayWrapper();
SerialWrapper srl = SerialWrapper(&Serial2, 115200);
// last value indicates dev mode deleting all old messages on restart
// TODO: set to false for production
SwarmNode tile = SwarmNode(&dspl, &srl, true);

// Sending every hour (3600s) meets the monthly included rate
const unsigned int sendFrequencyInSeconds = 3600;
// time polling frequency, set on the tile for unsolicitated time messages
// this determines the precision of the send schedule but also power consumption
const int tileTimeFrequency = 20;
// the watchdog reset time should be a multiple of the tileTimeFrequency since it
// is blocking
const int watchDogResetTime = 5 * tileTimeFrequency;
// index of message since last restart to keep track of restarts by the
// watchdog
int messageCounter = 0;
// by setting nextScheduled = 0 sending will start right after restart, schedule
// will start for the next message, good for testing
unsigned long int nextScheduled = 0;

/* 
 *  assemble a CSV like message by adding some metadata
 */
size_t assembleMessage(int ctr, int timeStamp, const char *payload, size_t len, 
  float batteryVoltage, char *bfr) 
{
  sprintf(bfr, "%06u", ctr);
  bfr[6] = ',';
  sprintf(bfr+7, "%10d", timeStamp);
  bfr[17] = ',';
  memcpy(bfr+18, payload, len);
  bfr[len+18] = ',';
  sprintf(bfr+len+19, "%1.2f", batteryVoltage);
  return len+23;
}

/*
 *  Measure battery/system voltage Adafruit Feather HUZZAH
 */
float getBatteryVoltage() {
  return analogRead(A13) * 2 * 3.3/4096;
};


/* 
 * Calculate the next scheduled time (in unix epoch time)
 */
unsigned long int getNextScheduled(int timeStamp, int interval) {
  return static_cast<int>(
    static_cast<double>(
      timeStamp + interval)/static_cast<double>(interval)) * interval;
};

/*
 * Setup roiutine
 */
void setup() {
  // Wait for Serial and hope that it is ready after a second
  // Keep going if Serial is not available for debugging
  Serial.begin(9600);
  delay(1000);
  // Initialize display and add some boiler plate
  dspl.begin();
  dspl.printBuffer("SWARM sensor node\n");
  dspl.printBuffer("falk.schuetzenmeister@tnc.org\n");
  dspl.printBuffer("July 2021\n");
  delay(2000);
  // initialize tile and wait until time has been obtained successfully 
  tile.begin(10);
  // and off we go
  dspl.printBuffer("\nTILE INIT SUCCESSFUL\n");
}

void loop() {
  size_t messageLen = 0;
  char messageBfr[256];
  // this command controls when loop advances
  unsigned long tileTime = tile.waitForTimeStamp();
  // send scheduled message
  if (tileTime > nextScheduled) {
    dspl.clearDisplay();
    dspl.setCursor(0, 0);
    dspl.printBuffer("SENDING AT ");
    char timeMessage[16];
    size_t strLen = sprintf(timeMessage, "%i", tileTime); 
    dspl.printBuffer(timeMessage, strLen);
    messageLen = assembleMessage(
      messageCounter, tileTime, "test", 4, getBatteryVoltage(), messageBfr);
    tile.sendMessage(messageBfr, messageLen);
    nextScheduled = getNextScheduled(tileTime, sendFrequencyInSeconds);
    messageCounter++;
  } 
}
