#include <WaziDev.h>
#include <xlpp.h>
#include <LowPower.h>
#include <Vcc.h> 
//#include <avr/sleep.h>
//#include <avr/power.h>
//#include <avr/wdt.h>

WaziDev wazidev;

// Globals:
// LoRaWANKey is used as both NwkSKey (Network Session Key) and Appkey (AppKey) for secure LoRaWAN transmission.
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char LoRaWANKeys[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
// Copy'n'paste the DevAddr (Device Address): 26011D87
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0xB5};
// You can change the Key and DevAddr as you want.

// Relay
int interval = 3000; //3sec
int relayPin = 7;

// Flow sensor
volatile  int  NumPulses = 0;  //variable for the number of pulses received
int  FlowMeterSensor  =  6 ;     //Sensor connected to pin 6
float  factor_conversion = 5.625;//7.5 ;  //to convert from frequency to flow rate, was 25% higher=factor*.75!!!
float  volume = 0 ;
long dt = 0 ; //time variation for each loop
long t0 = 0 ; //millis() from the previous loop
volatile float amountWater = 0; // anticipated amount of water to irrigate in liter, transmitted as analog input

// Flow sensor read flanks (edges)
int previousState = LOW;  // Initial state of the sensor pin
unsigned long startTime = 0;
unsigned long samplingTime = 1000;  // Sampling period in milliseconds (1 second)

// Pump related
int MAX_IRRIGATION_TIME = 180000; // 3 minutes in milliseconds
int REST_PERIOD = 300000; // 5 minutes in milliseconds

// sleep related
//volatile bool wdt_interrupt = false; // Flag to check if WDT interrupt occurred
//volatile int wakeup_count = 0; // Counter to track wake-ups
int sleep_sec = 1800;//60;

// Voltage transmission
unsigned long lastTransmissionTime = 0; // Stores the last transmission time
const unsigned long interval_vcc = 3600000; // 60 minutes in milliseconds

// LED
const int ledPin = 8; // Pin on microcontroller
const int totalBlinks = 20; // Total number of blinks
const int initialDelay = 100; // Initial delay in milliseconds
const int finalDelay = 10; // Final delay in milliseconds

// Read voltage
const int batt_pin = 6;
//finally set VccCorrection to measured Vcc by multimeter divided by reported Vcc
const float VccCorrection = 12.35/12.68; //for the one in the case
// calibration mode: uncomment to perform calibration
//const float VccCorrection = 1;
Vcc vcc(VccCorrection); 

//ISR(WDT_vect) {
//  wdt_interrupt = true;
//  wakeup_count++;
//}

//void setupWatchdog() {
//  cli(); // Disable global interrupts
//  // Clear WDT reset flag
//  MCUSR &= ~(1 << WDRF);
//  // Set up WDT interrupt mode and timeout for 8 seconds
//  WDTCSR = (1 << WDCE) | (1 << WDE);
//  WDTCSR = (1 << WDP3) | (1 << WDP0) | (1 << WDIE);
//  sei(); // Enable global interrupts
//}
//
//void enterSleep() {
//  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode to power down
//  sleep_enable(); // Enable sleep mode
//  
//  // Disable all peripherals except WDT
//  power_all_disable();
//
//  sleep_bod_disable(); // Disable brown-out detector
//  sleep_cpu(); // Enter sleep mode
//
//  sleep_disable(); // Disable sleep mode after waking up
//  power_all_enable(); // Re-enable all peripherals
//}

void blink_led() {
  for (int i = 0; i < totalBlinks; i++) {
    digitalWrite(ledPin, HIGH); // Turn LED on
    delay(map(i, 0, totalBlinks - 1, initialDelay, finalDelay)); // Wait based on current iteration
    digitalWrite(ledPin, LOW); // Turn LED off
    delay(map(i, 0, totalBlinks - 1, initialDelay, finalDelay)); // Wait based on current iteration
  }  
}

void setup()
{
  Serial.begin(38400);
  
  // PINs
  pinMode(relayPin, OUTPUT);
  pinMode(FlowMeterSensor , INPUT);
  pinMode(ledPin, OUTPUT);

  // Blink to say hello
  blink_led();

  // Lora
  wazidev.setupLoRaWAN(devAddr, LoRaWANKeys);
  sx1272.setSF(SF_12);  // SF = 12
  
  //attachInterrupt ( 0 , PulseCount , RISING ) ; //(Interrupt 0(Pin2),function,rising edge)
  Serial . println  ( "Send 'r' to reset the volume to 0 Liters" ) ;
  t0 = millis ( ) ;
  //setupWatchdog(); // Set up the watchdog timer
}

