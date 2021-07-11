
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
}

void SwarmDisplay::print(char character) {
  thisDisplay.print(character);
}

void SwarmDisplay::println(String line) {
  thisDisplay.println(line);
}

void SwarmDisplay::clearDisplay() {
  thisDisplay.clearDisplay();
}

void SwarmDisplay::setCursor(int x, int y) {
  thisDisplay.setCursor(x, y);
  thisDisplay.println("hallo");
}

void SwarmDisplay::display() {
  thisDisplay.display();
}

int SwarmDisplay::getCursorY() {
  return thisDisplay.getCursorY();
}
