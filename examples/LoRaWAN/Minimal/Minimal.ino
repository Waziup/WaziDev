
#include <WaziDev.h>

unsigned char LoRaWANKey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
unsigned char DevAddr[4] = {0x26, 0x01, 0x1D, 0x87};

WaziDev wazidev;

void setup()
{
    Serial.begin(38400);
    wazidev.setupLoRaWAN(DevAddr, LoRaWANKey);
}

char payload[255];

void loop(void)
{
    strcpy(payload, "\\!TC1/21.4");
    uint8_t len = strlen(payload);

    wazidev.sendLoRaWAN(payload, len);

    uint8_t offs = 0;
    wazidev.receiveLoRaWAN(payload, &offs, &len, 6000);
    if (len != 0)
    {
        serialPrintf("Received: [%d] \"%s\"\n", len, payload + offs);
    }
    delay(60000);
}
