/********************
 * Program:  DHT11 sensor tester
 * Description: displays infos on a LCD display 
 * Connections:
 * VSS -> WaziDev GND
 * VDD -> WaziDev +5v
 * V0 -> potentiometre
 * RS -> Arduino D12
 * R/W -> GND
 * E ->  WaziDev D11
 * D0 -> N/C
 * D1 -> N/C
 * D2 -> N/C
 * D3 -> N/C
 * D4 -> WaziDev D2
 * D5 -> WaziDev D3
 * D6 -> WaziDev D4
 * D7 -> WaziDev D5
 * A ->  WaziDev A0
 * K -> GND
 ***********************/

#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 2, 3, 4, 5);

void printLCD(int theLine, const char* theInfo, boolean clearFirst=true) {
  if (clearFirst) {
    lcd.clear();
  }
  lcd.setCursor(0, theLine);
  lcd.print(theInfo);
}  

void setup()
{
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);
  
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.home();
  lcd.print("Setup");
  printLCD(0,"   Welcome to", true);
  printLCD(1,"    WaziDev!", false);

  delay(500);
}


void loop(void)
{

  lcd.setCursor(0, 1);
  
  delay(1000);
}
