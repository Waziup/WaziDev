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
 *    12V,1 / 12V,0 switch the 12 V probe rail (D5) on / off (12,1 / 12,0 work
 *                  too). Handled here, never sent to a circuit: the reply
 *                  says "12 V rail ON"; a line "0x64 <- ..." means it went to
 *                  the EZO instead. Off at start. The main firmware
 *                  reads pH and EC with this rail ON and the DO probe
 *                  powered, so a test of what the other probes do to the EC
 *                  reading needs it on (Test Matrix EC-6).
 *
 *    Temp          water temperature from the DS18B20 (A2, 4.7 k to D6+)
 *    Temp,T        the same, then send T,<that value> to the selected circuit.
 *                  Both handled here; check what the circuit took with T,?
 *                  The probe must be IN the water: in air it reads the room.
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
 *    K,?               what cell constant does the circuit think it has?
 *    K,0.1 / K,1.0 / K,10   set it to the probe's own K. Until this is right
 *                      nothing else is, and an uncalibrated circuit answers
 *                      0.0 uS/cm in any liquid - which looks like a dead probe.
 *    Cal,clear         wipe any previous calibration
 *    Cal,dry           IN AIR, probe completely dry. ALWAYS FIRST.
 *    Cal,<value>       single point, e.g. Cal,84 (this file said Cal,one,<value>
 *                      until 19 Oct 2026 - not an Atlas command; check Cal,?)
 *    Cal,low,<value> / Cal,high,<value>    two point
 *    Cal,?             how many points are stored - expect ?CAL,1 or ?CAL,2
 *
 *  VALUES GO IN AS uS/cm, WHATEVER THE BOTTLE SAYS. The standards here are
 *  1413 uS/cm, 12.88 mS/cm and 111.3 mS/cm - a decade series of KCl, 0.01 M,
 *  0.1 M and 1 M. Two of the three labels are MILLI. Typing Cal,12.88
 *  because that is what is printed calibrates a 12880 uS/cm solution as
 *  12.88 uS/cm, and nothing reports an error. Enter 1413, 12880, 111300.
 *
 *  AND NONE OF THEM MATCHES THIS WATER. Lake Victoria is near 100 uS/cm;
 *  the lowest standard here is 14x above that. Order an 84 uS/cm standard
 *  before deployment. Until it arrives: Cal,dry then Cal,1413, and read
 *  12.88 mS/cm afterwards WITHOUT recalibrating to see the linearity.
 *
 *  All three are specified at 25 C / 77 F, so set T,<temperature> first.
 *
 *  Calibration is stored in the circuit's own EEPROM. It survives power loss
 *  and reflashing the ATmega - you do this once per probe, not once per
 *  build.
 * ============================================================================
 */

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const int RAIL33_EN = 6;          // "Sensor Power 1" -> switched 3.3 V
const int ISO_EN    = A1;         // #2810 (B) -> both isolators, off the cell
const int RAIL12_EN = 5;          // 12 V probe rail (carrier); D7 on the bare prototype
const int ledPin    = 8;
const int ONE_WIRE_PIN = A2;      // DS18B20, 4.7 kOhm pull-up to D6(+)

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);

const uint16_t ISO_SETTLE_MS  = 200;    // its DC-DC, before the bus comes up
const uint16_t RAIL_SETTLE_MS = 1500;   // EZO boot

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

// Appends v with two decimals ("24.50", "-1.00") and returns the new end.
// The main firmware's own formatter, so EZOCal sends T exactly as it does.
char *appendFixed2(char *p, float v) {
  if (v < 0) { *p++ = '-'; v = -v; }
  uint32_t n = (uint32_t)(v * 100.0f + 0.5f);
  char tmp[11];
  uint8_t i = 0;
  do { tmp[i++] = '0' + n % 10; n /= 10; } while (n || i < 3);
  while (i) {
    *p++ = tmp[--i];
    if (i == 2) *p++ = '.';
  }
  *p = 0;
  return p;
}

// The DS18B20 as the main firmware reads it: re-enumerate (it may have been
// plugged in since the start), one 12-bit conversion (~750 ms), and the same
// sanity band. -127 = nothing answered, +85 = its power-on value (conversion
// not finished or the supply marginal); neither is a water temperature.
float readWaterTemp() {
  ds18b20.begin();
  ds18b20.requestTemperatures();
  float t = ds18b20.getTempCByIndex(0);
  if (t > -20.0 && t < 60.0) return t;
  Serial.print(F("-> DS18B20 FAIL ("));
  Serial.print(t, 2);
  if (t == DEVICE_DISCONNECTED_C)
    Serial.println(F("): nothing on A2 - probe, wiring, 4.7k pull-up to D6+?"));
  else if (t == 85.0)
    Serial.println(F("): power-on value - conversion or supply"));
  else
    Serial.println(F("): out of range"));
  return NAN;
}

