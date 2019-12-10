#include "wazidev.h"
#include <SPI.h> 
#include <EEPROM.h>
#include <LowPower.h>

//setup WaziDev
WaziDev::WaziDev(int nodeAddr) {

  this->nodeAddr = nodeAddr;
}
    
WaziDev::WaziDev(int nodeAddr, int destAddr, int loraMode, int channel, int maxDBm) {

  this->nodeAddr = nodeAddr;
  this->destAddr = destAddr;
  this->loraMode = loraMode;
  this->channel = channel;
  this->maxDBm = maxDBm;

};

void WaziDev::setup()
{
  Serial.begin(38400);  

  // Print a start message
  writeSerial("WaziDev starting...\n");

  // Power ON the module
  sx1272.ON();

  // get config from EEPROM
  EEPROM.get(0, config);

  // found a valid config?
  if (config.flag1==0x12 && config.flag2==0x34) {
    writeSerial("Getting back previous sx1272 config...\n");

    // set sequence number for SX1272 library
    sx1272._packetNumber=config.seq;
    writeSerial("Using packet sequence number of %d\n", sx1272._packetNumber);
  }
  else {
    // otherwise, write config and start over
    config.flag1=0x12;
    config.flag2=0x34;
    config.seq=sx1272._packetNumber;
  }
  
  // Set transmission mode and print the result
  int modeRes = sx1272.setMode(loraMode);
  writeSerial("Setting Mode: state %d\n", modeRes);
    
  int chanRes = sx1272.setChannel(channel);  
  writeSerial("Setting Channel: state %d\n", chanRes);

  // enable carrier sense
  sx1272._enableCarrierSense=true;
  // TODO: with low power, when setting the radio module in sleep mode
  // there seem to be some issue with RSSI reading
  sx1272._RSSIonSend=false;

  sx1272._needPABOOST=true; 

  int dbmRes = sx1272.setPowerDBM(maxDBm);
  writeSerial("Setting Power: state %d\n", dbmRes);
  
  // Set the node address and print the result
  int nodeAddrRes = sx1272.setNodeAddress(nodeAddr);
  writeSerial("Setting node addr: state %d\n", nodeAddrRes);
  
  // Print a success message
  writeSerial("WaziDev successfully configured\n");

}


void WaziDev::send(char sensor_id[], float val)
{
  writeSerial("Sending sensor %s with value %d\n", sensor_id, val);
      
  uint8_t message[50];
  uint8_t r_size = sprintf(message,"\\!%s/%s", sensor_id, String(val).c_str());

  writeSerial("Sending %s\n", message);
  writeSerial("Real payload size is %d\n", r_size);
      
  sx1272.CarrierSense();
  
  long startSend=millis();

  // just a simple data packet
  sx1272.setPacketType(PKT_TYPE_DATA);
 
  int sendRes = sx1272.sendPacketTimeout(destAddr, message, r_size);

  long endSend=millis();
    
  // save packet number for next packet in case of reboot
  config.seq=sx1272._packetNumber;     
  EEPROM.put(0, config);

  writeSerial("LoRa pkt size %d\n", r_size);
  writeSerial("LoRa pkt seq %d\n", sx1272.packet_sent.packnum);
  writeSerial("LoRa Sent in %ld\n", endSend-startSend);
  writeSerial("LoRa Sent w/CAD in %ld\n", endSend-sx1272._startDoCad);
  writeSerial("Packet sent, state %d\n", sendRes);
  writeSerial("Remaining ToA is %d\n", sx1272.getRemainingToA());
  writeSerial("Switch to power saving mode\n");

  int e = sx1272.setSleepMode();

  if (!e)
    writeSerial("Successfully switch LoRa module in sleep mode\n");
  else  
    writeSerial("Could not switch LoRa module in sleep mode\n");
    
  Serial.flush();
             
}

int WaziDev::getSensorValue(int pin) {

  //read the raw sensor value
  int value = analogRead(pin);

  writeSerial("Reading %s", value);

  return value;
}

// Power down the WaziDev for "duration" seconds
void WaziDev::powerDown(const int duration) {

  for (uint8_t i=0; i<duration; i++) {  
      // ATmega2560, ATmega328P, ATmega168, ATmega32U4
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
                              
      writeSerial(".");
      delay(1);                        
  }    
  writeSerial("\n");
  Serial.flush();

}

void WaziDev::writeSerial(const char* format, ...)
{
    char       msg[100];
    va_list    args;

    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args); // do check return value
    va_end(args);

    Serial.print(msg);
}

