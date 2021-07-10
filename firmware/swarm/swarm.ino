#include <Wire.h>
#include <Adafruit_GFX.h>
// this library is driving the OLED display
#include <Adafruit_SH110X.h>

// include our classes here
#include "swarmNode.h" 

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
SwarmNode tile = SwarmNode();

// from library example
// OLED FeatherWing buttons map to different pins depending on board:
#if defined(ESP8266)
  #define BUTTON_A  0
  #define BUTTON_B 16
  #define BUTTON_C  2
#elif defined(ESP32)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
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
  display.println("SWARM sensor node");
  display.println("");
  // display.println("falk.schuetzenmeister@tnc.org");
  // display.println("");
  // display.println("July 2021");
  // display.display();
  // display.println("ready");
  // display.display();
  // pass display to SwarmNode class for debug messages
  tile.setDisplay(&display);
  // start initialization of the swarm tile
  tile.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}
