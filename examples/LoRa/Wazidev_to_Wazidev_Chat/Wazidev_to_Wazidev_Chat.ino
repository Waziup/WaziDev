#include <string.h>
#include <WaziDev.h>

// change your name here:
const char* name = "Max";

WaziDev wazidev;

void setup() {
  // open serial port, set data rate to 9600 bps
  Serial.begin(9600);

  // setup LoRa radio chip
  wazidev.setupLoRa();
  
  Serial.print("Your name is: ");
  Serial.print(name);
  Serial.print("\r\n");
}


// The loop checks if there is data on the serial interface available.
// If yes, it reads one line, and send a text message with lora like this:
//   Name: hello world
// After that, it waits 5 seconds for any incomming message.
void loop() {
  int e;

  // buffer for sending and receiving
  char buf[128];
  uint8_t len;

  // if serial data is available...
  if (Serial.available() > 0)
  {
	// copy name to buffer
    strcpy(buf, name);
    len = strlen(name);
    
	// copy ": " to buffer
    strcpy(buf+len, ": ");
    len += 2;
    
    while (true)
    {
	  // read one line from serial
      if(Serial.available() > 0)
      {
        char c = Serial.read();
        Serial.print(c);
        if (c == '\n' || c == 0)
        {
          buf[len]=0;
          break;
        }
          
        buf[len]=c;
        len++;
      }
    }
	
	// 1.
	// Send LoRa message

    serialPrintf(buf);
    serialPrintf("\r\nSending %d bytes ... ", len);
    e = wazidev.sendLoRa(buf, len, false);
    if (e)
      serialPrintf("Err %d\r\n", e);
    else
      serialPrintf("OK\r\n");
  }
  
  // 2.
  // Receive LoRa message
  // (waiting for 5 seconds only)

  serialPrintf("Receiving ... ");
  e = wazidev.receiveLoRa(buf, &len, 5000, false);
  if (e)
  {
    if (e == ERR_LORA_TIMEOUT)
      serialPrintf("nothing received\r\n");
    else 
      serialPrintf("Err %d\r\n", e);
  }
  else
  {
    serialPrintf("OK\r\n");
    buf[len+1]=0;
    serialPrintf(buf);
    serialPrintf("\r\n");
  }
  

}