// One command to the selected circuit, with the log line and the bus check.
void sendToTarget(const char *c) {
  Serial.print(F("0x"));
  Serial.print(target, HEX);
  Serial.print(F(" <- "));
  Serial.println(c);
  Serial.flush();

  if (!busIdle()) {
    Serial.println(F("  I2C line stuck LOW - not sending. Run I2CScan."));
    return;
  }

  digitalWrite(ledPin, HIGH);
  if (!ezoSend(target, c)) {
    Serial.println(F("  no reply - wrong address, or the circuit is not on"));
  }
  digitalWrite(ledPin, LOW);
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

  // local: the 12 V probe rail. No EZO command starts with a digit, so
  // ANYTHING beginning with "12" stays here: "12V,1", "12v,1" and the short
  // "12,1". Until 5 Nov 2026 only "12V," was caught, and a "12,1" typed
  // without the V went to the EZO circuit, which answered [OK] - while the
  // rail stayed off.
  if (cmd[0] == '1' && cmd[1] == '2') {
    const char *a = cmd + 2;
    if (*a == 'V' || *a == 'v') a++;
    char sw = (a[0] == ',' && !a[2]) ? a[1] : 0;
    if (sw == '1') {
      digitalWrite(RAIL12_EN, HIGH);
      Serial.println(F("-> 12 V rail ON (D5): DO/turbidity probes powered."));
      Serial.println(F("   Check 12 V at the probe terminal; wait ~20 s"));
      Serial.println(F("   before a baseline R."));
    } else if (sw == '0') {
      digitalWrite(RAIL12_EN, LOW);
      Serial.println(F("-> 12 V rail OFF (D5)"));
    } else {
      Serial.println(F("-> use 12V,1 or 12V,0"));
    }
    cmdLen = 0;
    return;
  }

  // local: the water temperature. No EZO command starts with "Temp" (theirs
  // is "T,"), so this never shadows one.
  if (!strncasecmp(cmd, "temp", 4)) {
    bool toEzo = !strcasecmp(cmd + 4, ",t");
    if (cmd[4] && !toEzo) {
      Serial.println(F("-> use Temp or Temp,T"));
    } else {
      float t = readWaterTemp();
      if (!isnan(t)) {
        Serial.print(F("-> water "));
        Serial.print(t, 2);
        Serial.println(F(" degC (DS18B20)"));
        if (toEzo) {
          char c[16] = "T,";
          appendFixed2(c + 2, t);
          sendToTarget(c);
        }
      }
    }
    cmdLen = 0;
    return;
  }

  sendToTarget(cmd);
  cmdLen = 0;
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(RAIL12_EN, OUTPUT);
  digitalWrite(RAIL12_EN, LOW);         // 12 V only on request: 12V,1

  // THE ISOLATORS COME UP FIRST - same invariant as the main sketch.
  // An unpowered ADM3260 holds SDA and SCL down on both sides, so without
  // this nothing on the bus answers, not even the circuits that sit nowhere
  // near an isolator. THE CELL MUST BE CONNECTED: A1 only gates the #2810,
  // it does not supply it, and the FTDI alone will not run the isolators.
  pinMode(ISO_EN, OUTPUT);
  digitalWrite(ISO_EN, HIGH);
  delay(ISO_SETTLE_MS);

  pinMode(RAIL33_EN, OUTPUT);
  digitalWrite(RAIL33_EN, HIGH);        // circuits stay powered the whole time
  delay(RAIL_SETTLE_MS);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" EZO calibration console  --  I2C"));
  Serial.println(F(" Isolators ON (A1) - the cell must be connected."));
  Serial.println(F("=================================================="));
  Serial.println(F(" Type a command and press Enter. Any line ending."));
  Serial.println(F("   #63 / #64   choose pH or EC"));
  Serial.println(F("   i           device type - start here"));
  Serial.println(F("   Cal,?       how many points are stored"));
  Serial.println(F("   R           one reading"));
  Serial.println(F("   12V,1/12V,0 12 V probe rail on/off (off now)"));
  Serial.println(F("   Temp        water temperature (DS18B20)"));
  Serial.println(F("   Temp,T      ... and send it as T to the circuit"));
  Serial.println();
  Serial.println(F(" pH:  Cal,mid,7.00  then  Cal,low,4.00  then"));
  Serial.println(F("      Cal,high,10.00   (mid FIRST - it clears the rest)"));
  Serial.println(F(" EC:  Cal,dry  then  Cal,<value>"));
  Serial.println(F("--------------------------------------------------"));

  if (!busIdle()) {
    Serial.println(F(" WARNING: SDA or SCL is LOW with nothing sent."));
    Serial.println(F(" Missing pull-up, or a circuit holding the bus."));
    Serial.println(F(" With the isolator fitted, check the ISOLATED side:"));
    Serial.println(F(" its data lines need their own pull-ups to the"));
    Serial.println(F(" isolated 3.9 V, referenced to isolated GND."));
  }

  ds18b20.begin();                      // D6 is up: the DS18B20 has power

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
