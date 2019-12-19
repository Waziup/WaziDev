/********************
 *  Program:  HC-SR04 sensor tester
 *  Description: print distance to serial
 ********************/

#define ECHO_PIN 4  
#define TRIG_PIN 3
void setup(){
  Serial.begin(38400); // initialize the serial port
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);

  pinMode(2, OUTPUT);  digitalWrite(2, HIGH);
  pinMode(5, OUTPUT);  digitalWrite(5, LOW);
}

// Define a new function that reads and converts the raw reading to distance (cm)
float distance_centimetre() {
  // Send sound pulse
  digitalWrite(TRIG_PIN, HIGH); // pulse started
  delayMicroseconds(12);
  digitalWrite(TRIG_PIN, LOW); // pulse stopped

  // listen for echo 
  float tUs = pulseIn(ECHO_PIN, HIGH); // microseconds
  float distance = tUs / 58; // cm 
  return distance;
}

void loop(){
  Serial.print(distance_centimetre(), DEC);
  Serial.println("cm");
  delay(1000); // ms 
}
