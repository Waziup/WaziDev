/********************
 * Program:  Photocell simple testing sketch.
 * Connect one end of the photocell to 5V, the other end to Analog 0.
 * Then connect one end of a 10K resistor from Analog 0 to ground
 * For more information see http://learn.adafruit.com/photocells
 ********************/

int photocellPin = 0; // the cell and 10K pulldown are connected to A0
int photocellReading; // the analog reading from the analog resistor divider

void setup() {
  // We'll send debugging information via the Serial monitor
  Serial.begin(38400);
}

void loop() {
  photocellReading = analogRead(photocellPin);
  Serial.print("Analog reading = ");
  Serial.print(photocellReading); // the raw analog reading
  // We'll have a few threshholds, qualitatively determined
  if (photocellReading < 10) {
    Serial.println(" - Black");
  } else if (photocellReading < 200) {
    Serial.println(" - Dark");
  } else if (photocellReading < 500) {
    Serial.println(" - Light");
  } else if (photocellReading < 800) {
    Serial.println(" - Luminous");
  } else {
    Serial.println(" - Bright");
  }
  delay(2000);
}
