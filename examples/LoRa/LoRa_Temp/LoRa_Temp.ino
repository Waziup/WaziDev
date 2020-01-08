/********************
 * Program: Demo temp 
 * Description: sends temperature values to WaziGate
 ********************/
 
#include <WaziDev.h>
#include <Utils.cpp>

#define TEMP_PIN_READ  A0
#define TEMP_SCALE  3300.0

// new WaziDev with node address = 8 
WaziDev wazidev("MyDevice", 8);

void setup()
{
  wazidev.setup();

  // for the temperature sensor
  pinMode(TEMP_PIN_READ, INPUT);

  delay(3000);
  // Open serial communications and wait for port to open:
  Serial.begin(38400);
  // Print a start message
  Serial.println(F("Simple LoRa temperature sensor demo"));
}

void loop(void)
{
  int value = analogRead(TEMP_PIN_READ);
  Serial.print(F("Reading "));
  Serial.println(value);
        
  // change here how the temperature should be computed depending on your sensor type
  //  
  //LM35DZ
  //the LM35DZ needs at least 4v as supply voltage
  //can be used on 5v board
  //temp = (value*TEMP_SCALE/1024.0)/10;

  //TMP36
  //the TMP36 can work with supply voltage of 2.7v-5.5v
  //can be used on 3.3v board
  //we use a 0.95 factor when powering with less than 3.3v, e.g. 3.1v in the average for instance
  //this setting is for 2 AA batteries
  double temp = ((value*0.95*TEMP_SCALE/1024.0)-500)/10;
  
  Serial.print(F("Temp is "));
  Serial.println(temp);

  //Send temperature as sensor "TC"
  wazidev.sendSensorValue("TC", temp);

}
