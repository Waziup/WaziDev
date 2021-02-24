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

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  dht.begin(); // start DHT sensor
  delay(2000);
}

XLPP xlpp(120);

uint8_t uplink()
{
  uint8_t e;

  // 1.
  // Read sensor values.
  float humidity = dht.readHumidity(); // %
  float temperature = dht.readTemperature(); // °C

  // 2.
  // Create xlpp payload for uplink.
  xlpp.reset();
  
  // Add sensor payload
  xlpp.addRelativeHumidity(0, humidity);
  xlpp.addTemperature(0, temperature);
  
  // Declare the actuator to the Gateway (optional) so that it will appear on the Dashboard
  // and you don't need to configure it yourself.
  xlpp.addActuators(1,
    LPP_COLOUR
  );

  // 3.
  // Send payload uplink with LoRaWAN.
  serialPrintf("LoRaWAN send ... ");
  e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);
  if (e != 0)
  {
    serialPrintf("Err %d\n", e);
    return e;
  }
  serialPrintf("OK\n");
  return 0;
}

uint8_t downlink(uint16_t timeout)
{
  uint8_t e;

  // 1.
  // Receive LoRaWAN downlink message.
  serialPrintf("LoRa receive ... ");
  uint8_t offs = 0;
  long startSend = millis();
  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, timeout);
  long endSend = millis();
  if (e)
  {
    if (e == ERR_LORA_TIMEOUT)
      serialPrintf("nothing received\n");
    else 
      serialPrintf("Err %d\n", e);
    return e;
  }
  serialPrintf("OK\n");
  
  serialPrintf("Time On Air: %d ms\n", endSend-startSend);
  serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
  serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);
  serialPrintf("LoRaWAN frame size: %d\n", xlpp.offset+xlpp.len);
  serialPrintf("LoRaWAN payload len: %d\n", xlpp.len);
  serialPrintf("Payload: ");
  if (xlpp.len == 0)
  {
    serialPrintf("(no payload received)\n");
    return 1;
  }
  printBase64(xlpp.getBuffer(), xlpp.len);
  serialPrintf("\n");

  // 2.
  // Read xlpp payload from downlink message.
  // You must use the following pattern to properly parse xlpp payload!
  int end = xlpp.len + xlpp.offset;
  while (xlpp.offset < end)
  {
    // [1] Always read the channel first ...
    uint8_t chan = xlpp.getChannel();
    serialPrintf("Chan %2d: ", chan);

    // [2] ... then the type ...
    uint8_t type = xlpp.getType();

    // [3] ... then the value!
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
        return 1;
    }
  }
}

void loop(void)
{
  // error indicator
  uint8_t e;

  // 1. LoRaWAN Uplink
  e = uplink();
  // if no error...
  if (!e) {
    // 2. LoRaWAN Downlink
    // waiting for 6 seconds only!
    downlink(6000);
  }

  serialPrintf("Waiting 1min ...\n");
  delay(60000);
}
