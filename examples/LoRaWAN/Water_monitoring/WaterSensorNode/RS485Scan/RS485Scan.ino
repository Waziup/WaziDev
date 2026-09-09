/*
 * ============================================================================
 *  RS485Scan  --  find out what the Modbus device is actually doing
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud (read-only, nothing to type)
 *
 *  "Modbus timeout" tells you nothing about WHY. This sketch dumps the raw
 *  bytes instead, which separates the three failure modes that all look the
 *  same from the main sketch:
 *
 *    nothing arrives at all      -> wiring, A/B polarity, power, or baud rate
 *    exactly our own frame back  -> the module echoes; RS485_ECHOES_OWN_TX = 1
 *    a different frame arrives   -> the device answers; check address/format
 *
 *  It sweeps the baud rates the SEN0680 supports, and at any baud that
 *  produces bytes it sweeps addresses 1..16. For every reply it prints the
 *  raw hex, checks the CRC, and decodes the payload BOTH ways - big endian
 *  and word-swapped - so you can see which interpretation is plausible
 *  instead of guessing at FMT_FLOAT_ABCD vs FMT_FLOAT_CDAB.
 *
 *  SEN0680 reference (wiki.dfrobot.com/sen0680): 4800 8N1 default, address
 *  0x01, register 0x0002-3 = dissolved oxygen in mg/L as a big-endian float.
 *  Supply DC 10-30 V; brown = V+, black = GND, yellow = 485-A, blue = 485-B.
 * ============================================================================
 */

#include <SoftwareSerial.h>

const int RS485_RX_PIN = 3;     // transceiver RXD / RO
const int RS485_TX_PIN = 4;     // transceiver TXD / DI
const int RAIL33_EN    = 6;     // "Sensor Power 1" -> switched 3.3 V
const int RAIL12_EN    = 7;     // "Sensor Power 2" -> control of the 12 V rail
const int ledPin       = 8;

const uint16_t RAIL33_SETTLE_MS = 1200;
const uint16_t RAIL12_SETTLE_MS = 1000;   // just the boost and the bulk cap

// How long to keep asking at the factory default before giving up on it.
// DFRobot documents a "<= 60 s" response time, which is about tracking a
// change in DO - not how long the device needs from power-up until it will
// answer Modbus at all, which is not documented anywhere. So measure it: this
// asks once a second and reports the elapsed time at the first valid reply.
// That number is what DO_WARMUP_MS in the main sketch should be based on.
const uint16_t BOOT_WAIT_MS  = 45000;
const uint16_t BOOT_POLL_MS  = 1000;

// ####################################################################
// ##  THE BATTERY MUST BE CONNECTED.                                ##
// ##  USB/FTDI does not power the battery domain: X1-1 then sits at  ##
// ##  under 3 V, the boost converter is below its own minimum input  ##
// ##  and simply passes that through instead of regulating. The rail ##
// ##  reads about 7 V, the SEN0680 needs 10-30 V, and every baud     ##
// ##  rate looks equally silent. Measured 2.836 V on this build.     ##
// ####################################################################

// The device only has to be able to TALK here, not to have settled its
// optics, so this is much shorter than DO_WARMUP_MS in the main sketch.
uint16_t LISTEN_MS = 400;                 // how long to wait for a reply
// (not const: the register sweep lowers it to SWEEP_LISTEN_MS and restores it)

const uint32_t BAUDS[] = {4800, 9600, 19200, 2400, 38400};
const uint8_t  N_BAUDS = sizeof(BAUDS) / sizeof(BAUDS[0]);

// ---------------------------------------------------------------------------
//  AIM THE SCAN.  Change these three, nothing else, to point at a device.
//
//    SEN0680 (verified) :  4800, 0x01, register 0x0002
//    Y511-A  (factory)  :  9600, 0x01, register unknown -> use SWEEP_REGISTERS
// ---------------------------------------------------------------------------
#define AIM_BAUD        4800     // Y511-A: fixed 9600 8N1, 4800 for SEN0680
#define AIM_ADDR        0x01
uint16_t PROBE_REG   = 0x2600;            // Y511-A values; sweep moves it
const uint8_t  PROBE_COUNT = 2;           // two registers = one float

