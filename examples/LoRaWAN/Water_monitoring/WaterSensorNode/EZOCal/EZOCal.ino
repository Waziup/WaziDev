/*
 * ============================================================================
 *  EZOCal  --  talk to the EZO circuits over I2C, for calibration
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud. TYPE COMMANDS HERE.
 *
 *  WHY THIS EXISTS ALONGSIDE EZOBridge
 *  EZOBridge is a UART console. It was the tool that CARRIED OUT the switch
 *  to I2C ("I2C,99" / "I2C,100") - and the moment that switch took effect it
 *  stopped being able to reach the circuits at all. A UART console talking to
 *  an I2C-mode circuit gets silence, every time, and nothing about the silence
 *  says why. This sketch speaks I2C, so it works on the circuits as they are
 *  now, and there is no reason to put them back into UART mode.
 *
 *  COMMANDS
 *    #63 / #64     select which circuit to talk to (pH = 0x63, EC = 0x64)
 *    i             device type and firmware - always start here
 *    Status        restart reason and supply voltage as the circuit sees it
 *    R             take one reading
 *    Cal,?         how many calibration points are stored
 *    T,25.0        tell the circuit the water temperature (pH and EC both
 *                  need this; an uncompensated reading is off by a lot)
 *
 *  pH CALIBRATION - MIDPOINT FIRST, AND IT CLEARS THE OTHERS
 *    Cal,mid,7.00      in pH 7 buffer   <- always first, it wipes the rest
 *    Cal,low,4.00      in pH 4 buffer
 *    Cal,high,10.00    in pH 10 buffer
 *  Rinse the probe with distilled water between buffers and let the reading
 *  settle - watch R until it stops moving, usually 1-2 minutes in fresh
 *  buffer. Calibrating on a drifting value bakes the drift in.
 *
 *  EC CALIBRATION
 *    Cal,dry           IN AIR, probe completely dry. Always first.
 *    Cal,one,<value>   single point, in a standard close to your water
 *  Lake Victoria sits near 100 uS/cm, so an 84 uS/cm standard is the better
 *  single point for this deployment than the common 1413 uS/cm one.
 *
 *  Calibration is stored in the circuit's own EEPROM. It survives power loss
 *  and reflashing the ATmega - you do this once per probe, not once per
 *  build.
 * ============================================================================
 */

#include <Wire.h>

const int RAIL33_EN = 6;          // "Sensor Power 1" -> switched 3.3 V
const int ledPin    = 8;

const uint16_t RAIL_SETTLE_MS = 1500;   // EZO boot, plus the isolator's DC-DC

// One wait for every command. The slowest documented processing delay on
// these circuits is 900 ms (R and Cal); 1600 ms covers it with margin, and
// the cost of being generous in an interactive tool is nothing.
const uint16_t REPLY_WAIT_MS = 1600;

// Dispatch on idle rather than on a line ending, so it does not matter
// whether the monitor is set to "No line ending", "Newline" or "Both".
const uint16_t CMD_IDLE_MS = 600;

uint8_t target = 0x63;            // pH by default

char cmd[40];
uint8_t cmdLen = 0;
uint32_t lastChar = 0;

char resp[40];

// ---------------------------------------------------------------------------
//  A blocking Wire call cannot be interrupted on this AVR core - there is no
//  timeout before core 1.8.1. If a line is stuck low, endTransmission() waits
//  forever and the sketch dies with no message. Check before trusting it.
//  This is a net, not a cure: a line held low by a DRIVER also reads low, and
//  only I2CScan's bit-banged measurement tells those apart.
// ---------------------------------------------------------------------------
bool busIdle() {
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  delayMicroseconds(50);
  return digitalRead(A4) && digitalRead(A5);
}

