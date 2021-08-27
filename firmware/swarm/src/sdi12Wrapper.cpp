# include <SDI12.h>
# include "SDI12Wrapper.h"
# include <Arduino.h>

# define DATA_PIN 17
# define POWER_PIN -1


SDI12 mySDI12(DATA_PIN);


SDI12Measurement::SDI12Measurement() {
  mySDI12.begin();
};


void SDI12Measurement::readSDI12Buffer(uint8_t *bfr) {
  int i=0;
  bfr[0] = 0;
  while (mySDI12.available()) {
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r') && (i < SDI12_BUFFER_SIZE)) {
      bfr[i] = c;
    } else {
      bfr[i] = 0;
      break;
    }
    i++;
  }
};


void SDI12Measurement::sendSDI12(uint8_t *cmd, uint8_t *bfr) {
  mySDI12.clearBuffer();
  delay(300);
  mySDI12.sendCommand((char*) cmd);
  delay(300);
  readSDI12Buffer(bfr);
};

// a simple method to check configuration
void SDI12Measurement::getName(uint8_t *bfr) {
  // TODO: simplify with memcpy
  uint8_t msg[] = "SDI 12 Measurement";
  int i = 0;
  do {
    bfr[i] = msg[i];
    i++;
  } while (msg[i] != 0);
};

// get more (dynamically derived) information
void SDI12Measurement::getInfo(uint8_t *bfr) {
  // set terminating byte in case we don't get response
  uint8_t cmd[] = "?I!";
  uint8_t bffr[SDI12_BUFFER_SIZE];
  uint8_t msg[] = "Attached device: ";
  sendSDI12(cmd, bffr);
  memcpy(bfr, msg, sizeof(msg) - 1);
  for(int i=0; i < sizeof(bffr); i++) { 
    bfr[i+sizeof(msg)-1] = bffr[i]; 
  }
}

// return payLoad for LoRaWAN messages
uint8_t SDI12Measurement::getPayload(uint8_t addr, uint8_t *bfr) {
  uint8_t cmd[] = {addr, 'C', '!', 0, 0};
  uint8_t parseBuffer[16];
  uint8_t rspns[SDI12_BUFFER_SIZE]; 
  int wait = 0;
  int j = 0;
  int index = 0;
  int resIndex = 0;
  unsigned long timerStart;
  sendSDI12(cmd, rspns);
  // extracting the wait time from respns
  for (int i=0; i<3; i++) { parseBuffer[i] = rspns[i+1]; }
  wait = atoi((char*) parseBuffer);
  // waiting for the results
  // Hopefully coming in quicker than the election results in 2020
  timerStart = millis();
  while ((millis() - timerStart) < (1000 * wait)) {}
  // start response string with the address byte
  bfr[resIndex] = addr;
  // keep track of how much we have added
  resIndex = 1;
  // requesting results
  cmd[1] = 'D';
  cmd[3] = '!';
  // instead of converting from int to char we iterating
  // through the proper ASCII code, representing 0 .. 9
  // issuing commands x0D0! to x0D9! 
  for (uint8_t i=48; i<58; i++) {
    cmd[2] = i;
    sendSDI12(cmd, rspns);
    index = 0;
    do { 
      index++;
    } while ((rspns[index] != 0) && (index < SDI12_BUFFER_SIZE));
    for (j=0; j < index-1; j++) { bfr[j + resIndex] = rspns[j + 1]; }
    resIndex += j;
  }
  // We are returning this for convenience. See CayenneLPP copy method
  return resIndex;
}
