#include <Arduino.h>
#ifndef _SERIAL_WRAPPER_H_
#include "serialWrapper.h"
#endif

#define STACK_SIZE 512


boolean checkNmeaChecksum(const char *bfr, const size_t len);
size_t cleanCommand(char *bfr, const char *command, const size_t len);
size_t formatMessage(const char *message, const size_t len, char *bfr);
uint8_t nmeaChecksum(const char *bfr, const size_t len);
uint64_t parseTime(const char *timeResponse, const size_t len);
size_t popFromStack(
  char *part, char *bfr, const size_t bfrSize=STACK_SIZE, const char sep='\n');
boolean pushToStack(
  char *stack, const char *bfr, const size_t len,
  const size_t stackSize=STACK_SIZE, const char sep='\n');
int16_t searchBfr(
  const char *bfr, const size_t len, const char *srchTrm, const size_t srchLen);
size_t toHexString(const char *inputBfr, const size_t len, char *bfr);
boolean validateResponse(const char * bfr, const size_t len);
boolean validateTime(struct tm tme);


class NewSwarmNode {
  private:
    SerialWrapperBase *_wrappedSerialRef;
    boolean dev;
  public:
    // ---- local state -------------
    // a buffer holding an expected partial command response
    char expectedBfr[32] = {0};
    size_t expectedBfrLen = 0;
    // This is the command stack, default commands will be executed for
    // configuration
    char commandStack[STACK_SIZE] = "$DT 20\n$RT 60\n$GN 60\n$GS 60\n$MT D=U\n";
    // response stack queues responses for parsing and processes
    char responseStack[STACK_SIZE] = {0};
    // output stack provides an external interface
    char outputStack[STACK_SIZE] = {0};
    // after this time stop waiting for command response
    uint64_t commandTimeOut = 0;
    boolean swarmInitialized = false;
    int64_t timestamp = 0;
    // MC time when timestamp was obtained, to detect stale time readings
    int64_t timestampObtained = 0;

    // ---- end state --------------
    NewSwarmNode(
      SerialWrapperBase *wrappedSerialObject, const boolean dev=true);
    // main loop
    void loopOnce();
    // main loop methods ==> move to private?
    void initialize();
    void parseResponse();
    void readSerial();
    void sendCommands();
    // API methods
    size_t readLine(char *bfr);
    boolean sendMessage(const char *bfr, size_t len);
    // other methods acting on state
    void issueTileCommand(
      const char *command, size_t len, const char *release,
      const size_t releaseLen=0, const uint16_t blockTime = 1);
    void setCommandBlock(const uint64_t timeout);
    void removeCommandBlock();
    boolean checkCommandBlock();
    boolean setCommandStack(const char *bfr, size_t len);
    void checkTimeAndLocationAge();
    boolean tileReady();
};
