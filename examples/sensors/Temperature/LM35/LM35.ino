/********************
 *  Program:  LM35 sensor tester
 *
 *  Description: Reads the voltage from a LM35 temperature
 *  sensor on pin A0 of the Arduino. Converts the voltage to 
 *  a temperature and sends it out of the serial port for 
 *  display on the serial monitor.
 ********************/

// Use analog pin 0 (A0) to read the output of the sensor
#define TEMP_PIN_READ A0
 
void setup(){
  Serial.begin(38400); // initialize the serial port
}

// Define a new function that reads and converts the raw reading to temperature
float temperature_celcius() { 
  return analogRead(TEMP_PIN_READ) * (5.0 / 1023.0 * 100.0); 
}

void loop(){
  Serial.print(temperature_celcius());
  Serial.println("°C");
  delay(250);
}
