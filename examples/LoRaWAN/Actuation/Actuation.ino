#include <WaziDev.h>
#include <xlpp.h>

// LoRaWANKey is used as both NwkSKey (Network Session Key) and Appkey (AppKey) for secure LoRaWAN transmission.
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char LoRaWANKey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
// Copy'n'paste the DevAddr (Device Address): 26011D88
unsigned char DevAddr[4] = {0x26, 0x01, 0x1D, 0x88};
// You can change the Key and DevAddr as you want.


// A valid payload for the downlink for this sketch has been generated like this:
// > xlpp -e '{"colour0":"#ffaa00","switch1":true}'
// < AIf/qgABjgE=
// You can paste the base64 payload to Chirpstack.
// On Windows, you might need to escape it like this:
// > xlpp -e "{"""colour0""":"""#ffaa00""","""switch1""":true}"


WaziDev wazidev;

void setup()
{
    Serial.begin(38400);
    wazidev.setupLoRaWAN(DevAddr, LoRaWANKey);
}

XLPP xlpp(120);

void loop(void)
{
  // 1
  // Create xlpp payload.
  xlpp.reset();
  xlpp.addTemperature(1, 20.3); // °C

  // 2.
  // Send paload with LoRaWAN.
  serialPrintf("LoRaWAN send ... ");
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
  printBase64(xlpp.getBuffer(), xlpp.len);
  serialPrintf("\n");

  // 4.
  // Read xlpp message.
  // You must use the following pattern to properly parse xlpp payload.
  int end = xlpp.len + xlpp.offset;
  while (xlpp.offset < end)
  {
    // Always read the channel first ...
    uint8_t chan = xlpp.getChannel();
    serialPrintf("Chan %2d: ", chan);

    // ... then the type ...
    uint8_t type = xlpp.getType();

    // ... then the value!
    switch (type) {
      case LPP_DIGITAL_OUTPUT:
      {
        uint8_t v = xlpp.getDigitalInput();
        serialPrintf("Digital Output: %hu (0x%02x)\n", v, v);
        break;
      }
      case LPP_ANALOG_OUTPUT:
      {
        float f = xlpp.getAnalogOutput();
        serialPrintf("Analog Output: %.2f\n", f);
        break;
      }
      case LPP_COLOUR:
      {
        Colour c = xlpp.getColour();
        serialPrintf("Colour: R %d, G %d, B %d (#%02X%02X%02X)\n", c.r, c.g, c.b, c.r, c.g, c.b);
        break;
      }
      case LPP_SWITCH:
      {
        uint8_t v = xlpp.getSwitch();
        serialPrintf("Switch: %s\n", v?"on":"off");
        break;
      }
      default:
        // For all the other types, see https://github.com/Waziup/arduino-xlpp/blob/main/test/simple/main.cpp#L147
        serialPrintf("Other unknown type.\n");
        delay(60000);
        return;
    }
  }
  
  delay(60000);
}
