/*  SettleTime - how long does a probe need before its reading is TRUE?
 *  ---------------------------------------------------------------------
 *  DO_WARMUP_MS in the node firmware is 30 s and it was guessed. The sensor
 *  answers after 0.4 s, but answering is not the same as having settled, and
 *  DFRobot publishes no figure. This sketch measures it.
 *
 *  WHY IT DOES NOT SIMPLY WATCH FOR THE READING TO "STOP MOVING"
 *
 *  A slow drift flattens out for a while and then carries on. Judged only on
 *  local flatness you would declare it settled in the middle of the ramp and
 *  bake a wrong number into every uplink the node ever sends. So this sketch
 *  does it the other way round: it records the whole run, takes the LAST
 *  REF_SECONDS as the final value, and then asks when the reading first came
 *  within TOLERANCE of that final value AND never left again.
 *
 *  That is backwards-looking and cannot be done live - which is exactly why
 *  it is a bench tool and not something the node does for itself.
 *
 *  It also reports the naive flatness criterion, because the gap between the
 *  two numbers tells you how badly you would have been fooled.
 *
 *  WIRING is the node's own: nothing to change. Rails are switched here the
 *  same way the firmware switches them, and the clock starts at rail-on, so
 *  the result can be pasted straight into DO_WARMUP_MS.
 *
 *  SERIAL MONITOR AT 38400.
 */

#include <SoftwareSerial.h>

// ---- what to measure -----------------------------------------------------
//  0 = DFRobot SEN0680 dissolved oxygen (the open question)
//  1 = Yosemitech Y511-A turbidity      (same method, for comparison)
#define TARGET_TURBIDITY   0

// ---- the run -------------------------------------------------------------
const uint16_t SAMPLE_MS   = 2000;   // one reading every 2 s
const uint16_t RUN_S       = 240;    // total run length, 4 minutes
const uint8_t  REF_S       = 30;     // the last 30 s define the final value

//  TOLERANCE is the band around the final value that counts as "arrived".
//  Set it from the sensor's own noise, not from what looks tidy: run this
//  once, look at how much the last minute wobbles, and use a little more than
//  that. Too tight and you are timing the noise, not the settling.
#if TARGET_TURBIDITY
const float TOLERANCE = 0.20;        // NTU
const float ACCURACY  = 0.30;        // Y511-A, +/-0.3 NTU
#else
const float TOLERANCE = 0.10;        // mg/L
const float ACCURACY  = 0.60;        // SEN0680, +/-3 % of a 20 mg/L full scale
#endif

//  TOLERANCE answers "when did it stop moving". ACCURACY answers the question
//  that actually decides DO_WARMUP_MS: "when did it get closer to the truth
//  than this sensor can resolve anyway". Chasing drift below ACCURACY buys
//  precision the probe does not have, and on a power-gated node every second
//  of it is paid for 48 times a day.

// ---- pins, as on the node ------------------------------------------------
const int RS485_RX_PIN = 3;
const int RS485_TX_PIN = 4;
const int RAIL12_PIN   = 5;
const int RAIL33_PIN   = 6;

SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

// ---- the device ----------------------------------------------------------
#if TARGET_TURBIDITY
const uint32_t BAUD = 9600;   const uint8_t  ADDR = 0x01;
const uint16_t REG  = 0x2602; const bool     DCBA = true;    // fully reversed
const char     UNIT[] PROGMEM = "NTU";
#else
const uint32_t BAUD = 4800;   const uint8_t  ADDR = 0x01;
const uint16_t REG  = 0x0002; const bool     DCBA = false;   // big endian
const char     UNIT[] PROGMEM = "mg/L";
#endif

const uint16_t MODBUS_TIMEOUT_MS = 1000;
const uint8_t  MODBUS_RETRIES    = 3;

// ---- storage -------------------------------------------------------------
//  Values are kept as hundredths in an int16 so the whole run fits in RAM:
//  120 samples cost 240 bytes instead of 480. Range +/-327 covers mg/L and
//  NTU up to 327, which is past anything this probe will see in a pond.
const uint8_t NSAMP = RUN_S * 1000UL / SAMPLE_MS;
int16_t  buf[NSAMP];
uint8_t  nGot = 0;

const int16_t BAD = -32768;          // a failed read, kept in sequence


