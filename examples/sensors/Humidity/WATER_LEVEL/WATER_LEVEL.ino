/********************
 * Water level sensor tester
 * measuring the capacitance of water and hence the water level.
 ********************/
int sensorPin = A0; 
int sensorValue = 0;
 
void setup() {
 Serial.begin(38400); 
}
 
void loop() {
 sensorValue = analogRead(sensorPin); 
 
 Serial.print("Sensing = " ); 
 Serial.print(sensorValue*100/1024); 
 Serial.println("%");
 
 delay(1000); 
}
