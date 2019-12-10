#include "wazidev.h"

// new WaziDev with node address = 8 
WaziDev wazidev = new WaziDev(8);

void setup()
{
  wazidev.setup();
}

void loop(void)
{
  wazidev.send("TC1", 30.0);
  wazidev.powerDown(5);
  delay(5000);
}
