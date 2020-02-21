/********************
 * Program:  DHT11 sensor tester
 * Description: sends humidity and temperature values to WaziGate
 ********************/
 
#include <WaziDev.h>
#include <DHT.h>


#define DHTPIN 2        // what pin on the arduino is the DHT data line connected to
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

// new WaziDev with node address = 8 
WaziDev wazidev("MyDevice", 8);

void setup() { // to run once
  wazidev.setup();
  Serial.println("DHT11 Humidity - Temperature Sensor");

//  pinMode(5, OUTPUT);  digitalWrite(5, LOW);
  pinMode(3, OUTPUT);  digitalWrite(3, HIGH);
  
  dht.begin();
  delay(2000);
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT11 sensor!");
    return;
  }

  WaziDev::SensorVal fields[2] = {{"TC", t}, {"HUM", h}};
  //Send temperature as sensor "TC"
  wazidev.sendSensorValues(fields, 2);

  char tf[10]; ftoa(tf, t, 2); 
  char hf[10]; ftoa(hf, h, 2);
  wazidev.writeSerial("%s %% %s °C\n", hf, tf);
  Serial.flush();
  // Wait a few seconds between measurements. The DHT11 should not be read at a higher frequency of
  // about once every 2 seconds. So we add a 3 second delay to cover this.
  delay(3000);
}
