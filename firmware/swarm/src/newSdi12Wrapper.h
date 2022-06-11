#include <Arduino.h>

#define NEW_DATA_PIN 21
#define NEW_POWER_PIN -1
#define NEW_SDI12_BUFFER_SIZE 255


class sdi12Interface {
  private:
  public:
    // state variables
    char responseStack[NEW_SDI12_BUFFER_SIZE] = {0};
    char outputStack[NEW_SDI12_BUFFER_SIZE] = {0};
    uint64_t commandTimeOut = 0;
    // constructor
    sdi12Interface();
    // the main loops
    void loopOnce();
    // API
    size_t readLine(char *bfr);
    // helper methods
    bool checkCommandBlock();
    void readSdi12Buffer();
    void sendSdi12(char *bfr);
};


// A class holding more complex workflows based on newsdi12Interface
class newSdi12Measurement {
  private:
    sdi12Interface interface=sdi12Interface();
    // response is ready
    bool ready = true;
    // retrieving state
    bool retrieving;
    // holding the response
    char responseBfr[255];
    // putPut
    char outputBfr[255];
    // keep track of len
    size_t outputBfrLen;
    // last command
    char lastCommand[6];
    // internal methods
    void parseResponse();
    // process a command
    void processCommand(const char adr, const char *command);
    // pages retrieved, ascii '0' = 48 to ascii '9' = 57
    int retrievalIdx = 48;
    int variableIdx = 0;
    int variableCount = 0;
    // we might not need this
    uint64_t measurementReadyTime = 0;
  public:
    newSdi12Measurement();
    // loop
    void loopOnce();
    // public API
    // request a measurement
    void getMeasurement(const char adr, const char command='C');
    // request information about a sensor
    void getInfo(const char adr);
    // response is ready
    bool responseReady();
    // read response
    size_t readResponse(char *bfr);
    // parse the response to C and M commands
    void parseMeasurementResponse(const char *bfr);
    void retrieveReadings(const char addr, const char idx);
};
