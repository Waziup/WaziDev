/********************
 *  Program:  808H5V5 sensor tester
 *  http://www.cooking-hacks.com/skin/frontend/default/cooking/pdf/Humedad-808H5V5.pdf
 ********************/

float hum = 0;
float voltage = 0;
float percentRH = 0;

void setup() {
  // to run once
  Serial.begin(38400); // Initialize the serial port
}

void loop() {
  hum = analogRead(0); // Read voltage coming from sensor (value will be between 0-1023)
  voltage = hum * 5.0 / 1024.0; // Translate value into a voltage value
  percentRH = (voltage - 0.788) / 0.0314; // Translate voltage into percent relative humidity

  Serial.print("Read value is ");
  Serial.print(hum);
  Serial.print(".   Output voltage is ");
  Serial.print(voltage);
  Serial.print(" .    %RH = ");
  Serial.println(percentRH,DEC); 
  delay(2000);
}
