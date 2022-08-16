#include <WaziDev.h>
#include <xlpp.h>
#include <Base64.h>
#include <simpleRPC.h>

// Copy'n'paste the DevAddr (Device Address): 26011D22
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0x22};
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char appSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char nwkSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};

WaziDev wazidev;
XLPP xlpp(120);
char payload[100] = "";

void setup()
{
  Serial.begin(9600);
  wazidev.setupLoRaWAN(devAddr, appSkey, nwkSkey);
}

//Interface to sending LoRaWAN frames
Object<uint8_t, char *> sendLoRaWAN(int temp)
{
  xlpp.reset();
  xlpp.addTemperature(1, temp);
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);

  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, 3000);
  if (e == 0)
  {
    base64_decode(payload, xlpp.getBuffer(), xlpp.len);
  } else {
    payload[0] = '\0';
  }
  return {e, payload};
}

//Interface to sending LoRaWAN frames (simple version)
char *sendLoRaWAN2(int temp)
{
  char *res = malloc(50); 
  xlpp.reset();
  xlpp.addTemperature(1, temp);
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);

  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, 1000);
  switch (e)
  {
    case 0:
      base64_decode(payload, xlpp.getBuffer(), xlpp.len);
      sprintf(res, "Received: %s", payload);
      break;
    case 2: 
      sprintf(res, "Error: Nothing received");
      break;
    default:
      sprintf(res, "Error: %d", e);
      break;
  }
  return res;
}

void loop(void)
{
  interface(Serial, sendLoRaWAN, "sendLoRaWAN: Send a LoRaWAN value. @value: int @return: string",
                    sendLoRaWAN2, "sendLoRaWAN2: Send a LoRaWAN value. @value: int @return: string");
}

