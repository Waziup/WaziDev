#include <WaziDev.h>
#include <xlpp.h>
#include <Base64.h>
#include <simpleRPC.h>

// Copy'n'paste the DevAddr (Device Address): 26011D22
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0x23};
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char appSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char nwkSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};

WaziDev wazidev;
XLPP xlpp(120);
char result[100] = "";

void setup()
{
  Serial.begin(9600);
  wazidev.setupLoRaWAN(devAddr, appSkey, nwkSkey);
  wazidev.setLoRaSF(12);

}

//Interface to sending LoRaWAN frames
Object<uint8_t, char *> sendLoRaWAN(int temp)
{
  xlpp.reset();
  xlpp.addTemperature(1, temp);
  xlpp.addActuators(1, LPP_ANALOG_OUTPUT);
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);

  xlpp.reset();
  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, 3000);
  if (e == 0)
  {
    base64_decode(result, xlpp.getBuffer(), xlpp.len);
  } else {
    result[0] = '\0';
  }
  return {e, result};
}

//Interface to sending LoRaWAN frames (simple version)
char *sendLoRaWAN2(int temp)
{
  xlpp.reset();
  xlpp.addTemperature(1, temp);
  xlpp.addActuators(1, LPP_ANALOG_OUTPUT);
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);

  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, 5000);
  switch (e)
  {
    case 0:
      if (xlpp.len == 0) {
        sprintf(result, "Downlink received (no data).");
      } else {
        char payload [50];
        to_hex_string(payload, xlpp.getBuffer(), xlpp.len);
        sprintf(result, "Downlink received. Payload: 0x%s, with length %d", payload, xlpp.len);
      }
      break;
    case 2: 
      sprintf(result, "Error: Nothing received");
      break;
    default:
      sprintf(result, "Error: %d", e);
      break;
  }
  return result;
}

void to_hex_string(unsigned char *outstr, const unsigned char *array, size_t length)
{
    outstr[0] = '\0';
    char *p = outstr;
    for (size_t i = 0;  i < length;  ++i) {
      p += sprintf(p, "%02hhx", array[i]);
    }
}

void loop(void)
{
  interface(Serial, sendLoRaWAN, "sendLoRaWAN: Send a LoRaWAN value. @value: int @return: string",
                    sendLoRaWAN2, "sendLoRaWAN2: Send a LoRaWAN value. @value: int @return: string");
}

