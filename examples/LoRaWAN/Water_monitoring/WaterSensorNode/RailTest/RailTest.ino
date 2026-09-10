/*
 * ============================================================================
 *  RailTest  --  does the switched 3.3 V rail actually switch off?
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud (read-only)
 *  Tools: a multimeter, and the ammeter on your bench supply.
 *
 *  THE MEASUREMENT THIS SETTLES
 *  Sleep current came out at 20.78 mA, and disconnecting each device showed
 *  where it goes: EZO-pH 5.58 mA, EZO-EC 4.58 mA, transceiver plus bias
 *  4.04 mA. Those are normal operating currents of powered devices, not
 *  leakage - so during sleep the switched 3.3 V rail is live when it should
 *  be dead. Software cannot fix that; something is feeding those devices.
 *
 *  There are three possible reasons, and one toggle separates them:
 *
 *    1. D6 is low but the on-board MOSFET does not disconnect
 *       -> terminal stays at 3.3 V in both states
 *
 *    2. The devices are not fed from the D6 terminal at all - their VCC
 *       went to the board's PERMANENT 3V3 pin during one of the rewiring
 *       rounds
 *       -> terminal switches correctly, but a device's own VCC pin stays
 *          at 3.3 V and the current does not drop
 *
 *    3. Everything is fine and the drain is elsewhere
 *       -> terminal switches, current drops by about 14 mA
 *
 *  WHAT TO DO WHILE THIS RUNS
 *  It toggles both rails every 10 s and announces each state. In each state,
 *  measure:
 *
 *    a) "Sensor Power 1" + terminal, against GND
 *    b) the VCC pin of ONE EZO circuit, against GND
 *    c) the MAX3485 module's VCC pin, against GND
 *    d) total supply current
 *
 *  Reading (a) low while (b) or (c) stays at 3.3 V is reason 2 above, and it
 *  is the one worth suspecting first: this build has been rewired repeatedly,
 *  and the difference between the D6 terminal and the permanent 3V3 pin is
 *  two adjacent connection points.
 * ============================================================================
 */

const int RAIL33_EN = 6;      // "Sensor Power 1" -> switched 3.3 V
const int RAIL12_EN = 7;      // "Sensor Power 2" -> control of the 12 V rail
const int ledPin    = 8;

const uint16_t STATE_MS = 10000;

// Everything that leads to a switched-rail device, parked so it cannot feed
// anything through an ESD clamp and cannot float. Same list as the main
// sketch's parkPinsForSleep(), so this test sees the same conditions.
const uint8_t PARKED[] = {3, 4, A4, A5, A2};

void parkAll() {
  for (uint8_t i = 0; i < sizeof(PARKED); i++) {
    digitalWrite(PARKED[i], LOW);
    pinMode(PARKED[i], OUTPUT);
  }
  TWCR = 0;                     // TWI off, so its internal pull-ups are gone
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  pinMode(RAIL33_EN, OUTPUT);
  pinMode(RAIL12_EN, OUTPUT);
  digitalWrite(RAIL33_EN, LOW);
  digitalWrite(RAIL12_EN, LOW);

  parkAll();

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" Rail switching test"));
  Serial.println(F("=================================================="));
  Serial.println(F(" Both rails toggle every 10 s. Measure in each state:"));
  Serial.println(F("   a) Sensor Power 1 + terminal  to GND"));
  Serial.println(F("   b) one EZO circuit's VCC pin  to GND"));
  Serial.println(F("   c) MAX3485 module's VCC pin   to GND"));
  Serial.println(F("   d) total supply current"));
  Serial.println();
  Serial.println(F(" Every MCU pin that touches a sensor is driven LOW and"));
  Serial.println(F(" the TWI unit is off, so nothing can be fed through a"));
  Serial.println(F(" data line. Whatever you still measure is a supply path."));
  Serial.println(F("--------------------------------------------------"));
}

void announce(bool on) {
  Serial.println();
  
  // Apply the F() macro to each static string individually
  if (on) {
    Serial.print(F("RAILS ON   (D6 HIGH, D7 HIGH)"));
  } else {
    Serial.print(F("RAILS OFF  (D6 LOW,  D7 LOW)"));
  }
  
  Serial.print(F("   for "));
  Serial.print(STATE_MS / 1000);
  Serial.println(F(" s"));
  
  if (on) {
    Serial.println(F("  expect: terminal 3.3 V, devices 3.3 V, current high"));
  } else {
    Serial.println(F("  expect: terminal 0 V, devices 0 V, current ~14 mA lower"));
    Serial.println(F("  terminal 3.3 V here      -> the MOSFET is not switching"));
    Serial.println(F("  terminal 0 V but VCC 3.3 -> that device is wired to the"));
    Serial.println(F("                              permanent 3V3, not to D6"));
  }
  Serial.flush();
}

void loop() {
  announce(true);
  digitalWrite(RAIL33_EN, HIGH);
  digitalWrite(RAIL12_EN, HIGH);
  // Slow blink while the rails are up, so the state is visible from the bench
  // without reading the log.
  for (uint16_t t = 0; t < STATE_MS; t += 500) {
    digitalWrite(ledPin, (t / 500) & 1);
    delay(500);
  }
  digitalWrite(ledPin, LOW);

  announce(false);
  digitalWrite(RAIL33_EN, LOW);
  digitalWrite(RAIL12_EN, LOW);
  parkAll();                    // re-park: nothing may feed the dead rail
  delay(STATE_MS);
}
