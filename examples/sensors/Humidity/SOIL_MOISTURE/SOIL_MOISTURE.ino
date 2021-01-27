/********************
 * Soil humidity sensor tester
 * Read soil humidity by measuring its resistance.
 ********************/

int sensorPin = A0;
int soilHumidity = -1;

void setup() {
  Serial.begin(38400);  
}

void loop() {
  soilHumidity = analogRead(sensorPin);
  Serial.println(soilHumidity);
  delay(100);
}
