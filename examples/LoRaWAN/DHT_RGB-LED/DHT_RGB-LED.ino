#include <WaziDev.h>
#include <xlpp.h>
#include <DHT.h>

// RGB-LED analog PWM pins: red, gree, blue
#define LED_RED_PIN 3
#define LED_GREEN_PIN 5
#define LED_BLUE_PIN 6

// DHT analog pin
#define DHT_PIN 2



// LoRaWANKey is used as both NwkSKey (Network Session Key) and Appkey (AppKey) for secure LoRaWAN transmission.
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char LoRaWANKey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
// Copy'n'paste the DevAddr (Device Address): 26011D89
unsigned char DevAddr[4] = {0x26, 0x01, 0x1D, 0x89};
// You can change the Key and DevAddr as you want.


// A valid payload for the downlink for this sketch has been generated like this:
// > xlpp -e '{"colour0":"#ffaa00"}'
// < AIf/qgA=
// You can paste the base64 payload to Chirpstack.
// On Windows, you might need to escape it like this:
// > xlpp -e "{"""colour0""":"""#ffaa00"""}"


WaziDev wazidev;

DHT dht(DHT_PIN, DHT11);


void setup()
{
    Serial.begin(38400);
    wazidev.setupLoRaWAN(DevAddr, LoRaWANKey);
}

XLPP xlpp(120);

void loop(void)
{
  delay(2000);

  // 1.
  // Read sensor values.
  float humidity = dht.readHumidity(); // %
  float temperature = dht.readTemperature(); // °C

  // 2.
  // Create xlpp payload for uplink.
  xlpp.reset();
  xlpp.addRelativeHumidity(0, humidity);
  xlpp.addTemperature(0, temperature);

  // 2.
  // Send paload uplink with LoRaWAN.
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
  // Receive LoRaWAN downlink message (waiting for 6 seconds only).
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
  // Read xlpp downlink message.
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
      case LPP_COLOUR:
      {
        Colour c = xlpp.getColour();
        serialPrintf("Colour: R %d, G %d, B %d (#%02X%02X%02X)\n", c.r, c.g, c.b, c.r, c.g, c.b);
        analogWrite(LED_RED_PIN, c.r);
        analogWrite(LED_GREEN_PIN, c.g);
        analogWrite(LED_BLUE_PIN, c.b);
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
