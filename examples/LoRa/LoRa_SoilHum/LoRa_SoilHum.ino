/********************
 * Program: Soil humidity 
 * Description: sends soil humidity values to WaziGate
 ********************/
 
#include <WaziDev.h>

int sensorPin = A0;

// new WaziDev with node address = 8 
WaziDev wazidev("MyDevice", 8);

void setup()
{
  wazidev.setup();
  Serial.println("Soil Humidity Sensor");

  delay(500);
}

void loop(void)
{
  int soilHumidity = analogRead(sensorPin);
  Serial.println(soilHumidity);

  //Send humidity as sensor "SM1"
  wazidev.sendSensorValue("SM1", soilHumidity);

  delay(50);
}
