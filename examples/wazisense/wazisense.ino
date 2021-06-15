
#include <Wire.h>
#include "i2c.h"

/*-------*/

#include <xlpp.h>
XLPP xlpp(120);

/*-------*/

#include "i2c_SI7021.h"
SI7021 si7021;

/*-------*/

#include "WaziDev.h"
WaziDev wazidev;


/*-----------------------------*/


void setup()
{
  Serial.begin(38400);

  /*-----------*/
  
  unsigned char LoRaWANKey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25}; // 23158D3BBC31E6AF670D195B5AED5525
  unsigned char DevAddr[4] = {0x26, 0x01, 0x1D, 0x87}; // 26011D87
  
  /*-----------*/

    uint8_t errSetup = wazidev.setupLoRaWAN(DevAddr, LoRaWANKey);
    if (errSetup != 0)
    {
      serialPrintf("LoRaWAN Err %d\n", errSetup);
      delay(60000);
      return;
    }

    /*-----------*/

    pinMode(8, OUTPUT);

    /*-----------*/
      
    serialPrintf("Probe SI7021: ");
    if (si7021.initialize()) {
      serialPrintf("Sensor found!\n");
    
    }else{
      
        serialPrintf("Sensor missing\n");
        while(1) {delay(1*60*1000);};
    }

    /*-----------*/

    for( int i = 0; i != 3; i++){
      
      digitalWrite(8, HIGH);delay(50);
      digitalWrite(8, LOW); delay(50);
      
    }

}

char payload[255];

void loop(void)
{ 

    serialPrintf("\n\n---------------------\n\n");

    digitalWrite(8, HIGH);delay(50);
    digitalWrite(8, LOW); delay(50);
    
    /*----------*/

    static float humidity, temperature;

    si7021.getHumidity(humidity);
    si7021.getTemperature(temperature);
    si7021.triggerMeasurement();

    serialPrintf("TEMP: %d.%02d \t HUMI: %d.%02d\n", (int)temperature, (int)(temperature*100)%100, (int)humidity, (int)(humidity*100)%100);

    /*----------*/

    xlpp.reset();
    xlpp.addRelativeHumidity(2, humidity);
    xlpp.addTemperature(1, temperature);
    

    serialPrintf("LoRaWAN send ... ");
    int e = wazidev.sendLoRaWAN(xlpp.getBuffer(), xlpp.getSize());
    if (e == 0){
        serialPrintf("OK\n");
        
    }else{
      
        serialPrintf("Err %d\n", e);
    }
    

    delay(30*60*1000); // wait before pushing, we need to use sleep mode for a better battery performance
}
