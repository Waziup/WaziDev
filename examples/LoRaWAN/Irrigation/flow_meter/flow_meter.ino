volatile int NumPulses = 0;  //variable for the number of pulses received
int PinSensor = 2;     //Sensor connected to pin 10
float factor_conversion = 7.5;  //to convert from frequency to flow rate
float volume = 0;
long dt = 0; //time variation for each loop
long t0 = 0; //millis() from the previous loop

//---Function executed in interrupt---------------
void PulseCount()
{
  NumPulses++;   //increment the pulse variable
}

//---Function to obtain pulse frequency--------
int GetFrequency()
{
  int frequency;
  NumPulses = 0;    //We set the number of pulses to 0
  interrupts();     // We enable the interruptions
  delay(1000);      //sample for 1 second
  noInterrupts();   // We disable the interruptions
  frequency = NumPulses;  //Hz(pulses per second)
  
  return frequency;
}

void setup()
{
  Serial.begin(9600);
  pinMode(PinSensor, INPUT);
  attachInterrupt(digitalPinToInterrupt(PinSensor), PulseCount, RISING); //Using the correct interrupt pin
  Serial.println("Send 'r' to reset the volume to 0 Liters");
  t0 = millis();
}

void loop()
{
  if (Serial.available()) {
    if (Serial.read() == 'r') volume = 0; //reset the volume if we receive 'r'
  }
  float frequency = GetFrequency();  //we obtain the frequency of the pulses in Hz
  float flow_L_m = frequency / factor_conversion; //calculate the flow in L/m
  dt = millis() - t0;  //calculate the time variation
  t0 = millis();
  volume = volume + (flow_L_m / 60) * (dt / 1000);  // volume(L)=flow(L/s)*time(s)

  //-----Send through the serial port---------------
  Serial.print("Flow: ");
  Serial.print(flow_L_m, 3);
  Serial.print(" L/min\tVolume: ");
  Serial.print(volume, 3);
  Serial.print(" L\tFrequency: ");
  Serial.println(frequency, 3);
};
