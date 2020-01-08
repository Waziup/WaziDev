/********************
 * Program: LoRa Ping Pong 
 * Description: Play ping pong with WaziGate
 ********************/
 
#include <WaziDev.h>
#include <Utils.cpp>

// new WaziDev with node address = 8 
WaziDev wazidev("MyDevice", 8);

void setup()
{
  wazidev.setup();
  delay(500);
}


void loop(void)
{

  wazidev.writeSerial("Sending Ping\n");  
  
  int res = wazidev.send("Ping");
      
  if (res==0) {
      wazidev.writeSerial("Pong received from gateway!\n");
      wazidev.writeSerial("SNR at gw=%d\n", sx1272._rcv_snr_in_ack);
  } else {
      wazidev.writeSerial("No Pong from gw!\n");

  }
  delay(10000);    
}
