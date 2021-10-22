/*
 *  This file contains functionality used in swarm.ino but should be tested
 *  separately
 */

/*
 * A message can hold up to five of those BUT the message length is
 * limited to 192 and we don't control for this
 */
typedef struct {
  // a channel identifier to be used within the message type, will take up to 3
  // of 192 characters
  uint8_t channel = 0;
  // a payload to be used within the message type
  char payload[150] = {0};
} Payload;

 // struct holding all information for messages
typedef struct {
  // commas will take 5 of 192 characters
  // a message index, will take 6 of 192 characters
  unsigned long index;
  // a timeStamp (UNIX epoch), will take 10 of 192 characters
  unsigned long timeStamp;
  // batteryVoltage, will take 4 of 192 characters
  float batteryVoltage;
  // a two letter code for message type, will take 2 of 192 characters
  char type[2];
  Payload payloads[5];
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
      // allow for overshooting and safely trim down
      char localBfr[350] = {0};
      idx += sprintf(localBfr, "%06u", message.index);
      localBfr[idx] = ',';
      idx += 1 + sprintf(localBfr+idx+1, "%010d", message.timeStamp);
      localBfr[idx] = ',';
      idx += 1 + sprintf(localBfr+idx+1, "%1.2f", message.batteryVoltage);
      if (message.type[0] != 0) {
        localBfr[idx] = ',';
        idx += 1 + sprintf(localBfr+idx+1, "%.2s", message.type);
        for (size_t i=0; i<5; i++) {
          if (message.payloads[i].channel != 0 and idx < 192) {
            localBfr[idx] = ',';
            idx += 1 + sprintf(localBfr+idx+1, "%d", message.payloads[i].channel);
            localBfr[idx] = ',';
            idx += 1 + sprintf(localBfr+idx+1, "%s", message.payloads[i].payload);
          }
        }
      }
      if (idx > 189) { idx = 192; };
      memcpy(bfr, localBfr, idx);
      // mark that message has been truncated
      memcpy(bfr+190, "..", 2);
      return idx;
    };

    static unsigned long getNextScheduled(
      unsigned long timeStamp, unsigned long interval
    ) {
      // cast to double required otherwise intermeduare result will be int
      // float is not precise enough
      return static_cast<double>((timeStamp + interval)/interval) * interval;
    };
};