// A device whose register map you do not have will answer on some registers
// and stay silent (or return an exception) on all the others. Sweeping two
// narrow windows finds the live ones in well under a minute. Set to 0 to skip.
#define SWEEP_REGISTERS 1
// Yosemitech register areas, from the EnviroDIY library - not guesses:
//   0x2600  values + temperature (5 registers)   0x2500  start measurement
//   0x3000  slave address                        0x3100  activate brush
//   0x1400  serial number, 14 ASCII characters
// Sweeping 0x2600 pins down which pair is turbidity and which is temperature;
// sweeping 0x1400 returns readable ASCII, which confirms the device identity.
// NOTE: Yosemitech floats are LITTLE-endian, so the plausible column here is
// CDAB - the opposite of the SEN0680.
const uint16_t SWEEP_FROM[] = {0x2600, 0x1400};   // window starts
const uint16_t SWEEP_LEN    = 0x0A;               // 10 registers per window
const uint8_t  N_WINDOWS    = sizeof(SWEEP_FROM) / sizeof(SWEEP_FROM[0]);

// A device that answers at all answers fast. 150 ms is plenty at 4800 baud
// for a 9-byte reply, and it keeps a 40-register sweep down to a few seconds.
const uint16_t SWEEP_LISTEN_MS = 150;

SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

uint8_t sent[8];
uint8_t sentLen = 0;
uint8_t rx[64];
uint8_t rxLen = 0;

uint16_t modbusCRC(const uint8_t *buf, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= (uint16_t)buf[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1) { crc >>= 1; crc ^= 0xA001; } else { crc >>= 1; }
    }
  }
  return crc;
}

void hexDump(const uint8_t *b, uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    if (b[i] < 0x10) Serial.print('0');
    Serial.print(b[i], HEX);
    Serial.print(' ');
  }
}

// Sends a function-0x03 read and captures everything that comes back.
void probe(uint8_t slave) {
  sent[0] = slave;
  sent[1] = 0x03;
  sent[2] = (PROBE_REG >> 8) & 0xFF;
  sent[3] = PROBE_REG & 0xFF;
  sent[4] = 0x00;
  sent[5] = PROBE_COUNT;
  uint16_t crc = modbusCRC(sent, 6);
  sent[6] = crc & 0xFF;
  sent[7] = (crc >> 8) & 0xFF;
  sentLen = 8;

  while (rs485.available()) rs485.read();     // clear stale bytes

  rs485.write(sent, sentLen);
  rs485.flush();

  rxLen = 0;
  uint32_t t0 = millis();
  while ((millis() - t0) < LISTEN_MS) {
    while (rs485.available()) {
      if (rxLen < sizeof(rx)) rx[rxLen++] = rs485.read();
      t0 = millis();                          // extend while bytes still flow
    }
  }
}

// All FOUR byte permutations, not two. Showing only ABCD and CDAB once hid a
// Yosemitech reading completely: its floats arrive fully byte-reversed, so
// 10 5C CA 41 is 41 CA 5C 10 = 25.29 - which neither of the two columns
// produced. "Little-endian" in a vendor datasheet can mean word-swapped OR
// fully reversed, so print them all and let the plausible number speak.
void showAs(const __FlashStringHelper *name, uint32_t raw) {
  float f;
  memcpy(&f, &raw, 4);
  Serial.print(F("      as "));
  Serial.print(name);
  Serial.print(F(": "));
  if (isnan(f) || isinf(f)) Serial.println(F("not a number"));
  else Serial.println(f, 4);
}

void decodeFloat(const uint8_t *d) {
  showAs(F("ABCD (big endian)  "),
         ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) |
         ((uint32_t)d[2] << 8)  |  (uint32_t)d[3]);
  showAs(F("CDAB (word swap)   "),
         ((uint32_t)d[2] << 24) | ((uint32_t)d[3] << 16) |
         ((uint32_t)d[0] << 8)  |  (uint32_t)d[1]);
  showAs(F("DCBA (all reversed)"),
         ((uint32_t)d[3] << 24) | ((uint32_t)d[2] << 16) |
         ((uint32_t)d[1] << 8)  |  (uint32_t)d[0]);
  showAs(F("BADC (byte swap)   "),
         ((uint32_t)d[1] << 24) | ((uint32_t)d[0] << 16) |
         ((uint32_t)d[3] << 8)  |  (uint32_t)d[2]);
}

