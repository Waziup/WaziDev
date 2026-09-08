#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <avr/io.h>

// Pin definitions
#define DE_RE_PIN 2      // MAX485 transmit/receive control
#define RX_PIN 10        // Arduino receives from MAX485
#define TX_PIN 11        // Arduino transmits to MAX485

ModbusMaster node;
SoftwareSerial modbusSerial(RX_PIN, TX_PIN); // RX, TX

void printResetReason() {
  if (MCUSR & (1 << BORF)) Serial.println("Brown-out reset");
  if (MCUSR & (1 << WDRF)) Serial.println("Watchdog reset");
  if (MCUSR & (1 << PORF)) Serial.println("Power-on reset");
  MCUSR = 0;
}

void preTransmission() {
  digitalWrite(DE_RE_PIN, HIGH);  // Enable transmit
}

void postTransmission() {
  delayMicroseconds(200);
  digitalWrite(DE_RE_PIN, LOW);   // Enable receive
}

void setup() {
  printResetReason();
  Serial.begin(38400);            // Debug
  Serial.println("Arduino UNO Modbus gestartet");
  delay(5);
  
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);   // Start in receive mode

  modbusSerial.begin(4800);
  // Switch to save baud rate
  static bool baudSet = false;
  if (!baudSet) {
    node.writeSingleRegister(0x07D1, 0);
    baudSet = true;
    delay(3000);
  }
  modbusSerial.begin(2400);
  
  node.begin(1, modbusSerial);    // Slave ID 1
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

void loop() {
  uint8_t result;

  for (uint8_t retry = 0; retry < 2; retry++) {
    delay(20);  // bus idle time
    result = node.readInputRegisters(0x0000, 7);
    if (result == node.ku8MBSuccess) break;
    delay(200);
  }

  if (result == node.ku8MBSuccess) {
    float humidity    = node.getResponseBuffer(0) / 10.0;
    float temperature = node.getResponseBuffer(1) / 10.0;
    uint16_t ec       = node.getResponseBuffer(2);
    float ph          = node.getResponseBuffer(3) / 10.0;
    uint16_t n        = node.getResponseBuffer(4);
    uint16_t p        = node.getResponseBuffer(5);
    uint16_t k        = node.getResponseBuffer(6);

    Serial.print("T="); Serial.print(temperature);
    Serial.print(" H="); Serial.print(humidity);
    Serial.print(" EC="); Serial.print(ec);
    Serial.print(" PH="); Serial.print(ph);
    Serial.print(" N="); Serial.print(n);
    Serial.print(" P="); Serial.print(p);
    Serial.print(" K="); Serial.println(k);
  } else {
    Serial.print("Modbus error: ");
    Serial.println(result);
  }

  delay(10000);
}
