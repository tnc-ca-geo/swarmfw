/*
 *  This file contains functionality used in swarm.ino but should be tested
 *  separately
 */
typedef struct {
  // commas will take 5 of 192 characters
  // a message index, will take 6 of 192 characters
  unsigned long int index;
  // a timeStamp (UNIX epoch), will take 10 of 192 characters
  unsigned long int timeStamp;
  // a two letter code for message type, will take 2 of 192 characters
  char type[2];
  // a channel identifier to be used within the message type, will take up to 3
  // of 192 characters
  uint8_t channel;
  // a payload to be used within the message type, will take what is leftover,
  // we reserve 12 characters as backup
  char payload[150];
  // batteryVoltage, will take 4 of 192 characters
  float batteryVoltage;
} Message;


class MessageHelpers {

  public:

    /*
     * Convert message struct into a string. char buffers need to be \0
     * terminated or they will added at the lenght of their definition
     */
    static size_t formatMessage(Message message, char *bfr) {
      // sprintf returns int, the string copied to the buffer is \0 terminated
      int len;
      size_t idx = 0;
      idx += sprintf(bfr, "%06u", message.index);
      bfr[idx] = ',';
      idx += 1 + sprintf(bfr+idx+1, "%10d", message.timeStamp);
      bfr[idx] = ',';
      idx += 1 + sprintf(bfr+idx+1, "%.2s", message.type);
      bfr[idx] = ',';
      idx += 1 + sprintf(bfr+idx+1, "%d", message.channel);
      bfr[idx] = ',';
      idx += 1 + sprintf(bfr+idx+1, "%.150s", message.payload);
      bfr[idx] = ',';
      idx += 1 + sprintf(bfr+idx+1, "%1.2f", message.batteryVoltage);
      return idx;
    };

    static unsigned long int getNextScheduled(
      unsigned long int timeStamp, int interval
    ) {
      // cast to double required otherwise intermeduare result will be int
      // float is not precise enough
      return static_cast<double>((timeStamp + interval)/interval) * interval;
    };
};
