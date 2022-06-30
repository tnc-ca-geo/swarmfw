#include <Arduino.h>
#include <SDI12.h>

#define NEW_DATA_PIN 21
#define NEW_POWER_PIN -1
#define NEW_SDI12_BUFFER_SIZE 255
// see http://www.sdi-12.org/current_specification/SDI-12_version-1_4-Jan-10-2019.pdf
#define SDI12_DEFAULT_COMMAND_TIMEOUT 380000
#define SDI12_TIMEOUT 10000000


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
    // check for sensors i.e. blocking state
    bool sensorCheck = false;
    // holding the public output
    char outputBfr[255] = {0};
    size_t outputBfrLen = 0;
    // holding last command, since response is not indicative
    char lastCommand[6] = {0};
    // index ascii '0' = 48 to ascii '9' = 57 for aDx! and x! commands
    int retrievalIdx = 48;
    // variable index, NOT IMPLEMENTED
    int variableIdx = 0;
    int variableCount = 0;
    // time when measurement readings are expected to be ready
    uint64_t measurementReadyTime = 0;
    // time out all workflows
    uint64_t timeOutTime = 0;
    // buffer complete lines from incoming Serial
    char responseStack[NEW_SDI12_BUFFER_SIZE] = {0};
    // ----- private methods ---------------------------------------------------
    // parse the response of C and M commands
    void parseResponse();
    // initiate scheduled operations
    void runScheduled();
    void parseMeasurementResponse(const char *bfr);
    void retrieveReadings(const char addr, const char idx);
    void checkSensor(const char addr);
    void readSdi12Buffer();
    // reset local state, will terminate any running workflows
    void reset();
    // -------------------------------------------------------------------------
  public:
    // constructor
    NewSdi12Measurement(Sdi12WrapperBase *wrapper);
    // loop
    void loopOnce();
    // public API
    void processCommand(const char adr, const char *command);
    // request a measurement
    void requestMeasurement(const char adr, const char command='C');
    // request information about a sensor
    void requestInfo(const char adr);
    // check on which addresses sensor is attached
    void requestSensors();
    // response is ready
    bool getResponseReady();
    // read response
    size_t getResponse(char *bfr);
    // TODO: move to private
};
