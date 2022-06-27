#include <Arduino.h>
#include <SDI12.h>

#define NEW_DATA_PIN 21
#define NEW_POWER_PIN -1
#define NEW_SDI12_BUFFER_SIZE 255

/*
 * Wrap the hardware interaction in a base class for mocking in tests.
 * Only wrap what is needed.
 */
class Sdi12WrapperBase {
  public:
    virtual ~Sdi12WrapperBase() {};
    virtual void begin();
    virtual bool available();
    virtual char read();
    virtual void sendCommand(const char *bfr);
};

/*
 * Implement the actual SDI12 library into the wrapper
 */
class Sdi12Wrapper: public Sdi12WrapperBase {
  private:
    SDI12 _sdi12;
  public:
    Sdi12Wrapper(int datapin=NEW_DATA_PIN);
    void begin();
    bool available();
    char read();
    void sendCommand(const char *bfr);
};


// This class holds complex workflows based on Sdi12Interface
class NewSdi12Measurement {
  private:
    // A class reference wrapping the SDI12 library
    Sdi12WrapperBase *wrapper;
    // ----- Local state -------------------------------------------------------
    // response is ready
    bool ready = true;
    // retrieving i.e. blocking state
    bool retrieving = false;
    // holding the public output
    char outputBfr[255] = {0};
    size_t outputBfrLen = 0;
    // holding last command
    char lastCommand[6] = {0};
    // pages retrieved, ascii '0' = 48 to ascii '9' = 57
    int retrievalIdx = 48;
    int variableIdx = 0;
    int variableCount = 0;
    uint64_t measurementReadyTime = 0;
    char responseStack[NEW_SDI12_BUFFER_SIZE] = {0};
    // -------------------------------------------------------------------------
    // ----- private methods ---------------------------------------------------
    // parse the response to C and M commands
    void parseResponse();
    void processCommand(const char adr, const char *command);
    void parseMeasurementResponse(const char *bfr);
    void retrieveReadings(const char addr, const char idx);
    void readSdi12Buffer();
    // reset local state, will terminate any running retrieval process
    void reset();
    // -------------------------------------------------------------------------
  public:
    // constructor
    NewSdi12Measurement(Sdi12WrapperBase *wrapper);
    // loop
    void loopOnce();
    // public API
    // request a measurement
    void requestMeasurement(const char adr, const char command='C');
    // request information about a sensor
    void requestInfo(const char adr);
    // response is ready
    bool getResponseReady();
    // read response
    size_t getResponse(char *bfr);
    // TODO: move to private
};
