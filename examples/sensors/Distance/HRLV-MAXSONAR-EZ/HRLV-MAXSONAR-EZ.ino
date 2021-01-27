/********************
* HRLV EZ Max sonar 
* Reading analog voltage
* http://www.maxbotix.com
****************/

int sonarPin = A0;
float mm = 0.0;

void setup() {
  Serial.begin(38400);
}

void loop() {
  mm = analogRead(sonarPin) * 5;
  Serial.print(mm);
  Serial.println("mm");
  delay(100);
}
