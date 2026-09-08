/*
 * ============================================================================
 *  I2CScan  --  hang-proof I2C diagnostic for the KijaniSpace water node
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial: 38400 baud
 *
 *  This version does NOT use the Wire library at all.
 *
 *  Why: the AVR TWI hardware waits for an interrupt that never arrives if the
 *  bus cannot physically be driven, and on cores before 1.8.1 there is no
 *  timeout. Checking that both lines read HIGH beforehand is not enough -
 *  idle-high only proves nothing is pulling them down; it says nothing about
 *  whether they can be pulled down. A line tied to VCC reads HIGH and still
 *  hangs the first transaction.
 *
 *  So everything here is bit-banged open-drain with a bounded wait on every
 *  single edge. It cannot block. If the bus is unusable it says why.
 *
 *  PHASE 1 tests each line on its own: release it (must rise), pull it low
 *  (must fall), release again (must rise). That is the test that finds a
 *  short to VCC, a missing pull-up, or a line held down by a device.
 *
 *  PHASE 2 scans 0x08..0x77 only if phase 1 passed.
 * ============================================================================
 */

#define SDA_PIN A4
#define SCL_PIN A5

const int RAIL33_EN = 6;      // "Sensor Power 1" -> switched 3.3 V
const int ledPin    = 8;      // on-board LED1

const uint16_t RAIL_SETTLE_MS = 1500;   // EZO circuits need ~1 s to boot
const uint16_t SCAN_PERIOD_MS = 5000;

const uint8_t  HALF_US   = 5;    // ~100 kHz bit clock
const uint16_t EDGE_TRIES = 200; // x HALF_US -> ~1 ms per edge, then give up

// ---------------------------------------------------------------------------
//  Open-drain line primitives.
//  "release" means: input, internal pull-up OFF, so we are testing the
//  board's own 4.7 kOhm pull-ups (the I2C_J jumper) and nothing else.
// ---------------------------------------------------------------------------
inline void sdaRelease() { pinMode(SDA_PIN, INPUT); digitalWrite(SDA_PIN, LOW); }
inline void sclRelease() { pinMode(SCL_PIN, INPUT); digitalWrite(SCL_PIN, LOW); }
inline void sdaDriveLow() { digitalWrite(SDA_PIN, LOW); pinMode(SDA_PIN, OUTPUT); }
inline void sclDriveLow() { digitalWrite(SCL_PIN, LOW); pinMode(SCL_PIN, OUTPUT); }
inline bool sdaLevel() { return digitalRead(SDA_PIN); }
inline bool sclLevel() { return digitalRead(SCL_PIN); }

// Release SCL and wait for it to actually rise. False = held low by a device
// (clock stretching that never ends) or shorted to GND.
bool sclReleaseWait() {
  sclRelease();
  for (uint16_t i = 0; i < EDGE_TRIES; i++) {
    if (sclLevel()) return true;
    delayMicroseconds(HALF_US);
  }
  return false;
}

// ---------------------------------------------------------------------------
//  PHASE 1 - can these lines be used as a bus at all?
// ---------------------------------------------------------------------------
// How long does the line take to rise once released? A healthy 4.7 kOhm
// pull-up against normal wiring capacitance is well under 10 us. Hundreds of
// microseconds means a weak pull-up or a heavily loaded line. Never rising at
// all means there is no pull-up: the line floats, and a floating CMOS input
// holds its last level on the pin capacitance alone - which looks exactly like
// "stuck low" if you only sample once.
const uint16_t RISE_LIMIT_US = 4000;

// Residual-voltage bands while we drive a line low.
//
// V_IL for the ATmega328P is 0.3 x VCC, so about 990 mV at 3.3 V. Above that
// a receiver can no longer read the line as LOW and the bus genuinely cannot
// work. Below it the bus still functions, just with less margin - so anything
// in between is a warning, not a stop. A clean line sits near 10-20 mV (the
// AVR's own Ron), and anything well above that means a second driver or a
// hard pull-up is pushing back.
const uint16_t LOW_FAULT_MV = 990;    // above V_IL: cannot signal a low at all
const uint16_t LOW_WARN_MV  = 300;    // clean lines are far below this

uint16_t measureRise(bool isSda) {
  if (isSda) sdaDriveLow(); else sclDriveLow();
  delayMicroseconds(200);                       // fully discharged
  if (isSda) sdaRelease(); else sclRelease();

  for (uint16_t us = 0; us < RISE_LIMIT_US; us += 2) {
    if (isSda ? sdaLevel() : sclLevel()) return us;
    delayMicroseconds(2);
  }
  return 0xFFFF;                                // never got there
}

