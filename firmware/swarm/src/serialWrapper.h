/*
 *  A Wrapper around Arduino's Serial class to allow for better testing plus
 *  some added functionality. I considered to subclass Stream but decided
 *  otherwise since we might add some more complex functionality, e.g. an
 *  extended buffer size, interrupt driven read, etc.
 */
#ifndef _SERIAL_WRAPPER_H_
#define _SERIAL_WRAPPER_H_
#endif

 // Base class
 class SerialWrapperBase {
   public:
     virtual ~SerialWrapperBase() {};
     virtual boolean available() { return false; };
     virtual void begin(int speed) {};
     virtual char read() { return 0; };
     // virtual int readLine(*bfr) {};
     virtual void write(byte character) {};
     virtual size_t write(char *bfr, size_t len) { return 0; };
 };

 // Inherited class
 class SerialWrapper: public SerialWrapperBase {
  private:
    HardwareSerial *_serialRef;
  public:
    SerialWrapper(HardwareSerial *serial, int speed) {
      _serialRef = serial;
      _serialRef->begin(speed);
    };
    boolean available() { return _serialRef->available(); };
    char read() { return _serialRef->read(); };
    void write(byte character) { _serialRef->write(character); };
    size_t write(char *bfr, size_t len) { return _serialRef->write(bfr, len); }
 };
