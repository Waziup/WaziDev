#include <WaziDev.h>
#include <xlpp.h>

unsigned char loRaWANKey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0x26};

WaziDev wazidev;


void setup()
{
    delay(3000); 
    // Open serial communications and wait for port to open: 
    Serial.begin(38400);   
    delay(500); 
    wazidev.setupLoRaWAN(devAddr, loRaWANKey);
    sx1272.setChannel(0x6C4B33);
}

XLPP xlpp(120);

void loop(void)
{
    xlpp.reset();
    xlpp.addTemperature(1, 0.3); // °C
    xlpp.addVoltage(2, 230.5);     // V
    xlpp.addString(3, "Bye :D");

    serialPrintf("LoRaWAN send ... ");
    int e = wazidev.sendLoRaWAN(xlpp.getBuffer(), xlpp.getSize());
    if (e == 0)
        serialPrintf("OK\n");
    else
        serialPrintf("Err %d\n", e);
    delay(10000); // not within the LoRa specifiction (!!on the air time to high!!, just lab testing)
}
