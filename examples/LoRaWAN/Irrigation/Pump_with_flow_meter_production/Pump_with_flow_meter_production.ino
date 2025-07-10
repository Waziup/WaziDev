#include <WaziDev.h>
#include <xlpp.h>
#include <LowPower.h>
#include <Vcc.h> 
//#include <DHT.h>

WaziDev wazidev;

unsigned char LoRaWANKeys[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0xD9};

const int interval = 3000;
const int relayPin = 5;
const int relayPowerPin = 6;

volatile int NumPulses = 0;
const int FlowMeterSensorDataPin = 4;
const int FlowMeterSensorPowerPin = 7;
// const float factor_conversion = 5.625; // for DN20
const float factor_conversion = 12.0;     // for DN50
float volume = 0;
long dt = 0;
long t0 = 0;
volatile float amountWater = 0;

int previousState = LOW;
unsigned long startTime = 0;
const unsigned long samplingTime = 1000;

const long MAX_IRRIGATION_TIME = 180000;
const long REST_PERIOD = 300000;

const int sleep_sec = 1800;//1800 // Time in sec in sleep mode DEBUG

// unsigned long lastTransmissionTime = 0;
// const unsigned long interval_vcc = 3000;//3600000; // 60 minutes in milliseconds //DEBUG

const int ledPin = 8;
const int totalBlinks = 20;
const int initialDelay = 100;
const int finalDelay = 10;

const int batt_pin = A0;
// const float VccCorrection = 12.35 / 12.68; // 12V
const float VccCorrection = 3.85 / 7.5; // 4,2V LiIon
Vcc vcc(VccCorrection);

// const int dhtDataPin = 9;
// const int powerPinDht = A1;
// DHT dht(dhtDataPin, DHT11);

// struct DHTData {
//     float temperature;
//     float humidity;
// };

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

    pinMode(relayPin, OUTPUT);
    pinMode(relayPowerPin, OUTPUT);
    pinMode(FlowMeterSensorDataPin, INPUT);
    pinMode(FlowMeterSensorPowerPin, OUTPUT);
    pinMode(ledPin, OUTPUT);
    //pinMode(powerPinDht, OUTPUT);
    //digitalWrite(powerPinDht, HIGH);

    //dht.begin();
    //delay(2000);

    blink_led();

    wazidev.setupLoRaWAN(devAddr, LoRaWANKeys);
    sx1272.setSF(SF_12); // std config for gateway accepts only spreading factor: 12-> change in 

    Serial.println(F("Send 'r' to reset the volume to 0 Liters"));
    t0 = millis();

    // Preserve energy
    digitalWrite(relayPowerPin, LOW);
    //digitalWrite(powerPinDht, LOW);
}

XLPP xlpp(40);

void PulseCount() {
    NumPulses++;
}

int GetFrequencyPolling() {
    int currentState;
    NumPulses = 0;
    startTime = millis();

    while (millis() - startTime < samplingTime) {
        currentState = digitalRead(FlowMeterSensorDataPin);
        if (currentState != previousState) {
            if (currentState == HIGH) {
                NumPulses++;
            }
            previousState = currentState;
        }
    }
    return NumPulses;
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

int irrigate(float amount) {
    volume = 0;
    previousState = LOW;
    
    digitalWrite(FlowMeterSensorPowerPin, HIGH);

    digitalWrite(relayPowerPin, HIGH);
    delay(1000);
    digitalWrite(relayPin, LOW);

    t0 = millis();
    unsigned long startIrrigationTime = t0;

    Serial.print(F("Irrigation started. \nAmount already given:\t"));
    Serial.print(volume, 3);
    Serial.print(F("\tAmount to Irrigate: \t"));
    Serial.println(amount, 3);

    while (volume < amount) {
        if (millis() - startIrrigationTime >= MAX_IRRIGATION_TIME) {
            Serial.println(F("Max irrigation time reached. Pausing irrigation."));
            digitalWrite(relayPin, HIGH);
            delay(REST_PERIOD);
            return irrigate(amount - volume);
        }

        int frequency = GetFrequencyPolling();
        float flow_L_m = frequency / factor_conversion;
        dt = millis() - t0;
        t0 = millis();
        volume += (flow_L_m / 60) * (dt / 1000);

        Serial.print(F("Flow: "));
        Serial.print(flow_L_m, 3);
        Serial.print(F(" L/min\tVolume: "));
        Serial.print(volume, 3);
        Serial.print(F(" L\tFrequency: "));
        Serial.println(frequency, 3);
    }

    Serial.println(F("Irrigation completed."));
    digitalWrite(relayPin, HIGH);
    delay(100);
    digitalWrite(relayPowerPin, LOW);
    digitalWrite(FlowMeterSensorPowerPin, LOW);

    return 0;
}

float readVolts() {
    int j = 0;
    float vcc_reg = 0;
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
        last_vcc += ((analogRead(batt_pin) * (vcc_reg / 1023.0)) * 3.83);
        delay(5);
    }
    last_vcc /= i;
    Serial.print(F("Voltage of Battery: "));
    Serial.print(last_vcc, 2);
    Serial.println(F("V"));

    return last_vcc;
}

//DHTData readDHT() {
//    DHTData data;
//    digitalWrite(powerPinDht, HIGH);
//    delay(3000);
//
//    data.temperature = dht.readTemperature();
//    data.humidity = dht.readHumidity();
//
//    if (isnan(data.humidity) || isnan(data.temperature)) {
//        Serial.println(F("Failed to read from DHT11 sensor!"));
//    } else {
//        Serial.print(F("Temperature: "));
//        Serial.print(data.temperature);
//        Serial.print(F(" °C\tHumidity: "));
//        Serial.print(data.humidity);
//        Serial.println(F(" % "));
//    }
//
//    digitalWrite(powerPinDht, LOW);
//    
//    return data;
//}

uint8_t uplink() {
    xlpp.reset();

    xlpp.addActuators(1, LPP_ANALOG_OUTPUT);

    //if (millis() - lastTransmissionTime >= interval_vcc) { // Can be adjusted to not transmit in every uplink
    float last_vcc = readVolts();
    xlpp.addVoltage(2, last_vcc);

//    DHTData data = readDHT();
//    xlpp.addTemperature(3, data.temperature);
//    xlpp.addRelativeHumidity(4, data.humidity);

    //lastTransmissionTime = millis();
    //}

    serialPrintf(("LoRaWAN send ... "));
    delay(3000);
    uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);
    if (e != 0) {
        serialPrintf(("Err %d\n"), e);
        delay(interval);
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
        {
          amountWater = xlpp.getAnalogOutput();
          Serial.print(F("Anticipated amount to water: "));
          Serial.println(amountWater);
          e = irrigate(amountWater);
          if (e != 0)
          {
            Serial.print(F("There was an Error in the irrigation function: "));
            Serial.println(e);
          }
          else
          {
            Serial.println(F("Irrigation successful."));
          }
          break;
        }
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
    downlink_with_logs(interval);
    sleep(sleep_sec);
  }
  else {
    Serial.print(F("Error: "));
    Serial.println(e);
  }

  if  ( Serial . available ( ) )  {
    if ( Serial . read ( ) == 'r' ) volume = 0 ; //reset the volume if we receive 'r'
  }
}
