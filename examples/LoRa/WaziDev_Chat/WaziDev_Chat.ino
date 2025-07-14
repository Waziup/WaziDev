#include <WaziDev.h>

WaziDev wazidev;

void setup() {
  Serial.begin(9600);
  while (!Serial); // wait for serial port to connect

  Serial.println("");
  Serial.println("HTW LoRa Paramater");

  // Activate SX1272 chipset
  wazidev.setupLoRa();

  // Frequency
  // 868.1 MHz, 868.3 MHz, 868.5 MHz, ..
  wazidev.setLoRaFreq(868100000);

  // Coding Rate
  // LORA_CR_5 (4/5), LORA_CR_6 (4/6), LORA_CR_7 (4/7) or LORA_CR_8 (4/8)
  wazidev.setLoRaCR(LORA_CR_5);

  // Spreading factor
  // LORA_SF_6 .. LORA_SF_12
  wazidev.setLoRaSF(LORA_SF_12);

  // Bandwidth
  // LORA_BW_7_8 (7.8 kHz), .., LORA_BW_125 (125 kHz), LORA_BW_250 (250 kHz)
  wazidev.setLoRaBW(LORA_BW_125);

  // Sync Word
  // 0x12 for LoRa (private networks), 0x34 for LoRaWAN (public networks)
  wazidev.setLoRaSyncWord(0x12);

  // Transmission Power [dbm]
  // 0 .. 14 normal power, 20 high power
  wazidev.setPowerDBM(14);
}

char buf[250];
uint8_t len = 0;

void loop() {

  while(Serial.available())
  {
    // Read the serial (USB) data
    buf[len] = Serial.read();   
    if (buf[len] == '\n')
    {
      buf[len] = NULL;

      // Send LoRa Frame
      Serial.println("");
      Serial.println(">> Uplink >>");
      Serial.print("Payload: \""); Serial.print(buf); Serial.print("\" ");
      Serial.print(len); Serial.println(" B");

      long startTime = millis();
      wazidev.sendLoRa(buf, len, false);
      long endTime = millis();

      Serial.print("Time On Air: "); Serial.print(endTime-startTime); Serial.println(" ms");

      len = 0;
    }
    else
    {
      len ++;
    }
  }

  // Receive LoRa Frame
  uint8_t err = wazidev.receiveLoRa(buf, &len, 0, false);
  switch(err)
  {
    case 0: // ok

      Serial.println("");
      Serial.println("<< Downlink <<");
      buf[len] = NULL; // terminated with NULL like a C string
      Serial.print("Payload: \""); Serial.print(buf); Serial.print("\" ");
      Serial.print(len); Serial.println(" B");
      // Signal-to-Noise Ratio (SNR), -20dB (noisy, bad) .. +10db (clear, good)
      Serial.print("SNR: "); Serial.print(wazidev.loRaSNR); Serial.println("dB");
      // Received Signal Strength Indication (RSSI), -120dBm (weak) .. -30dBm (strong)
      Serial.print("RSSI: "); Serial.print(wazidev.loRaRSSI); Serial.println("dBm");
      break;

    case ERR_LORA_TIMEOUT:
      // Nothing received
      break;

    default: // err
      Serial.print("Error: "); Serial.println(err);
      break; 
  }

  delay(1000);
  Serial.print(".");
}
