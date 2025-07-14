#include "Adafruit_Si7021.h"
#include <xlpp.h>
#include <WaziDev.h>

Adafruit_Si7021 sensor = Adafruit_Si7021();

// DevAddr: 26011D13
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0x13};

// AppSKey (App Session Key): 23158D3BBC31E6AF670D195B5AED5525
unsigned char appSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};

// NwSKey (Network Session Key): 23158D3BBC31E6AF670D195B5AED5525
unsigned char nwkSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};

WaziDev wazidev;

#define LED 9
#define BUZZER 7

XLPP xlpp(120);

void setup() {
  Serial.begin(115200);

  Serial.println("LoRaWAN Class A Test");
  
  sensor.begin();

  wazidev.setupLoRaWAN(devAddr, appSkey, nwkSkey);

  wazidev.setLoRaSF(LORA_SF_12);

  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
}

void loop() {
  Serial.println(">> Uplink >>");

  Serial.print("Humidity:    ");
  float humidity = sensor.readHumidity();
  Serial.print(humidity, 2);
  Serial.print("\tTemperature: ");
  float temperature = sensor.readTemperature();
  Serial.println(temperature, 2);

  xlpp.reset();
  xlpp.addTemperature(1, temperature);
  xlpp.addRelativeHumidity(2, humidity); 

  wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);

  //

  Serial.println("<< Downlink <<");

  int err = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, 3000);
  switch(err) {
  case 0: // OK
    Serial.print("Payload: 0x");

    char* payload = xlpp.buf + xlpp.offset;

    for(int i = 0; i<xlpp.len; i++)
      Serial.print(payload[i], HEX);
    Serial.print(" ");
    Serial.print(xlpp.len); Serial.println(" B");
    Serial.print("Payload: \""); Serial.print(payload); Serial.println("\"");
    // Signal-to-Noise Ratio (SNR), -20dB (noisy, bad) .. +10db (clear, good)
    Serial.print("SNR: "); Serial.print(wazidev.loRaSNR); Serial.println("dB");
    // Received Signal Strength Indication (RSSI), -120dBm (weak) .. -30dBm (strong)
    Serial.print("RSSI: "); Serial.print(wazidev.loRaRSSI); Serial.println("dBm");

    if (strcmp(payload, "") == 0) {
      Serial.println("Empty payload.");

    } else if (strcmp(payload, "BUZZ") == 0) {
      Serial.println("Buzzing ..");
      for(int i=0; i<3; i++) {
        tone(BUZZER, 522);
        delay(500);
        tone(BUZZER, 660);
        delay(500);
        tone(BUZZER, 784);
        delay(500);
      }
      noTone(BUZZER);
    } else if (strcmp(payload, "BLINK") == 0) {
      Serial.println("Blinking ..");
      for(int i=0; i<20; i++) {
        digitalWrite(LED, HIGH);
        delay(200);
        digitalWrite(LED, LOW);
        delay(200);
      }
    } else {
      Serial.println("Unknown payload ?!");
    }

    break;
  case ERR_LORA_TIMEOUT:
    Serial.println("Nothing received.");
    break;
  default: // err
    Serial.print("Error: "); Serial.println(err);
    break;
  }

  //

  delay(60000);
}
