/*  DOCompensation - what unit does the SEN0680 want for air pressure?
 *  ---------------------------------------------------------------------
 *  Register 0x1022 holds the atmospheric-pressure compensation. DFRobot
 *  does not publish its unit or scaling anywhere, and the node firmware has
 *  carried a "do NOT guess" warning about it for weeks.
 *
 *  THERE IS NO NEED TO GUESS. The register ships with a sea-level default,
 *  and that default states the unit by itself. The candidate encodings are
 *  orders of magnitude apart and cannot be confused:
 *
 *      101.33  as a float      ->  kPa
 *      1013    as an integer   ->  hPa / mbar
 *      10133                   ->  kPa in hundredths
 *      101325                  ->  Pa
 *
 *  This sketch reads the register and prints it as all of them at once. One
 *  line will be a recognisable air pressure. That line is the answer.
 *
 *  WHY IT MATTERS. Dissolved-oxygen saturation follows the partial pressure
 *  of oxygen. Lake Victoria sits near 1135 m, where air pressure is about
 *  88.4 kPa against 101.3 at sea level - 13 % low. The sensor's own accuracy
 *  is +/-3 % FS, so a sea-level default puts four times the sensor's error
 *  into every reading, in one direction, invisibly.
 *
 *  WIRING is the node's own. One probe on the bus is enough; the turbidity
 *  probe can stay connected, it runs at a different baud rate and will not
 *  answer at 4800.
 *
 *  SERIAL MONITOR AT 38400.
 */

#include <SoftwareSerial.h>

// ---- the site -----------------------------------------------------------
//  p = 101.325 kPa * (1 - 2.25577e-5 * h)^5.25588
//      0 m -> 101.33    500 m -> 95.46    1135 m -> 88.42
//   1500 m ->  84.56   2000 m -> 79.50
//  Weather moves this by about +/-2 kPa. The altitude term is 13 %, so the
//  altitude is worth correcting and the weather is not.
const uint16_t SITE_ALTITUDE_M   = 1135;     // Lake Victoria
const float    SITE_PRESSURE_KPA = 88.42;

// ---- writing -------------------------------------------------------------
//  Leave at 0 for the first run. Read the printout, work out the unit, put
//  the matching number in WRITE_RAW, then set this to 1 for ONE run.
#define DO_WRITE   0
const uint16_t WRITE_RAW = 8842;   // 88.42 kPa in hundredths - CHANGE THIS
                                   // to whatever the printout tells you

// ---- pins, as on the node ------------------------------------------------
const int RS485_RX_PIN = 3;
const int RS485_TX_PIN = 4;
const int RAIL12_PIN   = 5;
const int RAIL33_PIN   = 6;

SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

const uint8_t  DO_ADDR        = 0x01;
const uint32_t DO_BAUD        = 4800;
const uint16_t SALINITY_REG   = 0x1020;
const uint16_t PRESSURE_REG   = 0x1022;
const uint16_t TIMEOUT_MS     = 1000;
const uint8_t  RETRIES        = 3;


