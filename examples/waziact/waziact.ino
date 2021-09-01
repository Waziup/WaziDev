#include <WaziDev.h>
#include <xlpp.h>
#include <Base64.h>

/*---------*/

// NwkSKey (Network Session Key) and Appkey (AppKey) are used for securing LoRaWAN transmissions.
// You need to copy them from/to your LoRaWAN server or gateway.
// You need to configure also the devAddr. DevAddr need to be different for each devices!!
// Copy'n'paste the DevAddr (Device Address): 26011D00
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0x00};

// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char appSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};

// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char nwkSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};

WaziDev wazidev;

/*-------*/

XLPP xlpp(120);

/*-------*/

// To save power we use a digital pin to power on the ExternalSensor,
// if it supports it, whenever we need to read it
#define ExternalSensorDataPin A6
#define ExternalSensorPowerPin 6

/*-------*/

#define LEDPin 8
#define RelayPin 7

/*-----------------------------*/

// This function reads an analogue pin couple of times
// and makes an average of it to have more accuracy
unsigned int analogReadAvg(unsigned int pin)
{
#define TotalReads 10

  unsigned int sum = 0;
  for (int i = 0; i != TotalReads; i++)
  {
    delay(50);
    sum += analogRead(pin);
  }

  return sum / TotalReads;
}

/*-----------------------------*/

void setup()
{
  Serial.begin(38400);

  /*---------*/

  uint8_t errSetup = wazidev.setupLoRaWAN(devAddr, appSkey, nwkSkey);
  if (errSetup != 0)
  {
    serialPrintf("LoRaWAN Err %d\n", errSetup);
    delay(60000);
    return;
  }

  /*--------*/

  pinMode(LEDPin, OUTPUT);
  pinMode(RelayPin, OUTPUT);
  pinMode(ExternalSensorPowerPin, OUTPUT); // To power up the sensor

  /*--------*/

  // Startup blink
  for (int i = 0; i != 3; i++)
  {
    digitalWrite(LEDPin, HIGH);
    delay(50);
    digitalWrite(LEDPin, LOW);
    delay(50);
  }
}

/*-----------------------------*/

void loop(void)
{
  // 1
  // Create xlpp payload, we may attach some sensor to the actuation node as well
  xlpp.reset();
  xlpp.addTemperature(1, 45.7); // °C

  // 2.
  // Send payload with LoRaWAN.
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
    if (e == ERR_LORA_TIMEOUT)
    {
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

  serialPrintf("Time On Air: %d ms\n", endSend - startSend);
  serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
  serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);
  serialPrintf("LoRaWAN frame size: %d\n", xlpp.offset + xlpp.len);
  serialPrintf("LoRaWAN payload len: %d\n", xlpp.len);
  serialPrintf("Payload: ");
  char payload[100];
  base64_decode(payload, xlpp.getBuffer(), xlpp.len);
  serialPrintf(payload);
  serialPrintf("\n");

  // Triggering the relay

  // atoi( payload)
  // payload

  delay(2000);
}