// Returns true if this looks like a real device reply.
bool report(uint8_t slave) {
  if (rxLen == 0) return false;

  Serial.print(F("    addr 0x"));
  if (slave < 0x10) Serial.print('0');
  Serial.print(slave, HEX);
  Serial.print(F("  got "));
  Serial.print(rxLen);
  Serial.print(F(" bytes: "));
  hexDump(rx, rxLen);
  Serial.println();

  // Is the leading part our own frame handed straight back?
  bool echo = (rxLen >= sentLen) && (memcmp(rx, sent, sentLen) == 0);
  uint8_t off = 0;
  if (echo) {
    Serial.println(F("      -> the first 8 bytes are OUR OWN frame."));
    Serial.println(F("         The module echoes: RS485_ECHOES_OWN_TX = 1"));
    off = sentLen;
  } else {
    Serial.println(F("      -> no echo of our frame: RS485_ECHOES_OWN_TX = 0"));
  }

  uint8_t n = rxLen - off;
  if (n == 0) {
    Serial.println(F("         ...but nothing followed it. The device itself"));
    Serial.println(F("         did not answer - address or baud still wrong."));
    return false;
  }

  const uint8_t *r = rx + off;
  const uint8_t expected = 5 + PROBE_COUNT * 2;

  if (n < expected) {
    Serial.print(F("         reply too short ("));
    Serial.print(n);
    Serial.print(F(" of "));
    Serial.print(expected);
    Serial.println(F(") - truncated frame"));
    return false;
  }
  if (r[0] != slave || r[1] != 0x03) {
    Serial.println(F("         header does not match the request"));
    return false;
  }

  uint16_t calc = modbusCRC(r, expected - 2);
  uint16_t recv = ((uint16_t)r[expected - 1] << 8) | r[expected - 2];
  if (calc != recv) {
    Serial.println(F("         CRC MISMATCH - baud rate or noise"));
    return false;
  }

  Serial.println(F("         CRC ok - this is a valid Modbus reply."));
  decodeFloat(r + 3);
  return true;
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(RAIL33_EN, OUTPUT);
  pinMode(RAIL12_EN, OUTPUT);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" RS485 / Modbus raw scan v2"));
  Serial.println(F("=================================================="));

  digitalWrite(RAIL33_EN, HIGH);
  Serial.println(F(" 3.3 V rail ON (transceiver, bias)"));
  delay(RAIL33_SETTLE_MS);

  digitalWrite(RAIL12_EN, HIGH);
  Serial.print(F(" 12 V rail ON (probe) - waiting "));
  Serial.print(RAIL12_SETTLE_MS);
  Serial.println(F(" ms for it to boot"));
  delay(RAIL12_SETTLE_MS);

  Serial.println(F(" Measure the rail at X2-1 UNDER LOAD now - it must be"));
  Serial.println(F(" 10-30 V for the SEN0680. Around 7 V means the boost is"));
  Serial.println(F(" starved: is the battery connected?"));
  Serial.println(F("--------------------------------------------------"));

  bool found = false;

  // ---- Phase A: be patient at the documented default, and time the boot ---
  Serial.print(F("PHASE A  "));
  Serial.print(AIM_BAUD);
  Serial.print(F(" baud, address 0x"));
  Serial.print(AIM_ADDR, HEX);
  Serial.println(F(", asking once a second"));
  rs485.begin(AIM_BAUD);
  delay(50);

  uint32_t t0 = millis();
  while ((millis() - t0) < BOOT_WAIT_MS) {
    probe(AIM_ADDR);
    if (rxLen > 0) {
      uint32_t elapsed = millis() - t0;
      Serial.print(F("  first bytes after "));
      Serial.print(elapsed / 1000.0, 1);
      Serial.println(F(" s from rail-on:"));
      if (report(AIM_ADDR)) {
        found = true;
        Serial.println();
        Serial.print(F("  >>> DEVICE READY "));
        Serial.print(elapsed / 1000.0, 1);
        Serial.println(F(" s after the 12 V rail came up."));
        Serial.println(F("  >>> Set DO_WARMUP_MS comfortably above that -"));
        Serial.println(F("  >>> answering is not the same as a settled reading,"));
        Serial.println(F("  >>> so leave margin for the optics on top."));
        break;
      }
    }
    Serial.print('.');
    delay(BOOT_POLL_MS);
  }
  Serial.println();

  if (!found) {
    Serial.println(F("  nothing at the default in 45 s - sweeping everything"));
    Serial.println();
  }

