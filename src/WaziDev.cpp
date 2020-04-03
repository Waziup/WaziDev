#include "WaziDev.h"
#include <SPI.h> 
#include <EEPROM.h>
#include <LowPower.h>

//setup WaziDev
WaziDev::WaziDev(String deviceId, int nodeAddr) {

  this->deviceId = deviceId;
  this->nodeAddr = nodeAddr;
}
    
WaziDev::WaziDev(String deviceId, int nodeAddr, int destAddr, int loraMode, int channel, int maxDBm) {

  this->deviceId = deviceId;
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
  Serial.flush();

}

void WaziDev::sendSensorValue(String sensorId, float val)
{
  SensorVal fields[1] = {{sensorId, val}};
  sendSensorValues(fields, 1);
}

String WaziDev::getPayload(const SensorVal vals[], int nbValues) {
  String message = "\\!";
  if(deviceId != NULL) {
    message += "UID/" + deviceId + "/";
  }
  for(int i = 0; i<nbValues; i++) {
    message += vals[i].sensorId + "/" + String(vals[i].value);
    if(i != nbValues -1) {
      message += "/";
    }
  }
  
  return message;
}

void WaziDev::sendSensorValues(const SensorVal vals[], int nb_values) {

  String message = getPayload(vals, nb_values);
 
  int r_size = message.length() + 1;
  writeSerial("Sending " + message + "\n");
  writeSerial("Real payload size is %d\n", r_size);

  send(message);
}

int WaziDev::send(String message) {

  int r_size = message.length() + 1;
      
  sx1272.CarrierSense();
 
  // just a simple data packet
  sx1272.setPacketType(PKT_TYPE_DATA);
  
  long startSend = millis();

  int sendRes = sx1272.sendPacketTimeoutACK(destAddr, message.c_str());

  long endSend = millis();
    
  // save packet number for next packet in case of reboot
  config.seq=sx1272._packetNumber;     
  EEPROM.put(0, config);

  writeSerial("LoRa pkt size %d\nLoRa pkt seq %d\nLoRa Sent in %ld\nLoRa Sent w/CAD in %ld\nPacket sent, state %d\nRemaining ToA is %d\n",
              r_size,
              sx1272.packet_sent.packnum,
              endSend-startSend,
              endSend-sx1272._startDoCad,
              sendRes, sx1272.getRemainingToA());

  writeSerial("Switch to power saving mode\n");
  int resSleep = sx1272.setSleepMode();

  if (!resSleep)
    writeSerial("Successfully switch LoRa module in sleep mode\n");
  else  
    writeSerial("Could not switch LoRa module in sleep mode\n");
    
  Serial.flush();

  return sendRes;

}

int WaziDev::receiveActuatorValue(String actuatorId, int wait, String &act) {

  char uidVal[55] = "";
  char actId[55] = "";
  char actVal[55] = "";
 
  //Get the data from LoRa
  String res;
  this->receive(res, wait);

  //Parse the data
  sscanf(res.c_str(), "\\!UID/%[^/]/%[^/]/%s", uidVal, actId, actVal);
  writeSerial("\nReceived: uid = %s, actuator Id = %s, value = %s\n", uidVal, actId, actVal);
  Serial.flush();
  
  if(String(uidVal).compareTo(deviceId) == 0 &&
     String(actId).compareTo(actuatorId) == 0) {
    
    //Return the actuator value.
    act = String(actVal);
    return 0;

  } else {

    return -1;
  }
}

int WaziDev::receive(String &out, int wait) {

  writeSerial("Listening LoRa...\n");
  int resRec = sx1272.receiveAll(wait);

  if (resRec != 0 && resRec != 3) {
     writeSerial("Receive error %d\n", resRec);

     if (resRec == 2) {
         // Power OFF the module
         sx1272.OFF();
         writeSerial("Resetting radio module\n");
         int resON = sx1272.ON();
         writeSerial("Setting power ON: state %d\n", resON);
     }
     Serial.flush();
     return -1;

  } else {
      
    sx1272.getSNR();
    sx1272.getRSSIpacket();
    char* data = (char*)malloc(sx1272._payloadlength * sizeof(char) + 1);  //TODO is it correct to malloc each time? Who will free?
    memcpy(data, sx1272.packet_received.data, sx1272._payloadlength);
    data[sx1272._payloadlength] = '\0';

    writeSerial("Received from LoRa:\n  data = %s\n", data);
    writeSerial("  dst = %d, type = 0x%02X, src = %d, seq = %d\n",
                sx1272.packet_received.dst,
                sx1272.packet_received.type,
                sx1272.packet_received.src,
                sx1272.packet_received.packnum);
    writeSerial("  len = %d, SNR = %d, RSSIpkt = %d, BW = %d, CR = 4/%d, SF = %d\n",
                sx1272._payloadlength,
                sx1272._SNR,
                sx1272._RSSIpacket,
                (sx1272._bandwidth==BW_125) ? 125 : ((sx1272._bandwidth==BW_250) ? 250 : 500),
                sx1272._codingRate+4,
                sx1272._spreadingFactor);

    Serial.flush();


    out = String(data);
    free(data);
    return 0;
  }
}


float WaziDev::getSensorValue(int pin) {

  //read the raw sensor value
  float value = analogRead(pin);

  writeSerial("Reading %f\n", value);

  return value;
}

void WaziDev::putActuatorValue(int pin, float val) {

  writeSerial("Writing on pin %d with value %d\n", pin, val);
  analogWrite(pin, val);

}

// Power down the WaziDev for "duration" seconds
void WaziDev::powerDown(const int duration) {

  for (uint8_t i=0; i<duration; i++) {  
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
      writeSerial(".");
      delay(1);                        
  }    
  writeSerial("\n");
  Serial.flush();

}

void WaziDev::writeSerial(String message)
{
   writeSerial(message.c_str());
}

void WaziDev::writeSerial(const char *format, ...)
{
   char       msg[100];
   va_list    args;

   va_start(args, format);
   vsnprintf(msg, sizeof(msg), format, args);
   va_end(args);

   Serial.print(msg);
}
