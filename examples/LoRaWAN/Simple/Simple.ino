
#include <WaziDev.h>

// LoRaWANKey is used as both NwkSKey (Network Session Key) and Appkey (AppKey) for secure LoRaWAN transmission.
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char LoRaWANKey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
// Copy'n'paste the DevAddr (Device Address): 26011D87
unsigned char DevAddr[4] = {0x26, 0x01, 0x1D, 0x87};
// You can change the Key and DevAddr as you want.


WaziDev wazidev;

void setup()
{
  Serial.begin(38400);

  serialPrintf("LoRaWAN setup ...\n");
  uint8_t e = wazidev.setupLoRaWAN(DevAddr, LoRaWANKey);
  if (e != 0)
  {
    serialPrintf("Err %d\n", e);
    while (true);
    return;
  }

  serialPrintf("OK\n");
}

char payload[255];

void loop(void)
{
  strcpy(payload, "hello :D");
  uint8_t len = strlen(payload);
  serialPrintf("Send: [%d] \"%s\"\n", len, payload);


  serialPrintf("LoRaWAN send ... ");
  uint8_t e = wazidev.sendLoRaWAN(payload, len);
  if (e != 0)
  {
    serialPrintf("Err %d\n", e);
    delay(60000);
    return;
  }
  serialPrintf("OK\n");

  
  serialPrintf("LoRa receive ... ");
  uint8_t offs = 0;
  long startSend = millis();
  e = wazidev.receiveLoRaWAN(payload, &offs, &len, 6000);
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
  serialPrintf("Received: [%d] \"%s\"\n", len, payload+offs);
  serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
  serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);

  delay(60000);
}
