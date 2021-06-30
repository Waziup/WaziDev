
#include "WaziDev.h"

WaziDev wazidev;

void setup()
{
  pinMode(13, OUTPUT);
  pinMode(8, OUTPUT);

  Serial.begin(38400);
}

char payload[255];

void loop(void)
{

  for (int i = 0; i != 5; i++)
  {
    serialPrintf("#%d LED Blink test\n", i + 1);
    digitalWrite(13, HIGH);
    digitalWrite(8, LOW);
    delay(1000);
    digitalWrite(13, LOW);
    digitalWrite(8, HIGH);
    delay(1000);
  }

  serialPrintf("\n\n---------------------\n\n");

  //Testing LoRawan

  unsigned char LoRaWANKey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
  unsigned char DevAddr[4] = {0x26, 0x01, 0x1D, 0x87};

  serialPrintf("LoRaWAN setup... \n");
  uint8_t errSetup = wazidev.setupLoRaWAN(DevAddr, LoRaWANKey);
  if (errSetup != 0)
  {
    serialPrintf("Err %d\n", errSetup);
    delay(60000);
    return;
  }
  serialPrintf("OK\n");

  strcpy(payload, "\\!TC1/21.4");
  uint8_t len = strlen(payload);

  serialPrintf("Send: [%d] \"%s\"\n", len, payload);

  serialPrintf("LoRaWAN send ... ");
  uint8_t errSend = wazidev.sendLoRaWAN(payload, len);
  if (errSend != 0)
  {
    serialPrintf("Err %d\n", errSend);
    delay(60000);
    return;
  }
  serialPrintf("OK\n");

  serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
  serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);

  delay(600000);
}
