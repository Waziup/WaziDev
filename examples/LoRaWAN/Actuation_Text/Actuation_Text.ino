#include <WaziDev.h>
#include <xlpp.h>

// LoRaWANKey is used as both NwkSKey (Network Session Key) and Appkey (AppKey) for secure LoRaWAN transmission.
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char LoRaWANKeys[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
// Copy'n'paste the DevAddr (Device Address): 26011D87
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0x88};
// You can change the Key and DevAddr as you want.

WaziDev wazidev;

void setup()
{
    Serial.begin(38400);
    wazidev.setupLoRaWAN(devAddr, LoRaWANKeys);
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
  uint8_t offset;
  uint8_t len;
  uint8_t buf[255];
  e = wazidev.receiveLoRaWAN(buf, &offset, &len, 6000);
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

  // 4.
  // Read message.
  serialPrintf("Message: %s (%d Bytes)\n", buf+offset, len);
  
  delay(60000);
}
