/********************
 *  Program:  MIC Sensor tester
 *  Description: print audio volume level to serial. Print "Sound" on loud sound.
 ********************/

#define AUDIO_READ_PIN A0 // Analog pin 0 to read the output of the sensor
#define SENSITIVITY 850  

void setup(){
  Serial.begin(115200); // initialize the serial port
}

void loop(){
  int sound_wave =   analogRead(AUDIO_READ_PIN);
  Serial.print(sound_wave);
  
  if (sound_wave > SENSITIVITY) {  // On loud sound
    Serial.println("Sound!");
    delay(500);
  }  
  delay(100); // ms
}
