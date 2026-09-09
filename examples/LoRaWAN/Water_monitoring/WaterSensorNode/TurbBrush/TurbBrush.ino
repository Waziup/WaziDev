/*
 * ============================================================================
 *  TurbBrush  --  make the Y511-A wiper sweep on command, and read its
 *                 automatic interval
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud (read-only, nothing to type)
 *  ONE probe on the bus: the Y511-A. Disconnect the SEN0680's A/B and power.
 *
 *  WHY THE MAIN SKETCH'S BRUSH COMMAND DID NOTHING
 *  Yosemitech triggers several actions with frames that are not valid Modbus
 *  by the letter of the specification: the register-COUNT field is 0x0000.
 *  EnviroDIY's library sends exactly that, and it is the part that was missed.
 *
 *      activateBrush      fn 0x10   reg 0x3100   count 0x0000
 *      startMeasurement   fn 0x03   reg 0x2500   count 0x0000
 *      stopMeasurement    fn 0x03   reg 0x2E00   count 0x0001
 *
 *  A read of 0x3100 with count 0x0001, and a 0x06 write to it, are both
 *  different frames - which is why the wiper ignored them. This sketch sends
 *  the documented bytes verbatim, one command at a time, with a watching
 *  window after each so you can see which one actually moves the wiper.
 *
 *  IT ALSO ANSWERS THE INTERVAL QUESTION
 *  Register 0x3200 holds the automatic brush interval in minutes. Neither the
 *  library nor the specification sheet says what the factory default is, or
 *  even states plainly that the brush runs on a timer - but the register's
 *  existence settles that it does, and reading it tells you what THIS unit is
 *  set to, which is better evidence than a manual.
 *
 *  Remember what that interval is worth on this node: it counts only while
 *  the sensor is powered, and the node powers it for under a minute per hour.
 *  An interval of 60 minutes would therefore never once fire. The number is
 *  worth knowing, but the sweep still has to be commanded.
 * ============================================================================
 */

#include <SoftwareSerial.h>

const int RS485_RX_PIN = 3;
const int RS485_TX_PIN = 4;
const int RAIL33_EN    = 6;
const int RAIL12_EN    = 7;
const int ledPin       = 8;

const uint32_t TURB_BAUD = 9600;      // Y511-A, fixed
const uint8_t  TURB_ADDR = 0x01;      // factory

const uint16_t RAIL33_SETTLE_MS = 1200;
const uint16_t RAIL12_SETTLE_MS = 3000;

// Long enough to see a 10 s sweep start, run and finish.
const uint16_t WATCH_MS = 18000;

SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

uint8_t rx[40];
uint8_t rxLen = 0;

uint16_t modbusCRC(const uint8_t *b, uint8_t n) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < n; i++) {
    crc ^= (uint16_t)b[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1) { crc >>= 1; crc ^= 0xA001; } else { crc >>= 1; }
    }
  }
  return crc;
}

