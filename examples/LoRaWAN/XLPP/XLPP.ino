
#include <WaziDev.h>
#include <xlpp.h>

unsigned char LoRaWANKey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
unsigned char DevAddr[4] = {0x26, 0x01, 0x1D, 0x87};

WaziDev wazidev;

void setup()
{
    Serial.begin(38400);
    wazidev.setupLoRaWAN(DevAddr, LoRaWANKey);
}

XLPP xlpp(120);

void loop(void)
{
    xlpp.reset();
    xlpp.addTemperature(1, 20.3); // °C
    xlpp.addVoltage(2, 12.5); // V
    xlpp.addString(3, "Hello :D");

    serialPrintf("LoRaWAN send ... ");
    int e = wazidev.sendLoRaWAN(xlpp.getBuffer(), xlpp.getSize());
    if (e == 0)
        serialPrintf("OK\n");
    else
        serialPrintf("Err %d\n", e);
    delay(60000);
}