// ==========================================================================
//  Modbus RTU, function 0x03 - identical on the wire to the node firmware
// ==========================================================================
uint16_t modbusCRC(uint8_t *buf, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= buf[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

bool readRegisters(uint8_t slave, uint16_t reg, uint8_t count, uint16_t *out) {
  for (uint8_t attempt = 0; attempt < MODBUS_RETRIES; attempt++) {
    uint8_t req[8];
    req[0] = slave; req[1] = 0x03;
    req[2] = reg >> 8; req[3] = reg & 0xFF;
    req[4] = 0x00;     req[5] = count;
    uint16_t crc = modbusCRC(req, 6);
    req[6] = crc & 0xFF; req[7] = crc >> 8;

    while (rs485.available()) rs485.read();
    rs485.write(req, 8);
    rs485.flush();
    delay(2);

    const uint8_t expected = 5 + count * 2;
    uint8_t resp[16], got = 0;
    unsigned long t0 = millis();
    while (got < expected && (millis() - t0) < MODBUS_TIMEOUT_MS)
      if (rs485.available()) resp[got++] = rs485.read();

    if (got < expected)                       continue;
    if (resp[0] != slave || resp[1] != 0x03)  continue;
    if (modbusCRC(resp, expected - 2) !=
        ((uint16_t)resp[expected - 1] << 8 | resp[expected - 2])) continue;

    for (uint8_t i = 0; i < count; i++)
      out[i] = ((uint16_t)resp[3 + i * 2] << 8) | resp[4 + i * 2];
    return true;
  }
  return false;
}

//  Two devices on this bus disagree about byte order: the SEN0680 is ABCD,
//  the Y511-A is fully reversed. Getting this wrong does not fail loudly - it
//  returns a plausible-looking number.
float readValue() {
  uint16_t r[2];
  if (!readRegisters(ADDR, REG, 2, r)) return NAN;
  uint8_t b[4];
  if (DCBA) { b[0] = r[1] & 0xFF; b[1] = r[1] >> 8; b[2] = r[0] & 0xFF; b[3] = r[0] >> 8; }
  else      { b[3] = r[0] >> 8; b[2] = r[0] & 0xFF; b[1] = r[1] >> 8; b[0] = r[1] & 0xFF; }
  float f; memcpy(&f, b, 4);
  return f;
}


// ==========================================================================
void setup() {
  Serial.begin(38400);
  while (!Serial) { }
  Serial.println();
  Serial.println(F("=== SettleTime ==="));
  Serial.print(F("target   "));
#if TARGET_TURBIDITY
  Serial.println(F("Y511-A turbidity, 0x2602 @ 9600"));
#else
  Serial.println(F("SEN0680 dissolved oxygen, 0x0002 @ 4800"));
#endif
  Serial.print(F("run      ")); Serial.print(RUN_S);
  Serial.print(F(" s, one sample every ")); Serial.print(SAMPLE_MS / 1000);
  Serial.println(F(" s"));
  Serial.print(F("band     +/-")); Serial.print(TOLERANCE, 2);
  Serial.print(' '); printUnit(); Serial.print(F(" around the mean of the last "));
  Serial.print(REF_S); Serial.println(F(" s"));
  Serial.println();

  rs485.begin(BAUD);

  // Rails on, and the clock starts HERE - the answer has to include the
  // probe's own boot time, because the firmware pays for that too.
  pinMode(RAIL33_PIN, OUTPUT); digitalWrite(RAIL33_PIN, HIGH);
  pinMode(RAIL12_PIN, OUTPUT); digitalWrite(RAIL12_PIN, HIGH);
  unsigned long tRailOn = millis();
  Serial.println(F("rails ON - clock starts now"));
  Serial.println();
  Serial.println(F("   t/s     value    delta"));

  int16_t prev = BAD;
  for (uint8_t i = 0; i < NSAMP; i++) {
    while (millis() - tRailOn < (unsigned long)i * SAMPLE_MS) { }

    float v = readValue();
    //  A booting SEN0680 answers Modbus perfectly and returns 0.00 for the
    //  first several seconds. That is NOT a reading, and counting it as one
    //  made the flatness test below declare "settled at 4 s" on three
    //  identical zeros. Treated as not-ready instead.
    int16_t q = (isnan(v) || fabs(v) < 0.005) ? BAD : (int16_t)lround(v * 100.0);
    buf[nGot++] = q;

    Serial.print((millis() - tRailOn) / 1000.0, 1);
    Serial.print(F("  "));
    if (q == BAD) {
      Serial.println(F("   ----      not ready"));
    } else {
      Serial.print(q / 100.0, 2);
      if (prev != BAD) { Serial.print(F("   ")); Serial.print((q - prev) / 100.0, 2); }
      Serial.println();
      prev = q;
    }
  }

  digitalWrite(RAIL12_PIN, LOW);
  digitalWrite(RAIL33_PIN, LOW);
  Serial.println(F("rails OFF"));
  Serial.println();

  analyse();
}

void printUnit() {
  for (uint8_t i = 0; i < sizeof(UNIT) - 1; i++) Serial.print((char)pgm_read_byte(&UNIT[i]));
}


// ==========================================================================
//  The two questions, answered separately on purpose.
// ==========================================================================
void analyse() {
  const uint8_t refN = REF_S * 1000UL / SAMPLE_MS;
  if (nGot < refN + 2) { Serial.println(F("too few samples")); return; }

  // --- the final value: mean of the last REF_S seconds --------------------
  long sum = 0; uint8_t cnt = 0;
  for (uint8_t i = nGot - refN; i < nGot; i++)
    if (buf[i] != BAD) { sum += buf[i]; cnt++; }
  if (!cnt) { Serial.println(F("no valid samples at the end - run it again")); return; }
  const int16_t ref = sum / cnt;

  // how much does the "settled" reading still wobble? This is the number
  // TOLERANCE should have been chosen from.
  int16_t lo = 32767, hi = -32768;
  for (uint8_t i = nGot - refN; i < nGot; i++)
    if (buf[i] != BAD) { if (buf[i] < lo) lo = buf[i]; if (buf[i] > hi) hi = buf[i]; }

  Serial.println(F("--- result ---"));
  Serial.print(F("final value       ")); Serial.print(ref / 100.0, 2);
  Serial.print(' '); printUnit(); Serial.println();
  Serial.print(F("noise, last "));  Serial.print(REF_S);
  Serial.print(F(" s   +/-")); Serial.print((hi - lo) / 200.0, 2);
  Serial.print(' '); printUnit();
  if ((hi - lo) / 2 > (int16_t)(TOLERANCE * 100))
    Serial.print(F("   <-- WIDER THAN YOUR TOLERANCE, raise it"));
  Serial.println();

  // --- arrival: last time it was OUTSIDE the band, plus one sample --------
  const int16_t tol = (int16_t)lround(TOLERANCE * 100.0);
  int16_t lastOut = -1;
  for (uint8_t i = 0; i < nGot; i++)
    if (buf[i] == BAD || abs(buf[i] - ref) > tol) lastOut = i;

  Serial.print(F("within TOLERANCE  "));
  if (lastOut < 0) {
    Serial.println(F("0 s - it was inside the band from the first reading"));
  } else if (lastOut >= (int16_t)nGot - 1) {
    Serial.print(F("NOT within ")); Serial.print(RUN_S);
    Serial.println(F(" s - the run was too short, raise RUN_S"));
  } else {
    const uint16_t tset = (uint16_t)((lastOut + 1) * (SAMPLE_MS / 1000));
    Serial.print(tset); Serial.println(F(" s"));
  }

  // --- the number that should actually set the warm-up --------------------
  const int16_t acc = (int16_t)lround(ACCURACY * 100.0);
  int16_t lastFar = -1;
  for (uint8_t i = 0; i < nGot; i++)
    if (buf[i] == BAD || abs(buf[i] - ref) > acc) lastFar = i;
  Serial.print(F("within ACCURACY   "));
  if (lastFar >= (int16_t)nGot - 1) {
    Serial.println(F("never - something is wrong, this probe never converged"));
  } else {
    const uint16_t tacc = (uint16_t)((lastFar + 1) * (SAMPLE_MS / 1000));
    Serial.print(tacc); Serial.println(F(" s"));
    Serial.print(F("  -> DO_WARMUP_MS  "));
    Serial.print((unsigned long)tacc * 1000UL + 5000UL);
    Serial.println(F("   (that time plus 5 s of margin)"));
    Serial.println(F("  Use THIS one. Waiting for TOLERANCE instead chases drift"));
    Serial.println(F("  the sensor cannot resolve, and you pay for it 48x a day."));
  }

  // --- the naive criterion, for contrast ----------------------------------
  //  Flatness over a 3-sample window: the first moment it "stops moving".
  int16_t flat = -1;
  for (uint8_t i = 2; i < nGot; i++) {
    if (buf[i] == BAD || buf[i-1] == BAD || buf[i-2] == BAD) continue;
    int16_t a = buf[i], b = buf[i-1], c = buf[i-2];
    int16_t mn = min(a, min(b, c)), mx = max(a, max(b, c));
    if (mx - mn <= tol) { flat = i; break; }
  }
  Serial.print(F("naive 'stopped moving' at "));
  if (flat < 0) Serial.println(F("never"));
  else { Serial.print((uint16_t)(flat * (SAMPLE_MS / 1000))); Serial.println(F(" s")); }
  Serial.println(F("  If that is much earlier than SETTLED, the reading flattened"));
  Serial.println(F("  mid-ramp and then kept drifting. Trust SETTLED."));
  Serial.println();
  Serial.println(F("Run this twice. A settling time that is not repeatable is"));
  Serial.println(F("not a settling time - look for a loose probe or a sagging rail."));
}

void loop() { }