uint16_t modbusCRC(uint8_t *b, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= b[i];
    for (uint8_t k = 0; k < 8; k++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

bool readRegs(uint16_t reg, uint8_t count, uint16_t *out) {
  for (uint8_t a = 0; a < RETRIES; a++) {
    uint8_t req[8];
    req[0] = DO_ADDR; req[1] = 0x03;
    req[2] = reg >> 8; req[3] = reg & 0xFF;
    req[4] = 0x00;     req[5] = count;
    uint16_t c = modbusCRC(req, 6);
    req[6] = c & 0xFF; req[7] = c >> 8;

    while (rs485.available()) rs485.read();
    rs485.write(req, 8); rs485.flush(); delay(2);

    const uint8_t exp = 5 + count * 2;
    uint8_t r[16], got = 0;
    unsigned long t0 = millis();
    while (got < exp && (millis() - t0) < TIMEOUT_MS)
      if (rs485.available()) r[got++] = rs485.read();

    if (got < exp || r[0] != DO_ADDR || r[1] != 0x03) continue;
    if (modbusCRC(r, exp - 2) != ((uint16_t)r[exp-1] << 8 | r[exp-2])) continue;
    for (uint8_t i = 0; i < count; i++)
      out[i] = ((uint16_t)r[3 + i*2] << 8) | r[4 + i*2];
    return true;
  }
  return false;
}

bool writeReg(uint16_t reg, uint16_t val) {
  uint8_t req[8];
  req[0] = DO_ADDR; req[1] = 0x06;
  req[2] = reg >> 8; req[3] = reg & 0xFF;
  req[4] = val >> 8; req[5] = val & 0xFF;
  uint16_t c = modbusCRC(req, 6);
  req[6] = c & 0xFF; req[7] = c >> 8;

  while (rs485.available()) rs485.read();
  rs485.write(req, 8); rs485.flush();

  // A 0x06 write echoes the request back verbatim.
  uint8_t r[8], got = 0;
  unsigned long t0 = millis();
  while (got < 8 && (millis() - t0) < TIMEOUT_MS)
    if (rs485.available()) r[got++] = rs485.read();
  if (got < 8) return false;
  return r[0] == DO_ADDR && r[1] == 0x06 &&
         r[4] == (val >> 8) && r[5] == (val & 0xFF);
}


void dump(const __FlashStringHelper *name, uint16_t reg, uint8_t count) {
  uint16_t r[2] = {0, 0};
  Serial.println();
  Serial.print(name); Serial.print(F("  (0x")); Serial.print(reg, HEX);
  Serial.println(F(")"));

  if (!readRegs(reg, count, r)) {
    Serial.println(F("   no reply - check the 12 V rail and the baud rate"));
    return;
  }

  Serial.print(F("   raw words      ")); Serial.print(r[0]);
  if (count > 1) { Serial.print(' '); Serial.print(r[1]); }
  Serial.println();
  Serial.print(F("   as uint16      ")); Serial.println(r[0]);
  Serial.print(F("   as int16       ")); Serial.println((int16_t)r[0]);

  if (count > 1) {
    uint32_t raw = ((uint32_t)r[0] << 16) | r[1];
    Serial.print(F("   as uint32      ")); Serial.println(raw);
    float f; memcpy(&f, &raw, 4);
    Serial.print(F("   as float ABCD  "));
    if (isnan(f) || isinf(f)) Serial.println(F("not a number"));
    else                      Serial.println(f, 3);
  }
  Serial.print(F("   /10            ")); Serial.println(r[0] / 10.0, 2);
  Serial.print(F("   /100           ")); Serial.println(r[0] / 100.0, 2);
}


void setup() {
  Serial.begin(38400);
  while (!Serial) { }
  Serial.println();
  Serial.println(F("=== DOCompensation - SEN0680 registers 0x1020 / 0x1022 ==="));
  Serial.print(F("site  ")); Serial.print(SITE_ALTITUDE_M);
  Serial.print(F(" m  ->  ")); Serial.print(SITE_PRESSURE_KPA, 2);
  Serial.println(F(" kPa"));

  rs485.begin(DO_BAUD);
  pinMode(RAIL33_PIN, OUTPUT); digitalWrite(RAIL33_PIN, HIGH);
  pinMode(RAIL12_PIN, OUTPUT); digitalWrite(RAIL12_PIN, HIGH);
  Serial.println(F("rails ON, waiting 3 s for the probe to boot"));
  delay(3000);

  dump(F("salinity"),            SALINITY_REG, 1);
  dump(F("atmospheric pressure"), PRESSURE_REG, 2);

  Serial.println();
  Serial.println(F("--- how to read this ---"));
  Serial.println(F("One line above is a recognisable air pressure:"));
  Serial.println(F("   101.33 or 88.42       -> kPa"));
  Serial.println(F("   1013 or 884           -> hPa, or kPa in tenths"));
  Serial.println(F("   10133 or 8842         -> kPa in hundredths"));
  Serial.println(F("   101325                -> Pa"));
  Serial.println(F("Whichever it is, that is the unit the register wants."));
  Serial.println(F("Salinity should read 0 - leave it there for fresh water."));

#if DO_WRITE
  Serial.println();
  Serial.print(F("WRITING 0x1022 = ")); Serial.println(WRITE_RAW);
  if (writeReg(PRESSURE_REG, WRITE_RAW)) {
    Serial.println(F("write acknowledged - reading it back:"));
    delay(200);
    dump(F("atmospheric pressure, after write"), PRESSURE_REG, 2);
    Serial.println();
    Serial.println(F("If the read-back matches, set DO_WRITE back to 0 and"));
    Serial.println(F("put the same number in DO_PRESSURE_RAW in the node sketch."));
  } else {
    Serial.println(F("write FAILED - nothing was stored"));
  }
#else
  Serial.println();
  Serial.println(F("DO_WRITE is 0, so nothing was changed."));
#endif

  digitalWrite(RAIL12_PIN, LOW);
  digitalWrite(RAIL33_PIN, LOW);
  Serial.println(F("rails OFF"));
}

void loop() { }