bool ezoSend(uint8_t addr, const char *c) {
  Wire.beginTransmission(addr);
  Wire.write((const uint8_t *)c, strlen(c));
  if (Wire.endTransmission() != 0) return false;

  delay(REPLY_WAIT_MS);

  Wire.requestFrom(addr, (uint8_t)sizeof(resp));
  if (!Wire.available()) return false;

  uint8_t code = Wire.read();
  uint8_t i = 0;
  while (Wire.available()) {
    char ch = Wire.read();
    if (ch == 0) break;
    if (i < sizeof(resp) - 1) resp[i++] = ch;
  }
  resp[i] = 0;
  while (Wire.available()) Wire.read();

  Serial.print(F("  ["));
  switch (code) {
    case 1:   Serial.print(F("OK"));      break;
    case 2:   Serial.print(F("FAILED"));  break;
    case 254: Serial.print(F("PENDING")); break;   // asked back too early
    case 255: Serial.print(F("NO DATA")); break;   // command returns nothing
    default:  Serial.print(code);         break;
  }
  Serial.print(F("] "));
  Serial.println(resp);
  return (code == 1);
}

void dispatch() {
  cmd[cmdLen] = 0;

  // strip anything the monitor appended
  while (cmdLen && (cmd[cmdLen - 1] == '\r' || cmd[cmdLen - 1] == '\n' ||
                    cmd[cmdLen - 1] == ' ')) {
    cmd[--cmdLen] = 0;
  }
  if (!cmdLen) return;

  if (cmd[0] == '#') {
    if (!strcmp(cmd + 1, "63")) {
      target = 0x63;
      Serial.println(F("-> pH  0x63"));
    } else if (!strcmp(cmd + 1, "64")) {
      target = 0x64;
      Serial.println(F("-> EC  0x64"));
    } else {
      Serial.println(F("-> use #63 or #64"));
    }
    cmdLen = 0;
    return;
  }

  Serial.print(F("0x"));
  Serial.print(target, HEX);
  Serial.print(F(" <- "));
  Serial.println(cmd);
  Serial.flush();

  if (!busIdle()) {
    Serial.println(F("  I2C line stuck LOW - not sending. Run I2CScan."));
    cmdLen = 0;
    return;
  }

  digitalWrite(ledPin, HIGH);
  if (!ezoSend(target, cmd)) {
    Serial.println(F("  no reply - wrong address, or the circuit is not on"));
  }
  digitalWrite(ledPin, LOW);

  cmdLen = 0;
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(RAIL33_EN, OUTPUT);
  digitalWrite(RAIL33_EN, HIGH);        // circuits stay powered the whole time
  delay(RAIL_SETTLE_MS);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" EZO calibration console  --  I2C"));
  Serial.println(F("=================================================="));
  Serial.println(F(" Type a command and press Enter. Any line ending."));
  Serial.println(F("   #63 / #64   choose pH or EC"));
  Serial.println(F("   i           device type - start here"));
  Serial.println(F("   Cal,?       how many points are stored"));
  Serial.println(F("   R           one reading"));
  Serial.println();
  Serial.println(F(" pH:  Cal,mid,7.00  then  Cal,low,4.00  then"));
  Serial.println(F("      Cal,high,10.00   (mid FIRST - it clears the rest)"));
  Serial.println(F(" EC:  Cal,dry  then  Cal,one,<value>"));
  Serial.println(F("--------------------------------------------------"));

  if (!busIdle()) {
    Serial.println(F(" WARNING: SDA or SCL is LOW with nothing sent."));
    Serial.println(F(" Missing pull-up, or a circuit holding the bus."));
    Serial.println(F(" With the isolator fitted, check the ISOLATED side:"));
    Serial.println(F(" its data lines need their own pull-ups to the"));
    Serial.println(F(" isolated 3.9 V, referenced to isolated GND."));
  }

  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif

  digitalWrite(ledPin, LOW);
  Serial.println(F(" Ready."));
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (cmdLen < sizeof(cmd) - 1) cmd[cmdLen++] = c;
    lastChar = millis();
  }
  if (cmdLen && (millis() - lastChar) > CMD_IDLE_MS) dispatch();
}
