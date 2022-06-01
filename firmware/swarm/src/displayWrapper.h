/*
 *  Wrapper class around display
 *
 *  - abstracts display and could be potentially swapped for other
 *    output options such as Serial
 *  - adds functionality
 */
 // libraries driving the OLED display
#ifndef _DISPLAY_WRAPPER_H_
#define _DISPLAY_WRAPPER_H_
#endif

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// this varies on different Feather boards see example code
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14

// add base class for mocking and testing
class DisplayWrapperBase {
  public:
    virtual ~DisplayWrapperBase() {};
    virtual void begin() {};
    virtual boolean button(int button) { return false; };
    virtual boolean buttonDebounced(int button) { return false; };
    virtual void clearDisplay() {};
    virtual void display() {};
    virtual int getCursorY() { return 0; };
    virtual void print(char character) {};
    virtual void printBuffer(char *bfr, size_t len) {};
    virtual void printBuffer(String string) {};
    virtual void setTextColor(uint16_t textcolor) {};
    virtual void shortPrintBuffer(char *bfr, size_t len) {};
    virtual void println(String line) {};
    virtual void resetDisplay() {};
    virtual void setCursor(int x, int y) {};
    virtual void write(char c) {};
};

class DisplayWrapper: public DisplayWrapperBase {
  private:
    Adafruit_SH1107 thisDisplay = Adafruit_SH1107(64, 128, &Wire);
    // buffer and lineNumber for later implementation of scrolling
    // display, UNUSED for now
    char displayBuffer[20][7];
    unsigned long buttonDownTime;
    int lineNumber;
    // keep track whether button has been released;
    boolean buttonState[32] = { true };
  public:
    DisplayWrapper() {
      pinMode(BUTTON_A, INPUT_PULLUP);
      pinMode(BUTTON_B, INPUT_PULLUP);
      pinMode(BUTTON_C, INPUT_PULLUP);
    };

    void begin() {
      // following commands have not been wrapped yet
      thisDisplay.begin(0x3C, true);
      thisDisplay.setTextSize(1);
      setTextColor(SH110X_WHITE);
      thisDisplay.setRotation(1);
      clearDisplay();
      setCursor(0, 0);
      lineNumber=0;
    };

    // read button A
    boolean button(int bttn) {
      return !digitalRead(bttn);
    };

    // check for release (or 1s time passed) and read button
    boolean buttonDebounced(int bttn) {
      boolean ret = !digitalRead(bttn);
      if (!buttonState[bttn]) {
        buttonState[bttn] = ret;
        if (ret) buttonDownTime = millis();
      } else {
        buttonState[bttn] = ret;
        if (buttonDownTime + 1000 > millis()) {
          ret = false;
        }
      }
      return ret;
    };

    void clearDisplay() { thisDisplay.clearDisplay(); };

    void display() { thisDisplay.display(); };

    int getCursorY() { return thisDisplay.getCursorY(); };

    void print(char character) { thisDisplay.print(character); };

    void print(int number, int format=DEC) {
      thisDisplay.print(number, format);
    };

    void println(String line) { thisDisplay.println(line); };

    void printBuffer(char *bfr, size_t len) {
      if (getCursorY() > 60) {
          clearDisplay();
          setCursor(0, 0);
      }
      for (size_t i=0; i<len; i++) {
        print(bfr[i]);
      }
      display();
    };

    void shortPrintBuffer(char *bfr, size_t len) {
      if (static_cast<int>(len) > 20) {
        printBuffer(bfr, 21);
        if (!bfr[20] != '\n') print('\n');
      } else {
        printBuffer(bfr, len);
      }
    };

    void printBuffer(String string) {
      char bfr[512];
      size_t len = string.length();
      if (len > 512) {
        string = string.substring(0, 512);
        len = 512;
      }
      // not sure why we need len+1 here but last letter
      // get truncated otherwise
      string.toCharArray(bfr, len+1);
      printBuffer(bfr, len);
    };

    /*
     *  convenience method combining two commands often used together
     */
    void resetDisplay() {
      clearDisplay();
      setCursor(0, 0);
    };

    void setCursor(int x, int y) { thisDisplay.setCursor(x, y); };

    void setTextColor(uint16_t textcolor) {
      thisDisplay.setTextColor(textcolor);
    };

    void write(char c) { thisDisplay.write(c); };
};
