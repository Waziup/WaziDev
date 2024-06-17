#include <WaziDev.h>
#include <xlpp.h>

// LoRaWANKey is used as both NwkSKey (Network Session Key) and Appkey (AppKey) for secure LoRaWAN transmission.
// Copy'n'paste the key to your Wazigate: 23158D3BBC31E6AF670D195B5AED5525
unsigned char LoRaWANKeys[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
// Copy'n'paste the DevAddr (Device Address): 26011D87
unsigned char devAddr[4] = {0x26, 0x01, 0x1D, 0xB5};
// You can change the Key and DevAddr as you want.

WaziDev wazidev;

// globals
int interval = 10000; //10 sec
int interval_rep = 5;
int relayPin = 7;

volatile  int  NumPulses ;  //variable for the number of pulses received
int  FlowMeterSensor  =  2 ;     //Sensor connected to pin 10
float  factor_conversion = 7.5 ;  //to convert from frequency to flow rate
float  volume = 0 ;
long dt = 0 ; //time variation for each loop
long t0 = 0 ; //millis() from the previous loop

void setup()
{
  Serial.begin(38400);
  wazidev.setupLoRaWAN(devAddr, LoRaWANKeys);
  pinMode(relayPin, OUTPUT);
  pinMode ( FlowMeterSensor ,  INPUT ) ;
  attachInterrupt ( 0 , PulseCount , RISING ) ; //(Interrupt 0(Pin2),function,rising edge)
  Serial . println  ( "Send 'r' to reset the volume to 0 Liters" ) ;
  t0 = millis ( ) ;
}

XLPP xlpp(120);

//---Function executed in interrupt---------------
void  PulseCount  ( )
{
  NumPulses ++ ;   //increment the pulse variable
}

//---Function to obtain pulse frequency--------
int  GetFrequency ( )
{
  int  frequency ;
  NumPulses  =  0 ;    //We set the number of pulses to 0
  interrupts ( ) ;     // We enable the interruptions
  delay ( 1000 ) ;    //sample for 1 second
  noInterrupts ( ) ;  // We disable the interruptions
  frequency = NumPulses ;  //Hz(pulses per second)
  return  frequency ;
}

uint8_t uplink()
{
  // 1
  // Create xlpp payload.
  xlpp.reset();

  // 2
  // Add actuator
  xlpp.addActuators(1, LPP_SWITCH);

  // 2.
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
      case XLPP_BOOL_FALSE:
        {
          digitalWrite(relayPin, LOW);
          break;
        }
      case XLPP_BOOL_TRUE:
        {
          digitalWrite(relayPin, HIGH);
          break;
        }
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
    for(int i = 0; i <= interval_rep; i++){
      downlink(interval);
      Serial.println("repeat_interval: " + String(i));
    }
  }
  else {
    Serial.println("Error: " + e);
  }

  if  ( Serial . available ( ) )  {
    if ( Serial . read ( ) == 'r' ) volume = 0 ; //reset the volume if we receive 'r'
  }
  float  frequency = GetFrequency ( ) ;  //we obtain the frequency of the pulses in Hz
  float  flow_L_m = frequency / factor_conversion ; //calculate the flow in L/m
  dt = millis ( ) - t0 ;  //calculate the time variation
  t0 = millis ( ) ;
  volume = volume + ( flow_L_m / 60 ) * ( dt / 1000 ) ;  // volume(L)=flow(L/s)*time(s)

  //-----Send through the serial port---------------
  Serial.print("Flow: ");
  Serial.print(flow_L_m, 3);
  Serial.print(" L/min\tVolume: ");
  Serial.print(volume, 3);
  Serial.print(" L\tFrequency: ");
  Serial.println(frequency, 3);
}
