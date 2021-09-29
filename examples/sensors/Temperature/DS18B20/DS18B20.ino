/********************
 * DS18B20 temperature sensor tester
 * https://create.arduino.cc/projecthub/TheGadgetBoy/ds18b20-digital-temperature-sensor-and-arduino-9cc806
 ********************/

// First we include the libraries
// you have to install the "OneWire" library published by Paul Stoffregen from Tools > Library Manager
#include <OneWire.h>
// you have to install the "DallasTemperature" library published by Miles Burton from Tools > Library Manager
#include <DallasTemperature.h>
 
// Data wire is plugged into pin 2 on the Arduino 
#define ONE_WIRE_BUS 2 

// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

void setup() 
{ 
 // start serial port 
 Serial.begin(38400); 
 Serial.println("Dallas Temperature IC Control Library Demo"); 
 // Start up the library 
 sensors.begin(); 
} 

void loop() 
{ 
 // call sensors.requestTemperatures() to issue a global temperature 
 // request to all devices on the bus 

 Serial.print(" Requesting temperatures..."); 
 sensors.requestTemperatures(); // Send the command to get temperature readings 
 Serial.println("DONE"); 

 Serial.print("Temperature is: "); 
 Serial.print(sensors.getTempCByIndex(0)); // Why "byIndex"?  
   // You can have more than one DS18B20 on the same bus.  
   // 0 refers to the first IC on the wire 
 delay(1000); 
}