XLPP xlpp(120);

//---Function executed in interrupt---------------
void  PulseCount  ( )
{
  NumPulses ++ ;   //increment the pulse variable
}

//---Function to obtain pulse frequency of HAL sensor, sensing interrupts is only possible on D2, D3 on Atmega 328p--------
int  GetFrequencyInterrupts ( )
{
  int  frequency ;
  NumPulses  =  0 ;    //We set the number of pulses to 0
  interrupts ( ) ;     // We enable the interruptions
  delay ( 1000 ) ;    //sample for 1 second
  noInterrupts ( ) ;  // We disable the interruptions
  frequency = NumPulses ;  //Hz(pulses per second)
  //Serial.print(frequency) ;

  return  frequency ;
}

//---Function to obtain pulse frequency of HAL sensor, alternative approach read the changing flanks (edges), possible on every digital pin --------
int GetFrequencyPolling () {
  int currentState;
  NumPulses = 0;  // Reset the pulse count
  startTime = millis();  // Record the start time

  while (millis() - startTime < samplingTime) {
    currentState = digitalRead(FlowMeterSensor);  // Read the current state of the sensor pin

    if (currentState != previousState) {
      // A change in state (rising or falling edge) is detected, only count HIGH
      if (currentState == HIGH) {
        NumPulses++;
      }
      previousState = currentState;  // Update the previous state
    }
  }
  //Serial.print(NumPulses) ;

  return NumPulses;  // Hz (pulses per second)
}

//void sleep_old(int sec_to_sleep) {
//  delay(1000);
//  Serial.println("Will sleep now for approximately " + String(sec_to_sleep) + " seconds.");
//  int cycles_sleep = round(sec_to_sleep / 8); //calc the cycles
//  
//  // Reset wake-up count
//  wakeup_count = 0;
//
//  while (wakeup_count < cycles_sleep) { // Loop for approximately 1 minute (8*8s=64s)
//    enterSleep(); // Enter sleep mode
//    if (wdt_interrupt) { // Check if WDT interrupt occurred
//      wdt_interrupt = false; // Reset WDT interrupt flag
//      Serial.println("Woke up from WDT!");
//    }
//  }
//
//  Serial.println(sleep_sec, " seconds has passed. Performing task...");
//}

void sleep(int sec_to_sleep) {
  Serial.print("Will sleep now for approximately ");
  Serial.print(sec_to_sleep);
  Serial.println(" seconds.");

  for (int i = 0; i < sec_to_sleep / 8; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  Serial.print(sec_to_sleep);
  Serial.println(" seconds has passed. Performing task...");
}


// ---Function to perform the actual irrigation---
int irrigate(float amount) {
  volume = 0; // Reset volume
  t0 = millis(); // Reset start time
  unsigned long startIrrigationTime = millis();
  previousState = LOW; // Reset previous state for polling

  digitalWrite(relayPin, HIGH); // Turn on the pump
  Serial.println("Irrigation started. \nVolume ");
  Serial.print(volume, 3);
  Serial.print("Amount: ");
  Serial.print(amount, 3);

  while (volume < amount) {
    if (millis() - startIrrigationTime >= MAX_IRRIGATION_TIME) {
      Serial.println("Max irrigation time reached. Pausing irrigation.");
      digitalWrite(relayPin, LOW); // Turn off the pump
      delay(REST_PERIOD); // Rest for a specified period
      return irrigate(amount - volume); // Call routine with rest amount again
    }

    int frequency = GetFrequencyPolling();
    float flow_L_m = frequency / factor_conversion; // Calculate the flow in L/min
    dt = millis() - t0; // Calculate the time variation
    t0 = millis();
    volume += (flow_L_m / 60) * (dt / 1000); // volume(L) = flow(L/s) * time(s)

    // Send through the serial port
    Serial.print("Flow: ");
    Serial.print(flow_L_m, 3);
    Serial.print(" L/min\tVolume: ");
    Serial.print(volume, 3);
    Serial.print(" L\tFrequency: ");
    Serial.println(frequency, 3);
  }
  
  Serial.println("Irrigation completed.");
  digitalWrite(relayPin, LOW); // Turn off the pump
  
  return 0;
}

uint8_t uplink()
{
  // 1
  // Create xlpp payload.
  xlpp.reset();

  // 2
  // Add actuator
  xlpp.addActuators(1, LPP_ANALOG_OUTPUT);

  // 3
  // send voltage in larger time intervals (capsulate function, but lazy^^)
  if (millis() - lastTransmissionTime >= interval_vcc) {
    // Read voltage of reg and bat
    // Regulator:
    int j = 0;
    float vcc_reg = 0;
    for (j = 0; j < 100; j++) {
      vcc_reg += vcc.Read_Volts();
      delay(5);
    }
    // Calc average of regulator readings
    vcc_reg = vcc_reg / j;
    // Print VCC
    Serial.print("VCC of regulator: ");
    Serial.print(vcc_reg, 3);
    Serial.println("V ");
    
    // Read voltage of battery:  
    int i = 0;
    float last_vcc = 0;
    for (i = 0; i < 100; i++) {
      last_vcc += ((analogRead(batt_pin) * (vcc_reg / 1023.0)) * 3.83); 
      delay(5);
    }
    // Calc average of battery readings
    last_vcc = last_vcc / i;
    // Print voltage of bat
    Serial.print("Voltage of Battery: ");
    Serial.print(last_vcc, 2);
    Serial.println("V");

    // Add payload
    xlpp.addVoltage(2, last_vcc);   
    // Update the last transmission time
    lastTransmissionTime = millis();
  }
  
  // 4
  // Send payload with LoRaWAN.
  serialPrintf("LoRaWAN send ... ");
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);
  if (e != 0)
  {
    serialPrintf("Err %d\n", e);
    delay(interval);
    return e;
  }
  serialPrintf("OK\n");

  return 0;
}

