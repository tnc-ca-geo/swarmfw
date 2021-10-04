# include <SDI12.h>
# include "SDI12Wrapper.h"
# include <Arduino.h>

# define DATA_PIN 21
# define POWER_PIN -1

SDI12 mySDI12(DATA_PIN);

SDI12Measurement::SDI12Measurement() {
  mySDI12.begin();
};

/*
 * Read the SDI-12 buffer (after command)
 */
size_t SDI12Measurement::readSDI12Buffer(char *bfr) {
  size_t i=0;
  // terminate return for the case that there is no return
  // this is important for \0 terminated strings
  bfr[0] = 0;
  while (mySDI12.available()) {
    uint8_t c = mySDI12.read();
    if ((c != '\n') && (c != '\r') && (i < SDI12_BUFFER_SIZE)) {
      bfr[i] = c;
    } else {
      bfr[i] = 0;
      break;
    }
    i++;
  }
  return i;
};


/*
 * Send an SDI-12 command and wait for return
 */
size_t SDI12Measurement::sendSDI12(char *cmd, char *bfr) {
  mySDI12.clearBuffer();
  delay(300);
  mySDI12.sendCommand((char*) cmd);
  delay(300);
  return readSDI12Buffer(bfr);
};

/*
 * Get a list of available channels in form of a char array since addresses
 * are 8 byte characters
 */
size_t SDI12Measurement::getChannels(char *bfr, const char maxChannel) {
  int idx = 0;
  char localBfr[128];
  for (char i='0'; i<=maxChannel; i++) {
    // Serial.print(i);
    if (getInfo(localBfr, i) > 1) {
      bfr[idx] = i;
      idx += 1;
    }
    if (idx>9) break;
  }
  return idx;
};

/*
 * A simple method to check configuration
 */
size_t SDI12Measurement::getName(char *bfr) {
  char retString[] = "SDI 12 Measurement\0";
  size_t len = sizeof(retString);
  memcpy(bfr, retString, len);
  return(len);
};

// get sensor information
// this is currently very simple but should be extended
// this approach does not work with more than one sensor
size_t SDI12Measurement::getInfo(char *bfr, char addr) {
  char commandBfr[4];
  commandBfr[0] = addr;
  memcpy(commandBfr+1, "I!\0", 3);
  return sendSDI12(commandBfr, bfr);
}

// return payLoad for LoRaWAN messages
size_t SDI12Measurement::getPayload(char addr, char *bfr) {
  char cmd[] = {addr, 'C', '!', 0, 0};
  char parseBuffer[4];
  char rspns[SDI12_BUFFER_SIZE];
  size_t len = 0;
  len = sendSDI12(cmd, rspns);
  // extract wait time from response
  memcpy(parseBuffer, rspns+1, 3);
  int wait = atoi((char*) parseBuffer);
  // waiting for results
  unsigned long int timerStart = millis();
  while ((millis() - timerStart) < (1000 * wait)) {}
  // start response string with the address byte
  bfr[0] = addr;
  size_t resIndex = 1;
  // request results
  // - iterate through ASCII code, representing 0..9 and
  // - issue commands x0D0! to x0D9!
  for (char i=48; i<56; i++) {
    char cmd[] = {addr, 'D', i, '!', 0};
    len = sendSDI12(cmd, rspns);
    memcpy(bfr+resIndex, rspns+1, len);
    // the response is \0 terminated but we want to concatenate those returns
    // for this reason we override the last character of the prior copy
    resIndex += len-1;
  }
  // however we would like to maintain one \0 at the end
  resIndex + 1;
  return resIndex;
}

/*
 * Use this for debugging
 * - write real tests eventually
 */
void SDI12Measurement::debug() {
  char bfr[256];
  size_t len;
  if (Serial) {
    len = getName(bfr);
    Serial.println(bfr);
    len = getInfo(bfr);
    Serial.println(bfr);
    len = getPayload('0', bfr);
    Serial.println(bfr);
  }
}
