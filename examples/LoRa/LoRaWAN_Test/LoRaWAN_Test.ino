
#include <SPI.h> 
#include "SX1272.h"
#include "local_lorawan.h"

const uint8_t  maxDBM = 14;
const uint32_t channel = CH_18_868; // 868.1MHz for LoRaWAN test
const int      destAddr = 1;
const int      loraMode = 1;
const uint8_t  node_addr = 6;
const int      SF = 7;

//ENTER HERE your App Session Key from the TTN device info (same order, i.e. msb)
unsigned char AppSkey[16] = {0x4E, 0xF6, 0x45, 0x76, 0x16, 0x0F, 0xAD, 0x7F, 0x09, 0x3F, 0x7B, 0xE6, 0x21, 0xE4, 0x33, 0xB4};

//ENTER HERE your Network Session Key from the TTN device info (same order, i.e. msb)
unsigned char NwkSkey[16] = {0x83, 0x06, 0x52, 0x98, 0x29, 0xB1, 0xE8, 0x1A, 0xDD, 0x67, 0xBD, 0x2E, 0xCE, 0xA7, 0x3E, 0xBE};

//ENTER HERE your Device Address from the TTN device info (same order, i.e. msb). Example for 0x12345678
unsigned char DevAddr[4] = {0x26, 0x01, 0x15, 0x05};

#define PRINTLN                   Serial.println("")
#define PRINT_CSTSTR(fmt,param)   Serial.print(F(param))
#define PRINT_STR(fmt,param)      Serial.print(param)
#define PRINT_VALUE(fmt,param)    Serial.print(param)
#define PRINT_HEX(fmt,param)      Serial.print(param,HEX)
#define FLUSHOUTPUT               Serial.flush();


uint8_t message[80];

void setup()
{
  int e;

  delay(3000);
  Serial.begin(38400);  
  // Print a start message
  PRINT_CSTSTR("%s","LoRa temperature sensor, extended version\n");

  // Power ON the module
  sx1272.ON();
  
  local_lorawan_init(SF);

  // Set transmission mode and print the result
  e = sx1272.setMode(loraMode);
  PRINT_CSTSTR("%s","Setting Mode: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;
  
  e = sx1272.setChannel(channel);  
  PRINT_CSTSTR("%s","Setting frequency: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;
  
  // enable carrier sense
  sx1272._enableCarrierSense=true;  

  // Select amplifier line; PABOOST or RFO
  sx1272._needPABOOST=true;

  e = sx1272.setPowerDBM(maxDBM);

  //e = sx1272.setPowerDBM((uint8_t)20);
  
  PRINT_CSTSTR("%s","Setting Power: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;
  
  // Set the node address and print the result
  e = sx1272.setNodeAddress(node_addr);
  PRINT_CSTSTR("%s","Setting node addr: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;
  
  // Print a success message
  PRINT_CSTSTR("%s","SX1272 successfully configured\n");

  //printf_begin();
  delay(500);
}


void loop(void)
{
  long startSend;
  long endSend;
  int e;
  uint8_t r_size;

  r_size = sprintf((char*)message,"TEST");
  
  PRINT_CSTSTR("%s","Sending ");
  PRINT_STR("%s",(char*)(message));
  PRINTLN;
  
  PRINT_CSTSTR("%s","Real payload size is ");
  PRINT_VALUE("%d", r_size);
  PRINTLN;
  
  sx1272.setPacketType(PKT_TYPE_DATA);      
  
  int pl = local_aes_lorawan_create_pkt(message, r_size, 0, true);
  
  startSend = millis();
  
  e = sx1272.sendPacketTimeout(destAddr, message, pl);
  
  endSend = millis();

  PRINT_CSTSTR("%s","LoRa pkt size ");
  PRINT_VALUE("%d", pl);
  PRINTLN;
  
  PRINT_CSTSTR("%s","LoRa pkt seq ");
  PRINT_VALUE("%d", sx1272.packet_sent.packnum);
  PRINTLN;
  
  PRINT_CSTSTR("%s","LoRa Sent in ");
  PRINT_VALUE("%ld", endSend-startSend);
  PRINTLN;
      
  PRINT_CSTSTR("%s","LoRa Sent w/CAD in ");
  PRINT_VALUE("%ld", endSend-sx1272._startDoCad);
  PRINTLN;
   
  PRINT_CSTSTR("%s","Packet sent, state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  delay(7000); 

}
