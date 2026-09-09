/*
 * ============================================================================
 *  RS485Drive  --  prove the node's own RS485 output works, with no sensor
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud (read-only)
 *  Tool needed: a plain DC multimeter. No scope, no sensor, no probe.
 *
 *  When a Modbus device stays silent, the fault is either on the node side or
 *  on the sensor side, and "timeout" cannot tell you which. This sketch tests
 *  the node side alone, in two steps you can read off a multimeter.
 *
 *  ---------------------------------------------------------------------------
 *  STEP 1  IDLE.  The fail-safe bias network is a plain resistor divider:
 *
 *      +3V3 --[680R]-- A --[120R]-- B --[680R]-- GND
 *
 *  3.3 V across 1480 ohms is 2.23 mA, which fixes all three voltages:
 *
 *      A to GND   ~1.78 V
 *      B to GND   ~1.52 V
 *      A minus B  ~268 mV      <- the number the single terminator was chosen for
 *
 *  Measuring that proves four things at once: the transceiver has its 3.3 V,
 *  both bias resistors are on the right rails, the 120 ohm terminator is
 *  across A-B and not shorting, and A/B actually reach the terminal block.
 *
 *  ---------------------------------------------------------------------------
 *  STEP 2  DRIVING.  The sketch then transmits 0x55 continuously - alternating
 *  bits, so the driver is active essentially all the time and an
 *  auto-direction module keeps its transmitter enabled.
 *
 *  While driving, A and B swing against each other at 50 % duty, so their DC
 *  averages both move toward mid-rail and the differential COLLAPSES:
 *
 *      A minus B  falls from ~268 mV to roughly 0 mV
 *
 *  If that number does not move, the transceiver is not driving the bus:
 *  either DI is not reaching it from D4, or the module is faulty. That is a
 *  node-side fault and no amount of address or baud hunting will fix it.
 *
 *  If it does move, the node side is proven and the fault is beyond the
 *  terminal block: A/B swapped, sensor power, or the sensor itself.
 * ============================================================================
 */

#include <SoftwareSerial.h>

const int RS485_RX_PIN = 3;
const int RS485_TX_PIN = 4;
const int RAIL33_EN    = 6;
const int RAIL12_EN    = 7;
const int ledPin       = 8;

const uint32_t RS485_BAUD = 4800;

const uint16_t IDLE_PHASE_MS  = 20000;
const uint16_t DRIVE_PHASE_MS = 20000;

SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

void banner() {
  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" RS485 drive test  --  multimeter, no sensor"));
  Serial.println(F("=================================================="));
  Serial.println(F(" Measure at the terminal block: X2-3 = A, X2-4 = B,"));
  Serial.println(F(" X2-2 or X1-2 = GND. The two phases below alternate"));
  Serial.println(F(" forever, so take your time."));
  Serial.println(F("--------------------------------------------------"));
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);

  pinMode(RAIL33_EN, OUTPUT);
  pinMode(RAIL12_EN, OUTPUT);

  digitalWrite(RAIL33_EN, HIGH);        // transceiver + bias need this
  delay(1200);
  digitalWrite(RAIL12_EN, LOW);         // no sensor involved in this test

  rs485.begin(RS485_BAUD);

  banner();
  Serial.println(F(" 3.3 V rail ON. 12 V rail deliberately OFF - this test"));
  Serial.println(F(" is about the node, not the probe."));
}

void idlePhase() {
  digitalWrite(ledPin, LOW);
  Serial.println();
  Serial.println(F("PHASE IDLE  (20 s)  transmitter quiet"));
  Serial.println(F("  expect   A = 1.78 V   B = 1.52 V   A-B = +268 mV"));
  Serial.println(F("  both 0 V      -> transceiver unpowered, or A/B not wired"));
  Serial.println(F("  A-B = 0 mV    -> 120R misplaced, or both bias resistors"));
  Serial.println(F("                   on the same rail"));
  Serial.println(F("  A-B negative  -> your A and B are swapped"));
  delay(IDLE_PHASE_MS);
}

void drivePhase() {
  Serial.println();
  Serial.println(F("PHASE DRIVING  (20 s)  sending 0x55 continuously"));
  Serial.println(F("  expect   A-B to COLLAPSE toward 0 mV"));
  Serial.println(F("  unchanged at +268 mV -> the transceiver is NOT driving."));
  Serial.println(F("                          D4 is not reaching DI, or the"));
  Serial.println(F("                          module is dead. Node-side fault."));

  uint32_t t0 = millis();
  while ((millis() - t0) < DRIVE_PHASE_MS) {
    rs485.write((uint8_t)0x55);
    digitalWrite(ledPin, ((millis() / 100) & 1) ? HIGH : LOW);
  }
  digitalWrite(ledPin, LOW);
}

void loop() {
  idlePhase();
  drivePhase();
}
