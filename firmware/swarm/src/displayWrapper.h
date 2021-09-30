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

// add base class for mocking and testing
class DisplayWrapperBase {
  public:
    virtual ~DisplayWrapperBase() {};
    virtual void begin() {};
    virtual void clearDisplay() {};
    virtual void display() {};
    virtual int getCursorY() { return 0; };
    virtual void print(char character) {};
    virtual void printBuffer(char *bfr, size_t len) {};
    virtual void printBuffer(String string) {};
    virtual void shortPrintBuffer(char *bfr, size_t len) {};
    virtual void println(String line) {};
    virtual void setCursor(int x, int y) {};
};

class DisplayWrapper: public DisplayWrapperBase {
  private:
    Adafruit_SH1107 thisDisplay = Adafruit_SH1107(64, 128, &Wire);
    // buffer and lineNumber for later implementation of scrolling
    // display, UNUSED for now
    char displayBuffer[20][7];
    int lineNumber;
  public:
    DisplayWrapper() {};

    void begin() {
      // following commands have not been wrapped yet
      thisDisplay.begin(0x3C, true);
      thisDisplay.setTextSize(1);
      thisDisplay.setTextColor(SH110X_WHITE);
      thisDisplay.setRotation(1);
      clearDisplay();
      setCursor(0, 0);
      lineNumber=0;
    };

    void clearDisplay() { thisDisplay.clearDisplay(); };

    void display() { thisDisplay.display(); };

    int getCursorY() { return thisDisplay.getCursorY(); };

    void print(char character) { thisDisplay.print(character); };

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

    void println(String line) { thisDisplay.println(line); };

    void setCursor(int x, int y) { thisDisplay.setCursor(x, y); };
};
