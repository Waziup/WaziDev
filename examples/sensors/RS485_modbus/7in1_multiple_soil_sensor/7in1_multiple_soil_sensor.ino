#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include <LowPower.h>

// ---------------- PIN DEFINITIONS ----------------
#define DE_RE_PIN 2
#define RX_PIN 10
#define TX_PIN 11

// ---------------- MODBUS ----------------
SoftwareSerial modbusSerial(RX_PIN, TX_PIN);
ModbusMaster node;

// ---------------- SENSOR CONFIG ----------------
#define NUM_SENSORS 1
uint8_t sensorIDs[NUM_SENSORS] = {1}; // depths: shallow, mid, deep

#define MODBUS_RETRIES 3
#define RETRY_DELAY_MS 1000

// ---------------- DATA STRUCT ----------------
struct SoilData {
  float temperature;
  float humidity;
  uint16_t ec;
  float ph;
  uint16_t n;
  uint16_t p;
  uint16_t k;
};

SoilData soil[NUM_SENSORS];

// ---------------- RS485 CONTROL ----------------
void preTransmission() {
  digitalWrite(DE_RE_PIN, HIGH);
}

void postTransmission() {
  delayMicroseconds(300);   // important for SoftwareSerial stability
  digitalWrite(DE_RE_PIN, LOW);
}

// ---------------- READ ONE SENSOR ----------------
bool readSensor(uint8_t id, SoilData &data) {
  node.begin(id, modbusSerial);

  for (uint8_t attempt = 1; attempt <= MODBUS_RETRIES; attempt++) {

    uint8_t result = node.readInputRegisters(0x0000, 7);

    if (result == node.ku8MBSuccess) {
      data.humidity    = node.getResponseBuffer(0) / 10.0;
      data.temperature = node.getResponseBuffer(1) / 10.0;
      data.ec          = node.getResponseBuffer(2);
      data.ph          = node.getResponseBuffer(3) / 10.0;
      data.n           = node.getResponseBuffer(4);
      data.p           = node.getResponseBuffer(5);
      data.k           = node.getResponseBuffer(6);
      return true;
    }

    // Retry handling
    if (attempt < MODBUS_RETRIES) {
      delay(RETRY_DELAY_MS);
    } else {
      Serial.print("Sensor "); Serial.print(id);
      Serial.print(" failed after ");
      Serial.print(MODBUS_RETRIES);
      Serial.print(" attempts. Last error: ");
      Serial.println(result);
    }
  }

  return false;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(38400);
  Serial.println("Soil RS485 system booting");
  delay(5);

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  modbusSerial.begin(4800);

  // Switch to save baud rate
  node.writeSingleRegister(0x07D1, 0);
  delay(1000);
  modbusSerial.begin(2400);

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

// ---------------- MAIN LOOP ----------------
void loop() {

  Serial.println("Waking up, reading sensors...");

  for (int i = 0; i < NUM_SENSORS; i++) {
    if (readSensor(sensorIDs[i], soil[i])) {
      Serial.print("Sensor "); Serial.print(sensorIDs[i]); Serial.print(": ");
      Serial.print("T="); Serial.print(soil[i].temperature);
      Serial.print(" H="); Serial.print(soil[i].humidity);
      Serial.print(" EC="); Serial.print(soil[i].ec);
      Serial.print(" PH="); Serial.print(soil[i].ph);
      Serial.print(" N="); Serial.print(soil[i].n);
      Serial.print(" P="); Serial.print(soil[i].p);
      Serial.print(" K="); Serial.println(soil[i].k);
    }
    delay(3000); // small gap between sensors
  }

  // ---------------- LORA PLACEHOLDER ----------------
  // transmitSoilData(soil, NUM_SENSORS);

  Serial.println("Measurement done. Sleeping...");
  delay(300);

  // ---------------- SLEEP ~1 MIN ----------------
  for (int i = 0; i < 15; i++) {
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  }
}
