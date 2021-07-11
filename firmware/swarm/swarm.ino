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
 * 
 * Falk Schuetzenmeister, falk.schuetzenmeister@tnc.org
 * July 2021
 */
#include <Wire.h>
// libraries driving the OLED display
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// include my own classes here
#include "swarmNode.h" 

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
SwarmNode tile = SwarmNode();

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
  Serial.begin(9600);
  delay(500);
  Serial.println("\nDebugging");
  // Initialize display
  display.begin(0x3C, true);
  display.setRotation(1);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();
  // pass display to node object
  display.setCursor(0, 0);
  // add some boiler plate here
  // TODO: move to class handling communication
  display.println("SWARM sensor node\n");
  // display.println("falk.schuetzenmeister@tnc.org\n");
  // display.println("July 2021\n");
  display.display();
  // pass display to SwarmNode class for debug messages
  tile.setDisplay(&display);
  // begin initialization of swarm tile
  tile.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}
