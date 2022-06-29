#include "src/serialWrapper.h"
#include "src/newSwarmNode.h"


SerialWrapper srl = SerialWrapper(&Serial2, 115200);
// second arguments indicates dev mode deleting unsent messages on restart
NewSwarmNode tile = NewSwarmNode(&srl, false);


boolean flag = false;


void setup () {
  Serial.begin(115200);
  delay(1000);
  while(!Serial);
  Serial.write("\n\n");
}


void loop () {
  char bfr[255] = {0};
  tile.loopOnce();
  size_t len = tile.readLine(bfr);
  if (len) { Serial.print(bfr); }
  if (!flag && tile.tileReady()) {
    flag = tile.sendMessage("hello\n", 5);
  }
}
