#include <WaziDev.h>
#include <xlpp.h>

// LoRaWANKey is used as both NwkSKey (Network Session Key) and Appkey (AppKey) for secure LoRaWAN transmission.
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char LoRaWANKeys[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
// Copy'n'paste the DevAddr (Device Address): 26011D87
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0xE7};
// You can change the Key and DevAddr as you want.

WaziDev wazidev;

// globals
const int interval = 10000; //10 sec
const int interval_rep = 5;
const int relayPin = 5;
const int mosfetPin = 6; // for power

void setup()
{
  Serial.begin(38400);
  wazidev.setupLoRaWAN(devAddr, LoRaWANKeys);
  pinMode(relayPin, OUTPUT);
  delay(1000);
  pinMode(mosfetPin, OUTPUT);
  delay(1000);
  digitalWrite(relayPin, LOW);
  delay(1000);
  digitalWrite(mosfetPin, HIGH);
  delay(2000);
  serialPrintf("Setup done...");
}

XLPP xlpp(120);


uint8_t uplink()
{
  // 1
  // Create xlpp payload.
  xlpp.reset();

  // 2
  // Add actuator
  xlpp.addActuators(1, LPP_SWITCH);

  // 2.
  // Send payload with LoRaWAN.
  serialPrintf("LoRaWAN send ... ");
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);
  if (e != 0)
  {
    serialPrintf("Err %d\n", e);
    delay(interval);
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

  serialPrintf("Time On Air: %d ms\n", endSend - startSend);
  serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
  serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);
  serialPrintf("LoRaWAN frame size: %d\n", xlpp.offset + xlpp.len);
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
    serialPrintf("Type %2d: ", type);

    // [3] ... then the value!
    switch (type) {
      case XLPP_BOOL_FALSE:
        {
          digitalWrite(relayPin, LOW);
          serialPrintf("Switch relay off\n");
          break;
        }
      case XLPP_BOOL_TRUE:
        {
          digitalWrite(relayPin, HIGH);
          serialPrintf("Switch relay on\n");
          break;
        }
      case XLPP_BOOL:
        {
          digitalWrite(relayPin, xlpp.getBool());
          serialPrintf("Switch relay %s", xlpp.getBool() ? "true" : "false");
          break;
        }
      case LPP_SWITCH:
        {
          digitalWrite(relayPin, xlpp.getSwitch());
          serialPrintf("Switch relay %s", xlpp.getSwitch() ? "true" : "false");
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
    // waiting for interval time only! => LoRa chip cannot be turned on longer than 10sec
    for(int i = 0; i <= interval_rep; i++){
      downlink(interval);
      Serial.println("repeat_interval: " + String(i));
    }
  }
  else {
    Serial.println("Error: " + e);
  }
}
