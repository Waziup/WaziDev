/*
 * ============================================================================
 *  VccCal  --  the two numbers behind the battery reading
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud (read-only, nothing to type)
 *  Tool  : a multimeter. Run on the BATTERY, not the FTDI - you are
 *          calibrating against the cell.
 *
 *  WHY TWO NUMBERS
 *  The main sketch computes
 *
 *      battery = analogRead(A0) * (Vcc / 1023) * DIVIDER
 *
 *  and it has to measure Vcc, because Vcc IS the ADC's reference: the raw
 *  count alone says nothing about volts. Vcc comes from the AVR's internal
 *  1.1 V bandgap, read back through the ADC, and every chip's bandgap is off
 *  by a percent or two - that is what VccCorrection trims. DIVIDER then turns
 *  the voltage at A0 into the cell voltage.
 *
 *  Both are pure multipliers on the same result, so ONE meter reading cannot
 *  separate them. This sketch reports the raw halves and asks for two.
 *
 *  IMPORTANT: correction is deliberately 1.0 here, so the printed Vcc is the
 *  untrimmed bandgap reading. Do not "fix" that to match the meter - it is
 *  the input to the calculation, not an answer.
 *
 *  Both rails stay OFF, which is the state the main sketch measures the
 *  battery in (readBattery() runs after railsOff()). Same load, same numbers.
 * ============================================================================
 */

#include <Vcc.h>

const int batt_pin  = A0;
const int RAIL33_EN = 6;
const int RAIL12_EN = 7;
const int ledPin    = 8;

const uint8_t SAMPLES = 16;

Vcc vcc(1.0);                 // untrimmed on purpose

void setup() {
  Serial.begin(38400);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Same conditions as readBattery(): rails off, nothing drawing.
  pinMode(RAIL33_EN, OUTPUT);
  pinMode(RAIL12_EN, OUTPUT);
  digitalWrite(RAIL33_EN, LOW);
  digitalWrite(RAIL12_EN, LOW);

  analogRead(batt_pin);       // discard the first conversion after the mux
                              // settles on a high-impedance divider
  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" Battery calibration"));
  Serial.println(F("=================================================="));
  Serial.println(F(" Take TWO meter readings while this runs:"));
  Serial.println(F("   (1) Vcc      -> the 3.3 V rail, easiest at the"));
  Serial.println(F("                   \"Sensor Power\" PLUS terminal,"));
  Serial.println(F("                   which is permanently live"));
  Serial.println(F("   (2) the CELL -> at the battery input"));
  Serial.println();
  Serial.println(F(" Then report those two, plus the two averaged lines"));
  Serial.println(F(" below. Run on the battery, not the FTDI."));
  Serial.println(F("--------------------------------------------------"));
}

void loop() {
  float bandgap = 0;
  for (uint8_t i = 0; i < SAMPLES; i++) { bandgap += vcc.Read_Volts(); delay(2); }
  bandgap /= SAMPLES;

  float counts = 0;
  for (uint8_t i = 0; i < SAMPLES; i++) { counts += analogRead(batt_pin); delay(2); }
  counts /= SAMPLES;

  Serial.print(F("Vcc untrimmed = "));
  Serial.print(bandgap, 4);
  Serial.print(F(" V     A0 counts = "));
  Serial.println(counts, 1);

  // What the main sketch would report right now, for reference only.
  Serial.print(F("  -> main sketch would say "));
  Serial.print((counts * ((bandgap * (3.85 / 7.5)) / 1023.0)) * 3.83, 2);
  Serial.println(F(" V"));

  digitalWrite(ledPin, HIGH);
  delay(60);
  digitalWrite(ledPin, LOW);
  delay(2940);
}
