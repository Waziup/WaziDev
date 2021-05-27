#include <WaziDev.h>
#include <xlpp.h>
#include <Base64.h>

unsigned char devAddr[4];
unsigned char appSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
unsigned char nwkSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};

WaziDev wazidev;

float temp = 0;

void revBytes(unsigned char*, size_t);

void setup()
{
  Serial.begin(38400);
  Serial.setTimeout(1000000000);
  Serial.println("Ready");

  Serial.println("Enter the DevAddr: ");
  String in = Serial.readStringUntil('\n');
  Serial.println(in);

  //Convert string to integer
  long value = strtoul(in.c_str(), '\0', 16);
  memcpy(devAddr, (uint8_t*)&value, 4);
  //LoRaWAN is big endian, Arduino is little endian so we need reverse the bytes before sending
  revBytes(devAddr, 4);

  Serial.println("Enter the temperature: ");
  in = Serial.readStringUntil('\n');
  Serial.println(in);
  temp = atoi(in.c_str());

  wazidev.setupLoRaWAN(devAddr, appSkey, nwkSkey);
  
}

XLPP xlpp(120);

void loop(void)
{

  xlpp.reset();
  xlpp.addTemperature(1, temp); // °C
  char payload[100];
  base64_decode(payload, xlpp.getBuffer(), xlpp.len); 
  serialPrintf(payload);
  serialPrintf("\n");

  // 2.
  // Send paload with LoRaWAN.
  serialPrintf("LoRaWAN send ... ");
  Serial.setTimeout(1000000);
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);
  if (e != 0)
  {
    serialPrintf("Err %d\n", e);
    delay(60000);
    return;
  }
  serialPrintf("OK\n");
  
  // 3.
  // Receive LoRaWAN message (waiting for 6 seconds only).
  serialPrintf("LoRa receive ... ");
  uint8_t offs = 0;
  long startSend = millis();
  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, 6000);
  long endSend = millis();
  if (e != 0)
  {
    if (e == ERR_LORA_TIMEOUT){
      serialPrintf("nothing received\n");
    }
    else
    {
      serialPrintf("Err %d\n", e);
    }
    delay(60000);
    return;
  }
  serialPrintf("OK\n");
  
  serialPrintf("Time On Air: %d ms\n", endSend-startSend);
  serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
  serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);
  serialPrintf("LoRaWAN frame size: %d\n", xlpp.offset+xlpp.len);
  serialPrintf("LoRaWAN payload len: %d\n", xlpp.len);
  serialPrintf("Payload: ");
  //char payload[100];
  base64_decode(payload, xlpp.getBuffer(), xlpp.len); 
  serialPrintf(payload);
  serialPrintf("\n");
 
  while(1){}
  //delay(6000);
}

// **********************************************************
// Function to do a byte swap in a byte array
void revBytes(unsigned char* b, size_t c)
{
  int i;
  for (i = 0; i < c / 2; i++)
  {
    unsigned char t = b[i];
    b[i] = b[c - 1 - i];
    b[c - 1 - i] = t;
  }
}

