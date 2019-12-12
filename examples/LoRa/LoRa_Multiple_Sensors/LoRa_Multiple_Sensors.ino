#include "SX1272.h"
#include <Wire.h>
#include <SPI.h>
#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10

//Variables
int chk;

// IMPORTANT
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// please uncomment only 1 choice
//
#define ETSI_EUROPE_REGULATION
//#define FCC_US_REGULATION
//#define SENEGAL_REGULATION
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef ETSI_EUROPE_REGULATION
#define MAX_DBM 14
// previous way for setting output power
// char powerLevel='M';
#elif defined SENEGAL_REGULATION
#define MAX_DBM 10
// previous way for setting output power
// 'H' is actually 6dBm, so better to use the new way to set output power
// char powerLevel='H';
#elif defined FCC_US_REGULATION
#define MAX_DBM 14
#endif

// IMPORTANT
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// uncomment if your radio is an HopeRF RFM92W, HopeRF RFM95W, Modtronix inAir9B, NiceRF1276
// or you known from the circuit diagram that output use the PABOOST line instead of the RFO line
#define PABOOST
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
#define DEFAULT_DEST_ADDR 1
#define LORAMODE  1

///////////////////////////////////////////////////////////////////
// CHANGE HERE THE NODE ADDRESS 
#define node_addr 20
//////////////////////////////////////////////////////////////////

unsigned long nextTransmissionTime=0L;

uint8_t message[100];

int loraMode=LORAMODE;

//ADD NEW BOOLEAN HERE FOR MORE SENSORS
boolean a = true;
boolean b = false;

void setup(){
  int e;
  delay(3000);
  // Open serial communications and wait for port to open:
  Serial.begin(38400);
  // Print a start message
  Serial.println(F("Simple LoRa Universal Sensor Code"));

#if defined ARDUINO_Heltec_WIFI_LoRa_32 || defined ARDUINO_WIFI_LoRa_32 || defined HELTEC_LORA
  // for the Heltec ESP32 WiFi LoRa module
  sx1272.setCSPin(18);
#endif

  // Power ON the module
  sx1272.ON();
  
  // Set transmission mode and print the result
  e = sx1272.setMode(loraMode);
  Serial.print(F("Setting Mode: state "));
  Serial.println(e, DEC);

  // enable carrier sense
  sx1272._enableCarrierSense=true;
    
  // Select frequency channel
  e = sx1272.setChannel(CH_10_868);
  Serial.print(F("Setting Channel: state "));
  Serial.println(e, DEC);
  
  // Select amplifier line; PABOOST or RFO
  #ifdef PABOOST
    sx1272._needPABOOST=true;
    // previous way for setting output power
    // powerLevel='x';
  #else
    // previous way for setting output power
    // powerLevel='M';  
  #endif

  e = sx1272.setPowerDBM((uint8_t)MAX_DBM);
  
  Serial.print(F("Setting Power: state "));
  Serial.println(e);
  
  // Set the node address and print the result
  e = sx1272.setNodeAddress(node_addr);
  Serial.print(F("Setting node addr: state "));
  Serial.println(e, DEC);
  
  // Print a success message
  Serial.println(F("SX1272 successfully configured"));
  delay(500);
}

char *ftoa(char *a, double f, int precision){
   long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
   
   char *ret = a;
   long heiltal = (long)f;
   itoa(heiltal, a, 10);
   while (*a != '\0') a++;
   *a++ = '.';
   long desimal = abs((long)((f - heiltal) * p[precision]));
   if (desimal < p[precision-1]) {
    *a++ = '0';
   }  
   itoa(desimal, a, 10);
   return ret;
}

void loop(void){
    float temp = 13.00; //SENSOR VALUE 1, DESCRIPTION TC
    float hum = 11.00;  //SENSOR VALUE 2, DESCRIPTION HM
    
      //ADD MORE ELSE IF TO INCREASE LIST OF SENSORS TO POST//
      ///////////////////////////
      if(a){
        a = false;
        b = true;
        postValues("\\!TC/%s", temp);
      }else if(b){
        a = true;
        b = false;
        postValues("\\!HM/%s", hum);
      }
      ///////////////////////////
}

void postValues(char* sensorDes, float sensorVal){
      uint8_t r_size;
      char float_str[20];
      long startSend;
      long endSend;
      int e;
      
   if (millis() > nextTransmissionTime){
      ftoa(float_str,sensorVal,2);
      r_size=sprintf((char*)message, sensorDes, float_str);   

      Serial.print(F("Sending "));
      Serial.println((char*)(message));

      Serial.print(F("Real payload size is "));
      Serial.println(r_size);
     
      int pl=r_size;
      
      sx1272.CarrierSense();
   
      startSend=millis();

      // indicate that we have an appkey
      sx1272.setPacketType(PKT_TYPE_DATA);
      
      // Send message to the gateway and print the result
      // with the app key if this feature is enabled
      e = sx1272.sendPacketTimeout(DEFAULT_DEST_ADDR, message, pl);
 
      endSend=millis();
      
      Serial.print(F("LoRa pkt seq "));
      Serial.println(sx1272.packet_sent.packnum);
    
      Serial.print(F("LoRa Sent in "));
      Serial.println(endSend-startSend);+
          
      Serial.print(F("LoRa Sent w/CAD in "));

      Serial.println(endSend-sx1272._startDoCad);

      Serial.print(F("Packet sent, state "));
      Serial.println(e);

      
     //15 Seconds
     nextTransmissionTime=millis()+10000;
  }
}