// Pad voltage in millivolts while we are actively driving the line LOW.
// A4/A5 are ADC channels, so we can read what the pad actually sits at.
// Near 0 mV = we win. Anything high = an external source is fighting us,
// and the number tells you how hard.
uint16_t residualLowMv(bool isSda) {
  if (isSda) sdaDriveLow(); else sclDriveLow();
  delayMicroseconds(500);
  analogRead(isSda ? SDA_PIN : SCL_PIN);        // discard first conversion
  uint16_t counts = analogRead(isSda ? SDA_PIN : SCL_PIN);
  if (isSda) sdaRelease(); else sclRelease();
  return (uint16_t)((uint32_t)counts * 3300UL / 1023UL);
}

bool testLine(const __FlashStringHelper *name, bool isSda) {
  bool ok = true;

  if (isSda) sdaRelease(); else sclRelease();
  delayMicroseconds(500);
  bool high1 = isSda ? sdaLevel() : sclLevel();

  uint16_t mv   = residualLowMv(isSda);
  uint16_t rise = measureRise(isSda);

  Serial.print(F("  "));
  Serial.print(name);
  Serial.print(F(": idle="));
  Serial.print(high1 ? F("H") : F("L"));
  Serial.print(F("  driven-low="));
  Serial.print(mv);
  Serial.print(F(" mV  rise="));
  if (rise == 0xFFFF) Serial.print(F("never"));
  else { Serial.print(rise); Serial.print(F(" us")); }

  if (mv >= LOW_FAULT_MV) {
    Serial.println(F("   FAULT: cannot be pulled LOW"));
    Serial.print(F("    -> held at "));
    Serial.print(mv);
    Serial.println(F(" mV, at or above V_IL (~990 mV)."));
    Serial.println(F("       A receiver cannot read this as a low, so the bus"));
    Serial.println(F("       cannot work and Wire will hang. A low-impedance"));
    Serial.println(F("       3.3 V source is on the line - check that no EZO"));
    Serial.println(F("       VCC pin is wired to A4/A5, and do not leave it"));
    Serial.println(F("       running: the AVR pin is sinking that current."));
    ok = false;
  } else if (mv >= LOW_WARN_MV) {
    Serial.println(F("   WARN: something is fighting the low driver"));
    Serial.print(F("    -> held at "));
    Serial.print(mv);
    Serial.println(F(" mV. Still below V_IL (~990 mV), so the bus"));
    Serial.println(F("       can work - but on 165 mV of margin instead of 980."));
    Serial.println(F("       Most likely an EZO still in UART mode: its TX pin"));
    Serial.println(F("       idles HIGH and drives whichever bus line it is"));
    Serial.println(F("       wired to. Scanning anyway - a UART-mode circuit"));
    Serial.println(F("       will not ACK, so expect 0 devices until you"));
    Serial.println(F("       switch it (LED green -> blue)."));
    // deliberately not a fault: let phase 2 run and produce real information
  } else if (rise == 0xFFFF) {
    Serial.println(F("   FAULT: no pull-up"));
    Serial.println(F("    -> the line never rises, so it is floating. Check the"));
    Serial.println(F("       I2C_J jumper on the back of the board (closed from"));
    Serial.println(F("       the factory) and that A4/A5 are really connected."));
    ok = false;
  } else if (rise > 500) {
    Serial.println(F("   WARN: very slow rise"));
    Serial.println(F("    -> pull-up too weak for the load, or a long cable."));
    Serial.println(F("       I2C may work but marginally."));
  } else if (!high1) {
    Serial.println(F("   FAULT: idle low"));
    Serial.println(F("    -> something is holding the line down."));
    ok = false;
  } else {
    Serial.println(F("   ok"));
  }
  return ok;
}

// Are the two lines shorted to each other? Drive one low and watch the other.
bool testNotShorted() {
  sdaRelease(); sclRelease();
  delayMicroseconds(50);

  sclDriveLow();
  delayMicroseconds(50);
  bool sdaFollowed = !sdaLevel();
  sclRelease();
  delayMicroseconds(50);

  sdaDriveLow();
  delayMicroseconds(50);
  bool sclFollowed = !sclLevel();
  sdaRelease();
  delayMicroseconds(50);

  if (sdaFollowed || sclFollowed) {
    Serial.println(F("  SDA/SCL: FAULT - the two lines are shorted together"));
    Serial.println(F("    -> pulling one down pulls the other with it"));
    return false;
  }
  Serial.println(F("  SDA/SCL: independent  ok"));
  return true;
}

// ---------------------------------------------------------------------------
//  PHASE 2 - bit-banged address probe
// ---------------------------------------------------------------------------
bool i2cStart() {
  sdaRelease();
  if (!sclReleaseWait()) return false;
  delayMicroseconds(HALF_US);
  sdaDriveLow();
  delayMicroseconds(HALF_US);
  sclDriveLow();
  return true;
}

void i2cStop() {
  sdaDriveLow();
  delayMicroseconds(HALF_US);
  sclReleaseWait();
  delayMicroseconds(HALF_US);
  sdaRelease();
  delayMicroseconds(HALF_US);
}

