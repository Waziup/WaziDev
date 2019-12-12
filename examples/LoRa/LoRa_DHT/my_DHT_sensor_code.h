#ifndef MY_DHT_SENSOR_CODE
#define MY_DHT_SENSOR_CODE

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include "DHT.h"

extern char nomenclature_str[4];
void sensor_Init();
double sensor_getValue();

///////////////////////////////////////////////////////////////////
// CHANGE HERE THE READ PIN AND THE POWER PIN FOR THE TEMP. SENSOR
#define PIN_READ  2
#define PIN_POWER 3
///////////////////////////////////////////////////////////////////

#define DHTTYPE DHT11       // DHT 11 
//#define DHTTYPE DHT22         // DHT 22  (AM2302 & AM2305)

#endif
