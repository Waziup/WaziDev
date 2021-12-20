#include <WaziDev.h>
#include <xlpp.h>
#include <Base64.h>
#include <simpleRPC.h>

unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0x22};
unsigned char appSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
unsigned char nwkSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};

WaziDev wazidev;
XLPP xlpp(120);

void setup()
{
  Serial.begin(9600);
  wazidev.setupLoRaWAN(devAddr, appSkey, nwkSkey);
}

void loop(void)
{
  interface(Serial, sendLoRaWAN, "sendLoRaWAN: Send a LoRaWAN value. @value: int @return: string");
}

char* sendLoRaWAN(int temp)
{
  xlpp.reset();
  xlpp.addTemperature(1, temp);
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);

  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, 6000);
  if (e != 0)
  {
    if (e == ERR_LORA_TIMEOUT){
      return("Nothing received");
    } else {
      return("Err %d", e);
    }
  }
  char payload[100];
  base64_decode(payload, xlpp.getBuffer(), xlpp.len);
  return payload;
}


