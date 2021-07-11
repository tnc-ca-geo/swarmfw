
#include "display.h"

SwarmDisplay::SwarmDisplay() {
}

void SwarmDisplay::begin() {
  thisDisplay.begin(0x3C, true);
  thisDisplay.setTextSize(1);
  thisDisplay.setTextColor(SH110X_WHITE);
  thisDisplay.setRotation(1);
  thisDisplay.clearDisplay();
  thisDisplay.setCursor(0, 0);
  lineNumber=0;
}

void SwarmDisplay::print(char character) {
  thisDisplay.print(character);
}

void SwarmDisplay::println(String line) {
  thisDisplay.println(line);
}

void SwarmDisplay::printBuffer(char *bfr, size_t len) {
  if (thisDisplay.getCursorY() > 50) {
      thisDisplay.clearDisplay();
      thisDisplay.setCursor(0, 0);
  }
  for (int i=0; i<len; i++) {
    thisDisplay.print(bfr[i]);
  }
  thisDisplay.display();
}

void SwarmDisplay::clearDisplay() {
  thisDisplay.clearDisplay();
}

void SwarmDisplay::setCursor(int x, int y) {
  thisDisplay.setCursor(x, y);
}

void SwarmDisplay::display() {
  thisDisplay.display();
}

int SwarmDisplay::getCursorY() {
  return thisDisplay.getCursorY();
}