// Sends a payload verbatim with the CRC appended, and dumps whatever returns.
// No interpretation - these frames are not standard Modbus, so a "malformed
// reply" is not necessarily a failure.
void sendRaw(const uint8_t *payload, uint8_t len, const __FlashStringHelper *what) {
  uint8_t f[16];
  memcpy(f, payload, len);
  uint16_t crc = modbusCRC(f, len);
  f[len]     = crc & 0xFF;
  f[len + 1] = (crc >> 8) & 0xFF;

  Serial.print(F("  -> "));
  Serial.print(what);
  Serial.print(F("  ["));
  for (uint8_t i = 0; i < len + 2; i++) {
    if (f[i] < 0x10) Serial.print('0');
    Serial.print(f[i], HEX);
    if (i < len + 1) Serial.print(' ');
  }
  Serial.println(']');

  while (rs485.available()) rs485.read();
  rs485.write(f, len + 2);
  rs485.flush();

  rxLen = 0;
  uint32_t t0 = millis();
  while ((millis() - t0) < 400) {
    while (rs485.available()) {
      if (rxLen < sizeof(rx)) rx[rxLen++] = rs485.read();
      t0 = millis();
    }
  }

  Serial.print(F("  <- "));
  if (rxLen == 0) {
    Serial.println(F("(silence - normal for a command frame)"));
    return;
  }
  for (uint8_t i = 0; i < rxLen; i++) {
    if (rx[i] < 0x10) Serial.print('0');
    Serial.print(rx[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
}

void watch(const __FlashStringHelper *question) {
  Serial.print(F("     "));
  Serial.print(question);
  Serial.print(F("  ("));
  Serial.print(WATCH_MS / 1000);
  Serial.println(F(" s)"));
  Serial.flush();

  // Blink while watching, so it is obvious the sketch is in the window.
  uint32_t t0 = millis();
  while ((millis() - t0) < WATCH_MS) {
    digitalWrite(ledPin, ((millis() / 250) & 1) ? HIGH : LOW);
  }
  digitalWrite(ledPin, LOW);
}

// Standard read of one register.
//
// The two data bytes are assembled LOW BYTE FIRST. Modbus transmits a
// register high byte first, but Yosemitech puts its values in little-endian
// order inside the register - the same convention as its fully byte-reversed
// floats, and what EnviroDIY documents as "uint16, little-endian".
//
// Reading it the Modbus way turned 1E 00 into 0x1E00 = 7680 minutes, which is
// 5.3 days and obviously not a brush interval. Little-endian gives 0x001E =
// 30 minutes. Both hex bytes are printed so the raw value stays visible.
bool readOne(uint16_t reg, uint16_t *out) {
  uint8_t f[6] = {TURB_ADDR, 0x03, (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), 0x00, 0x01};
  sendRaw(f, 6, F("read one register"));
  if (rxLen < 7 || rx[0] != TURB_ADDR || rx[1] != 0x03) return false;
  uint16_t calc = modbusCRC(rx, 5);
  uint16_t recv = ((uint16_t)rx[6] << 8) | rx[5];
  if (calc != recv) return false;

  Serial.print(F("     raw bytes "));
  if (rx[3] < 0x10) Serial.print('0');
  Serial.print(rx[3], HEX);
  Serial.print(' ');
  if (rx[4] < 0x10) Serial.print('0');
  Serial.print(rx[4], HEX);
  Serial.print(F("   big-endian would be "));
  Serial.println(((uint16_t)rx[3] << 8) | rx[4]);

  *out = ((uint16_t)rx[4] << 8) | rx[3];   // little-endian, as the device sends
  return true;
}

// Two registers as a fully byte-reversed float - the Y511-A's format.
bool readFloatDCBA(uint16_t reg, float *out) {
  uint8_t f[6] = {TURB_ADDR, 0x03, (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), 0x00, 0x02};
  sendRaw(f, 6, F("read two registers"));
  if (rxLen < 9 || rx[0] != TURB_ADDR || rx[1] != 0x03) return false;
  uint16_t calc = modbusCRC(rx, 7);
  uint16_t recv = ((uint16_t)rx[8] << 8) | rx[7];
  if (calc != recv) return false;
  uint32_t raw = ((uint32_t)rx[6] << 24) | ((uint32_t)rx[5] << 16) |
                 ((uint32_t)rx[4] << 8)  |  (uint32_t)rx[3];
  memcpy(out, &raw, 4);
  return true;
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(RAIL33_EN, OUTPUT);
  pinMode(RAIL12_EN, OUTPUT);
  digitalWrite(RAIL33_EN, HIGH);
  delay(RAIL33_SETTLE_MS);
  digitalWrite(RAIL12_EN, HIGH);

  rs485.begin(TURB_BAUD);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" Y511-A wiper test"));
  Serial.println(F("=================================================="));
  Serial.println(F(" 9600 8N1, address 0x01. ONE probe on the bus."));
  Serial.println(F(" Rails stay ON for the whole run - watch the wiper."));
  Serial.print(F(" Waiting "));
  Serial.print(RAIL12_SETTLE_MS);
  Serial.println(F(" ms for the sensor to boot."));
  Serial.flush();
  delay(RAIL12_SETTLE_MS);
  digitalWrite(ledPin, LOW);

  // ---- the interval question, answered by the device itself --------------
  Serial.println(F("--------------------------------------------------"));
  Serial.println(F("A  automatic brush interval, register 0x3200"));
  uint16_t mins = 0;
  if (readOne(0x3200, &mins)) {
    Serial.print(F("     interval = "));
    Serial.print(mins);
    Serial.println(F(" minutes"));
    if (mins == 0) {
      Serial.println(F("     0 means the automatic sweep is disabled."));
    } else {
      Serial.println(F("     Note this only counts while the sensor is POWERED."));
      Serial.println(F("     The node powers it for under a minute per hour, so"));
      Serial.println(F("     this timer will never reach its interval in service."));
    }
  } else {
    Serial.println(F("     no valid reply - this model may not expose it"));
  }
}

void loop() {
  Serial.println(F("--------------------------------------------------"));
  Serial.println(F("B  activateBrush   fn 0x10  reg 0x3100  count 0x0000"));
  Serial.println(F("   This is the documented frame. A count field of zero is"));
  Serial.println(F("   not valid Modbus, which is exactly why a normal read or"));
  Serial.println(F("   write to this register did nothing."));
  {
    uint8_t f[7] = {TURB_ADDR, 0x10, 0x31, 0x00, 0x00, 0x00, 0x00};
    sendRaw(f, 7, F("activateBrush"));
  }
  watch(F("DOES THE WIPER MOVE NOW?"));

  Serial.println(F("--------------------------------------------------"));
  Serial.println(F("C  startMeasurement   fn 0x03  reg 0x2500  count 0x0000"));
  Serial.println(F("   Suspected to sweep as a side effect - wipe, then measure."));
  {
    uint8_t f[6] = {TURB_ADDR, 0x03, 0x25, 0x00, 0x00, 0x00};
    sendRaw(f, 6, F("startMeasurement"));
  }
  watch(F("DOES THE WIPER MOVE NOW?"));

  Serial.println(F("--------------------------------------------------"));
  Serial.println(F("D  read the values while it is measuring"));
  float t = 0, ntu = 0;
  if (readFloatDCBA(0x2600, &t)) {
    Serial.print(F("     0x2600 = "));
    Serial.print(t, 2);
    Serial.println(F("   (expected: temperature in degC)"));
  }
  if (readFloatDCBA(0x2602, &ntu)) {
    Serial.print(F("     0x2602 = "));
    Serial.print(ntu, 2);
    Serial.println(F("   (expected: turbidity in NTU)"));
  }
  Serial.println(F("     Stir milk or silt into the water: 0x2602 must jump"));
  Serial.println(F("     and 0x2600 must not. That settles which is which."));

  Serial.println();
  Serial.println(F("Repeating the B / C / D sequence."));
  Serial.flush();
  delay(5000);
}
