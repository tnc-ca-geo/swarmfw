/*
 *  Wrapper class around display
 *
 *  - abstracts display and could be potentially swapped for other
 *    output options such as Serial
 *  - adds functionality
 */
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


class SwarmDisplay {

  private:
    Adafruit_SH1107 thisDisplay = Adafruit_SH1107(64, 128, &Wire);
    // buffer and lineNumber for later implementation of scrolling
    // display, UNUSED for now
    char displayBuffer[20][7];
    int lineNumber;
  public:
    SwarmDisplay(); 
    void begin();
    void clearDisplay();
    void display();
    int getCursorY();
    void print(char character);
    void printBuffer(char *bfr, size_t len);
    void printBuffer(String string);
    void println(String line);
    // use render to implement scrolling display later
    // UNUSED for now
    void render();
    void setCursor(int x, int y);
};
