#include <WaziDev.h>
#include <xlpp.h>
#include <LowPower.h>
#include <Vcc.h> 

WaziDev wazidev;

unsigned char LoRaWANKeys[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0xB8};

const int sleep_sec = 1800;//1800 // Time in sec in sleep mode DEBUG

const int ledPin = 8;
const int totalBlinks = 20;
const int initialDelay = 100;
const int finalDelay = 10;

const int batt_pin = A7;
//finally set VccCorrection to measured Vcc by multimeter divided by reported Vcc, to calibrate use 1!!!
const float VccCorrection = 3.2/0.86;//3.57 / 3.17;
Vcc vcc(VccCorrection);

XLPP xlpp(40);


void blink_led() {
    for (int i = 0; i < totalBlinks; i++) {
        digitalWrite(ledPin, HIGH);
        delay(map(i, 0, totalBlinks - 1, initialDelay, finalDelay));
        digitalWrite(ledPin, LOW);
        delay(map(i, 0, totalBlinks - 1, initialDelay, finalDelay));
    }
}

void setup() {
    Serial.begin(38400);
    
    pinMode(ledPin, OUTPUT);

    delay(2000);

    blink_led();

    wazidev.setupLoRaWAN(devAddr, LoRaWANKeys);
    sx1272.setSF(SF_12); // std config for gateway accepts only spreading factor: 12-> change in 
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

float readVolts() {
    int j = 0;
    float vcc_reg = 0;
    float scaleFactor = 1.272;
    for (j = 0; j < 100; j++) {
        vcc_reg += vcc.Read_Volts();
        delay(5);
    }
    vcc_reg /= j;
    Serial.print(F("VCC of regulator: "));
    Serial.print(vcc_reg, 3);
    Serial.println(F("V "));

    int i = 0;
    float last_vcc = 0;
    for (i = 0; i < 100; i++) {
        last_vcc += ((analogRead(batt_pin) * (vcc_reg / 1023.0)) * scaleFactor);
        delay(5);
    }
    last_vcc /= i;
    Serial.print(F("Voltage of Battery: "));
    Serial.print(last_vcc, 2);
    Serial.println(F("V"));

    return last_vcc;
}


uint8_t uplink() {
    xlpp.reset();

    float last_vcc = readVolts();
    xlpp.addVoltage(2, last_vcc);

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

void loop(void)
{
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