uint8_t downlink(uint16_t timeout)
{
  uint8_t e;

  // 1.
  // Receive LoRaWAN downlink message.
  serialPrintf("LoRa receive ... ");
  uint8_t offs = 0;
  long startSend = millis();
  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, timeout);
  long endSend = millis();
  if (e)
  {
    if (e == ERR_LORA_TIMEOUT)
      serialPrintf("nothing received\n");
    else
      serialPrintf("Err %d\n", e);
    return e;
  }
  serialPrintf("OK\n");

  serialPrintf("Time On Air: %d ms\n", endSend - startSend);
  serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
  serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);
  serialPrintf("LoRaWAN frame size: %d\n", xlpp.offset + xlpp.len);
  serialPrintf("LoRaWAN payload len: %d\n", xlpp.len);
  serialPrintf("Payload: ");
  if (xlpp.len == 0)
  {
    serialPrintf("(no payload received)\n");
    return 1;
  }
  printBase64(xlpp.getBuffer(), xlpp.len);
  serialPrintf("\n");

  // 2.
  // Read xlpp payload from downlink message.
  // You must use the following pattern to properly parse xlpp payload!
  int end = xlpp.len + xlpp.offset;
  while (xlpp.offset < end)
  {
    // [1] Always read the channel first ...
    uint8_t chan = xlpp.getChannel();
    serialPrintf("Chan %2d: ", chan);

    // [2] ... then the type ...
    uint8_t type = xlpp.getType();
    serialPrintf("Type %2d: ", type);

    // [3] ... then the value!
    switch (type) {
//      case XLPP_BOOL_FALSE:
//        {
//          digitalWrite(relayPin, LOW);
//          break;
//        }
//      case XLPP_BOOL_TRUE:
//        {
//          digitalWrite(relayPin, HIGH);
//          break;
//        }
      case LPP_ANALOG_OUTPUT:
      case LPP_ANALOG_INPUT:
        {
          amountWater = xlpp.getAnalogOutput();
          Serial.print("Anticipated amount to water: ");
          Serial.println(amountWater);
          e = irrigate(amountWater);
          if (e != 0) {
            Serial.print("There was an Error in the irrigation function: ");
            Serial.println(e);
          }
          else {
            Serial.print("Irrigation successful.");
          }
          break;
        }
      //      case LPP_ANLOG_INPUT:
      //        {
      //
      //        }
      default:
        // For all the other types, see https://github.com/Waziup/arduino-xlpp/blob/main/test/simple/main.cpp#L147
        serialPrintf("Other unknown type.\n");
        return 1;
    }
  }
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
    downlink(interval);
    sleep(sleep_sec);
  }
  else {
    Serial.println("Error: " + e);
  }

  if  ( Serial . available ( ) )  {
    if ( Serial . read ( ) == 'r' ) volume = 0 ; //reset the volume if we receive 'r'
  }
}