#if SWEEP_REGISTERS
  // ---- Register sweep: for a device whose map we do not have -------------
  if (found) {
    Serial.println();
    Serial.println(F("REGISTER SWEEP  which registers actually return data"));
    uint16_t saveReg  = PROBE_REG;
    uint16_t saveWait = LISTEN_MS;
    LISTEN_MS = SWEEP_LISTEN_MS;

    for (uint8_t w = 0; w < N_WINDOWS; w++) {
      Serial.print(F("  window 0x"));
      Serial.print(SWEEP_FROM[w], HEX);
      Serial.print(F(" .. 0x"));
      Serial.println(SWEEP_FROM[w] + SWEEP_LEN - 1, HEX);

      for (uint16_t r = 0; r < SWEEP_LEN; r++) {
        PROBE_REG = SWEEP_FROM[w] + r;
        probe(AIM_ADDR);

        if (rxLen == 0) continue;

        // Skip the echo if this module loops our frame back.
        uint8_t off = (rxLen >= sentLen && memcmp(rx, sent, sentLen) == 0)
                      ? sentLen : 0;
        uint8_t n = rxLen - off;
        if (n < 5) continue;

        const uint8_t *q = rx + off;

        // A Modbus exception sets the top bit of the function code. That is
        // a real answer - it proves the device is alive - but not a value.
        if (q[1] & 0x80) {
          Serial.print(F("    0x"));
          Serial.print(PROBE_REG, HEX);
          Serial.print(F("  exception 0x"));
          Serial.println(q[2], HEX);
          continue;
        }

        const uint8_t expected = 5 + PROBE_COUNT * 2;
        if (n < expected) continue;
        uint16_t calc = modbusCRC(q, expected - 2);
        uint16_t recv = ((uint16_t)q[expected - 1] << 8) | q[expected - 2];
        if (calc != recv) continue;

        Serial.print(F("    0x"));
        Serial.print(PROBE_REG, HEX);
        Serial.print(F("  DATA: "));
        hexDump(q + 3, PROBE_COUNT * 2);
        Serial.println();
        decodeFloat(q + 3);
      }
    }

    PROBE_REG = saveReg;
    LISTEN_MS = saveWait;
    Serial.println(F("  sweep done. A register that returns a plausible number"));
    Serial.println(F("  in ONE word order and nonsense in the other has told"));
    Serial.println(F("  you both the register AND the format."));
    Serial.println();
  }
#endif

  // ---- Phase B: sweep the other baud rates and addresses -----------------
  for (uint8_t b = 0; b < N_BAUDS && !found; b++) {
    Serial.print(F("BAUD "));
    Serial.println(BAUDS[b]);

    rs485.end();
    rs485.begin(BAUDS[b]);
    delay(50);

    // Address 1 first - the SEN0680's factory default.
    probe(0x01);
    if (rxLen == 0) {
      Serial.println(F("    addr 0x01  silence"));
    } else if (report(0x01)) {
      found = true;
      break;
    }

    // Bytes appeared but did not parse: sweep the low addresses.
    if (rxLen > 0) {
      Serial.println(F("    something is on this baud - sweeping 0x02..0x10"));
      for (uint8_t a = 2; a <= 16; a++) {
        probe(a);
        if (rxLen > 0 && report(a)) { found = true; break; }
      }
    }
    Serial.println();
  }

  Serial.println(F("=================================================="));
  if (found) {
    Serial.println(F("DEVICE FOUND. Copy the baud, address, echo setting and"));
    Serial.println(F("the plausible float format into the main sketch."));
  } else {
    Serial.println(F("NOTHING ANSWERED ON ANY BAUD RATE."));
    Serial.println(F("Total silence means the problem is below the protocol:"));
    Serial.println(F("  1. SWAP yellow and blue. A/B reversed gives exactly"));
    Serial.println(F("     this, and cannot damage anything."));
    Serial.println(F("  2. Measure 12 V at the sensor's own brown/black wires,"));
    Serial.println(F("     not just at X2-1 - a terminal can look seated and"));
    Serial.println(F("     not be."));
    Serial.println(F("  3. Check the 120 Ohm terminator is across A-B, and the"));
    Serial.println(F("     two 680 Ohm bias resistors go A->+3V3 and B->GND."));
    Serial.println(F("  4. Confirm the transceiver has 3.3 V on its VCC."));
  }
  Serial.println(F("=================================================="));

  digitalWrite(RAIL12_EN, LOW);
  digitalWrite(RAIL33_EN, LOW);
  digitalWrite(ledPin, LOW);
}

void loop() {
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  delay(2900);
}