bool i2cWriteBit(bool b) {
  if (b) sdaRelease(); else sdaDriveLow();
  delayMicroseconds(HALF_US);
  if (!sclReleaseWait()) return false;
  delayMicroseconds(HALF_US);
  sclDriveLow();
  return true;
}

// Reads the ACK bit. ack = true means a device pulled SDA low.
bool i2cReadAck(bool *ack) {
  sdaRelease();
  delayMicroseconds(HALF_US);
  if (!sclReleaseWait()) return false;
  *ack = !sdaLevel();
  delayMicroseconds(HALF_US);
  sclDriveLow();
  return true;
}

// Returns: 1 = ACK (device present), 0 = NACK (nobody), -1 = bus fault
int8_t probe(uint8_t addr) {
  if (!i2cStart()) return -1;

  uint8_t byte0 = (addr << 1);          // write bit = 0
  for (int8_t i = 7; i >= 0; i--) {
    if (!i2cWriteBit((byte0 >> i) & 1)) { i2cStop(); return -1; }
  }

  bool ack = false;
  if (!i2cReadAck(&ack)) { i2cStop(); return -1; }

  i2cStop();
  return ack ? 1 : 0;
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);

  sdaRelease();
  sclRelease();

  pinMode(RAIL33_EN, OUTPUT);
  digitalWrite(RAIL33_EN, HIGH);        // sensors need power to answer
  // D7 (12 V rail) is deliberately left as an input - nothing here needs it.

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" I2C scan (bit-banged, cannot hang)"));
  Serial.println(F("=================================================="));
  Serial.println(F(" D6 (3.3 V sensor rail): ON"));
  Serial.print(F(" waiting "));
  Serial.print(RAIL_SETTLE_MS);
  Serial.println(F(" ms for the EZO circuits to boot"));
  delay(RAIL_SETTLE_MS);
  Serial.println();
}

void loop() {
  digitalWrite(ledPin, HIGH);
  Serial.println(F("--------------------------------------------------"));

  Serial.println(F("PHASE 1  line integrity"));
  bool okScl = testLine(F("SCL (A5)"), false);
  bool okSda = testLine(F("SDA (A4)"), true);
  bool okSep = (okScl && okSda) ? testNotShorted() : false;

  if (!(okScl && okSda && okSep)) {
    Serial.println();
    Serial.println(F("Bus is not usable. Fix the wiring before scanning."));
    Serial.println(F("Power the board OFF and check with a multimeter:"));
    Serial.println(F("  A4 <-> A5            must be OPEN"));
    Serial.println(F("  A4 <-> +3V3 (X3-1)   ~4.7k, NOT 0 ohm"));
    Serial.println(F("  A5 <-> +3V3 (X3-1)   ~4.7k, NOT 0 ohm"));
    Serial.println(F("  A4 <-> GND (X1-2)    must be OPEN"));
    Serial.println(F("  A5 <-> GND (X1-2)    must be OPEN"));
    Serial.println(F("  EZO VCC <-> X3-1     ~0 ohm  (this is its supply)"));
    Serial.println(F("  EZO VCC <-> A4 / A5  must be OPEN"));
    Serial.println();
    digitalWrite(ledPin, LOW);
    delay(SCAN_PERIOD_MS);
    return;
  }

  Serial.println();
  Serial.println(F("PHASE 2  address scan 0x08..0x77"));

  uint8_t found = 0;
  bool    fault = false;
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    int8_t r = probe(addr);
    if (r < 0) {
      Serial.print(F("  bus fault while probing 0x"));
      Serial.println(addr, HEX);
      Serial.println(F("  -> a line stopped responding mid-scan; something is"));
      Serial.println(F("     driving the bus intermittently (EZO in UART mode?)"));
      fault = true;
      break;
    }
    if (r == 1) {
      found++;
      Serial.print(F("  0x"));
      if (addr < 0x10) Serial.print('0');
      Serial.print(addr, HEX);
      Serial.print(F("  ACK"));
      if      (addr == 0x63) Serial.print(F("   <-- EZO-pH"));
      else if (addr == 0x64) Serial.print(F("   <-- EZO-EC"));
      Serial.println();
    }
  }

  Serial.print(F("  "));
  Serial.print(found);
  Serial.println(F(" device(s) found."));

  if (found == 0 && !fault) {
    Serial.println(F("  Bus is electrically fine, but nobody answered:"));
    Serial.println(F("   1. Both EZO LEDs BLUE? green = still UART mode,"));
    Serial.println(F("      which cannot answer on I2C at all"));
    Serial.println(F("   2. SDA/SCL wired STRAIGHT, not crossed - I2C goes"));
    Serial.println(F("      SDA->SDA, SCL->SCL; only UART is crossed"));
    Serial.println(F("   3. VCC and GND present at each circuit"));
  }

  Serial.println();
  digitalWrite(ledPin, LOW);
  delay(SCAN_PERIOD_MS);
}
