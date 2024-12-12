// Arduino program to read values from a capacitive soil moisture sensor
#include <WaziDev.h>
#include <xlpp.h>
#include <LowPower.h>
#include <Vcc.h> 

WaziDev wazidev;

unsigned char LoRaWANKeys[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0xC8};

const int sleep_sec = 1800;//1800 // Time in sec in sleep mode DEBUG

const int ledPin = 8;
const int totalBlinks = 20;
const int initialDelay = 100;
const int finalDelay = 10;

XLPP xlpp(40);

// Define the analog pin for the soil moisture sensor
const int moisturePin = A0;

// Define the digital pin to supply power to the sensor
const int sensorPowerPin = 5;

// Variable to store the sensor value
int sensorValue;

// Replace these with your calibration values
const int dryValue = 160;  
const int wetValue = 0;

void blink_led() {
    for (int i = 0; i < totalBlinks; i++) {
        digitalWrite(ledPin, HIGH);
        delay(map(i, 0, totalBlinks - 1, initialDelay, finalDelay));
        digitalWrite(ledPin, LOW);
        delay(map(i, 0, totalBlinks - 1, initialDelay, finalDelay));
    }
}

void sleep(int sec_to_sleep) {
    Serial.print(F("Will sleep now for approximately "));
    Serial.print(sec_to_sleep);
    Serial.println(F(" seconds."));
    delay(1000);

    for (int i = 0; i < sec_to_sleep / 8; i++) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }
    delay(1000);
    Serial.print(sec_to_sleep);
    Serial.println(F(" seconds has passed. Performing task..."));
}

int read_moisture() {
    // Power on the sensor
    digitalWrite(sensorPowerPin, HIGH);
  
    // Give the sensor a brief time to stabilize
    delay(50);
  
    // Read the analog value from the moisture sensor
    sensorValue = analogRead(moisturePin);
  
    // Map the sensor value to a percentage (0% dry, 100% wet)
    int moisturePercentage = map(sensorValue, dryValue, wetValue, 0, 100);

    // Clamp moisture percentage to the range 0-100
    moisturePercentage = constrain(moisturePercentage, 0, 100);
  
    // Print the raw value and the mapped percentage to the Serial Monitor
    Serial.print("Raw Value: ");
    Serial.print(sensorValue);
    Serial.print(" | Moisture: ");
    Serial.print(moisturePercentage);
    Serial.println("%");
  
    // Power off the sensor to save energy
    digitalWrite(sensorPowerPin, LOW);
 
    return moisturePercentage;
}

uint8_t uplink() {
    xlpp.reset();

    int moisturePercentage = read_moisture();
    xlpp.addTemperature(2, moisturePercentage);

    serialPrintf(("LoRaWAN send ... "));
    delay(3000);
    uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);
    if (e != 0) {
        serialPrintf(("Err %d\n"), e);
        delay(2000);
        return e;
    }
    Serial.println(F("OK\n"));

    return 0;
}

uint8_t downlink_with_logs(uint16_t timeout)
{
    uint8_t e;
  
    // 1.
    // Receive LoRaWAN downlink message in RX1
    Serial.print(F("Waiting for RX1, for a time of(in ms): "));
    Serial.println(timeout);
    //delay(1000); // former: Wait for 1 second (RX1)
    long startSend = millis();
    e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, timeout);
    long endSend = millis();
    if (e == ERR_LORA_TIMEOUT)
    {
      Serial.println(F("RX1 Timeout. Waiting for RX2..."));
      // Receive LoRaWAN downlink message in RX2
      delay(2000); // Wait for an additional 2 seconds (RX2)
      e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, timeout);
      if (e == ERR_LORA_TIMEOUT)
      {
        Serial.println(F("RX2 Timeout. No downlink received."));
        return ERR_LORA_TIMEOUT;
      }
      else
      {
        Serial.println(F("Downlink received in RX2."));
      }
    }
    else
    {
      Serial.println(F("Downlink received in RX1."));
    }
  
    // Further processing of the downlink message...
    serialPrintf("Time On Air: %d ms\n", endSend - startSend);
    serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
    serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);
    serialPrintf("LoRaWAN frame size: %d\n", xlpp.offset + xlpp.len);
    serialPrintf("LoRaWAN payload len: %d\n", xlpp.len);
    serialPrintf("Payload: ");
    if (xlpp.len == 0)
    {
      Serial.println(F("(no payload received)"));
      return 1;
    }
    printBase64(xlpp.getBuffer(), xlpp.len);
    Serial.println();
  
    // Read xlpp payload from downlink message
    int end = xlpp.len + xlpp.offset;
    while (xlpp.offset < end)
    {
      uint8_t chan = xlpp.getChannel();
      serialPrintf("Chan %2d: ", chan);
      uint8_t type = xlpp.getType();
      serialPrintf("Type %2d: ", type);
      switch (type)
      {
        case LPP_ANALOG_OUTPUT:
        case LPP_ANALOG_INPUT:
        default:
          Serial.println(F("Other unknown type."));
          return 1;
      }
    }
  
    return 0;
}

void setup() {
    // Set the sensor power pin as OUTPUT
    pinMode(sensorPowerPin, OUTPUT);
  
    pinMode(ledPin, OUTPUT);
  
    // Start the sensor powered off
    digitalWrite(sensorPowerPin, LOW);
    // Initialize the serial communication for debugging
    Serial.begin(38400);
    Serial.println("Soil Moisture Sensor Initialized.");
    
    blink_led();
  
    wazidev.setupLoRaWAN(devAddr, LoRaWANKeys);
    sx1272.setSF(SF_12); // std config for gateway accepts only spreading factor: 12-> change in
}

void loop() {
    // error indicator
    uint8_t e;
  
    // 1. LoRaWAN Uplink
    e = uplink();
    // if no error...
    if (!e) {
      // 2. LoRaWAN Downlink
      // waiting for interval time only!
      downlink_with_logs(2000);
      sleep(sleep_sec);
    }
    else {
      Serial.print(F("Error: "));
      Serial.println(e);
    }
}
