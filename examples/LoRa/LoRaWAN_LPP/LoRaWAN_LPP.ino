
#include <SPI.h> 
#include "SX1272.h"
#include "local_lorawan.h"
#include <WaziDev.h>
#include <CayenneLPP.h>


const uint8_t  maxDBM = 14;
const int      SF = 12;

//ENTER HERE your App Session Key from the TTN device info (same order, i.e. msb). Example for 23158D3BBC31E6AF670D195B5AED5525
unsigned char AppSkey[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};

//ENTER HERE your Network Session Key from the TTN device info (same order, i.e. msb). Example for D83CB057CEBD2C43E21F4CDE01C19AE1
unsigned char NwkSkey[16] = {0xD8, 0x3C, 0xB0, 0x57, 0xCE, 0xBD, 0x2C, 0x43, 0xE2, 0x1F, 0x4C, 0xDE, 0x01, 0xC1, 0x9A, 0xE1};

//ENTER HERE your Device Address from the TTN device info (same order, i.e. msb). Example for 26011D87
unsigned char DevAddr[4] = {0x27, 0x01, 0x1D, 0x80};

uint8_t message[80];
CayenneLPP lpp(51);

void setup()
{
  int e;

  delay(3000);
  Serial.begin(38400);  
  // Print a start message
  serialPrintf("LoRaWAN test\n");

  // Power ON the module
  sx1272.ON();
  
  local_lorawan_init(SF);

  // enable carrier sense
  sx1272._enableCarrierSense=true;  

  // Select amplifier line; PABOOST or RFO
  sx1272._needPABOOST=true;

  e = sx1272.setPowerDBM(maxDBM);
  serialPrintf("Setting Power to %d DBM. state: %d\n", e, maxDBM);
  
  // Print a success message
  serialPrintf("SX1272 successfully configured\n");

  //printf_begin();
  delay(500);
}


void loop(void)
{
  //Create payload
  lpp.reset();
  lpp.addTemperature(1, 22.5);

  memcpy((char*)message, lpp.getBuffer(), lpp.getSize());
  serialPrintf("Sending payload: %s with size %d\n", message, lpp.getSize());
  
  sx1272.setPacketType(PKT_TYPE_DATA);      
  int len = local_aes_lorawan_create_pkt(message, lpp.getSize(), 0, true);
  
  long startSend = millis();
  int e = sx1272.sendPacketTimeout(0, message, len);
  long endSend = millis();

  serialPrintf("LoRa pkt size %d\n", len);
  serialPrintf("LoRa pkt seq %d\n", sx1272.packet_sent.packnum);
  serialPrintf("LoRa Sent in %ld\n", endSend-startSend);
  serialPrintf("LoRa Sent w/CAD in %ld\n", endSend-sx1272._startDoCad);

  delay(10000); 

}
