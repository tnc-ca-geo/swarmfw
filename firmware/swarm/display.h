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
    char displayBuffer[20][7];
    int lineNumber;
  public:
    SwarmDisplay(); 
    void begin();
    void print(char character);
    void println(String line);
    void printBuffer(char *bfr, size_t len);
    void clearDisplay();
    void display();
    void setCursor(int x, int y);
    int getCursorY();
};
