
#ifndef wazidev_h
#define wazidev_h

#include "SX1272.h"


class WaziDev
{
  public:
    //setup WaziDev
    WaziDev(int nodeAddr);
    
    WaziDev(int nodeAddr, int destAddr, int loraMode, int channel, int maxDBm);

    //setup the WaziDev
    void setup();
   
    //read a sensor
    int getSensorValue(int pin);

    //Send a LoRa message
    void send(char sensor_id[], float val);

    // Power down the WaziDev for "duration" seconds
    void powerDown(const int duration); 
    
    // Write a message to the serial monitor
    void writeSerial(const char* format, ...);

  private:
    uint8_t  maxDBm = 14;
    uint32_t channel = CH_10_868;
    int nodeAddr = 8;
    int destAddr = 1;
    int loraMode = 1;
    
    struct Config {
    
      uint8_t flag1;
      uint8_t flag2;
      uint8_t seq;
      // can add other fields such as LoRa mode,...
    };

    Config config;
    

};

#endif
