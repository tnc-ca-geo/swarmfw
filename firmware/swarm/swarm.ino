/*
 * Firmware for SWARM evaluation kit with Adafruit ESP32 Huzzah Feather and
 * SDI-12 sensor (eventually)
 * 
 * TODOs:
 * - investigate testing
 * - investigate use of modules accross projects
 * 
 * Current goals: 
 * - proof of concept and regular sending of messages
 * - simple as possible
 * - do not wrangle control from the SWARM tile
 * - we can use simple synchronous loop for now
 * 
 * Falk Schuetzenmeister, falk.schuetzenmeister@tnc.org
 * July 2021
 */
#include <Wire.h> 
// libraries driving the OLED display
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// include my own classes here
// #include "display.h"
#include "swarmNode.h" 

// Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
SwarmDisplay display = SwarmDisplay();
SwarmNode tile = SwarmNode();
int16_t ctr = 0;

// from library example
// OLED FeatherWing buttons map to different pins depending on board:
// TODO: remove boards we are not likely to use
#if defined(ESP8266)
  #define BUTTON_A  0
  #define BUTTON_B 16
  #define BUTTON_C  2
#elif defined(ESP32)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
// SWARM is using this one
#elif defined(ARDUINO_STM32_FEATHER)
  #define BUTTON_A PA15
  #define BUTTON_B PC7
  #define BUTTON_C PC5
#elif defined(TEENSYDUINO)
  #define BUTTON_A  4
  #define BUTTON_B  3
  #define BUTTON_C  8
#elif defined(ARDUINO_NRF52832_FEATHER)
  #define BUTTON_A 31
  #define BUTTON_B 30
  #define BUTTON_C 27
#else // 32u4, M0, M4, nrf52840 and 328p
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5
#endif


void setup() {
  // for debugging
  Serial.begin(9600);
  delay(500);
  // Initialize display
  display.begin();
  // add some boiler plate here
  display.printBuffer("SWARM sensor node\n");
  display.printBuffer("falk.schuetzenmeister@tnc.org\n");
  display.printBuffer("July 2021\n");
  delay(1000);
  // pass display to SwarmNode class for debug messages
  tile.setDisplay(&display);
  // initialize tile
  tile.begin();
  display.printBuffer("\nTILE INIT SUCCESSFUL\n");
}

void loop() {
  char res[256];
  char numberBffr[4];
  char commandBffr[255];
  int len = tile.tileCommand("$DT @", 5, res);
  display.printBuffer(res, len);
  delay(500);
  sprintf(numberBffr, "%04u", ctr);
  memcpy(commandBffr, "$TD HD=7200,", 12);
  for (int i=0; i<4; i++) commandBffr[12+i] = numberBffr[i];
  len = tile.tileCommand(commandBffr, 16, res); 
  display.printBuffer(res, len);
  ctr++;
  // send every 30 minutes
  delay(1800000);
  // put your main code here, to run repeatedly:
}
