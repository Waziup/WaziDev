/*
 * ============================================================================
 *  RS485Config  --  change one Modbus device's address (or baud), safely
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud (read-only, nothing to type)
 *
 *  Both probes ship on slave address 0x01, so they cannot share a bus until
 *  one of them moves. This writes that change - and refuses to do it unless
 *  exactly one device is present, because a write goes to whichever devices
 *  are listening. Address two devices at once and you cannot address either
 *  of them afterwards.
 *
 *  ############################################################
 *  ##  ONE DEVICE ON THE BUS. Disconnect the other probe's   ##
 *  ##  A/B *and* its power before running this.              ##
 *  ############################################################
 *
 *  WHAT IT DOES
 *    1. sweeps addresses 0x01..0x10 at AIM_BAUD and counts who answers
 *    2. refuses unless exactly one device answered
 *    3. performs the enabled operation with function code 0x06
 *    4. re-finds the device across every baud rate and address, and reports
 *       where it actually ended up
 *
 *  Step 4 is the important one. A write that lands somewhere unexpected is
 *  recoverable as long as you can find the device again, so the tool always
 *  looks rather than assuming the write did what you asked.
 * ============================================================================
 */

#include <SoftwareSerial.h>

// ---------------------------------------------------------------------------
//  WHO ARE WE TALKING TO
// ---------------------------------------------------------------------------
#define AIM_BAUD        9600      // Y511-A: 9600.  SEN0680: 4800.
#define VERIFY_REG      0x2600    // Y511-A values. SEN0680: 0x0002.

// ---------------------------------------------------------------------------
//  WHAT TO DO.  Enable exactly ONE, run it, then set it back to 0.
//  Both are 0 by default so an accidental upload changes nothing.
// ---------------------------------------------------------------------------
#define OP_SET_ADDRESS  0
#define OP_SET_BAUD     0

// -- Address change. The value is simply the new address, so this is safe:
//    Y511-A slave-address register is 0x3000 (EnviroDIY library).
//    SEN0680 slave-address register is 0x07D0 (DFRobot wiki).
#define ADDR_REGISTER   0x3000
#define NEW_ADDRESS     0x03

// -- Baud change. READ THIS BEFORE ENABLING IT.
//    The SEN0680 has a baud register at 0x07D1, but DFRobot does not document
//    what value selects which rate - it may be the literal number, or an
//    index into their list. A wrong value can land the device on 57600 or
//    115200, and SoftwareSerial on a 16 MHz AVR cannot reliably reach those,
//    so the device would be out of reach of this tool. Prefer avoiding the
//    baud change altogether: if the Y511-A also answers at 4800, keep the
//    whole bus at 4800 and only move an address.
#define BAUD_REGISTER   0x07D1
#define BAUD_VALUE      9600      // try the literal first; then 2, then 3...

// ---------------------------------------------------------------------------
const int RS485_RX_PIN = 3;
const int RS485_TX_PIN = 4;
const int RAIL33_EN    = 6;
const int RAIL12_EN    = 7;
const int ledPin       = 8;

const uint32_t BAUDS[] = {4800, 9600, 19200, 2400, 38400};
const uint8_t  N_BAUDS = sizeof(BAUDS) / sizeof(BAUDS[0]);

const uint16_t LISTEN_MS = 200;

SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

uint8_t rx[32];
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

void sendFrame(uint8_t *f, uint8_t len) {
  uint16_t crc = modbusCRC(f, len);
  f[len]     = crc & 0xFF;
  f[len + 1] = (crc >> 8) & 0xFF;

  while (rs485.available()) rs485.read();
  rs485.write(f, len + 2);
  rs485.flush();

  rxLen = 0;
  uint32_t t0 = millis();
  while ((millis() - t0) < LISTEN_MS) {
    while (rs485.available()) {
      if (rxLen < sizeof(rx)) rx[rxLen++] = rs485.read();
      t0 = millis();
    }
  }
}

// Function 0x03 read of two registers. True if a CRC-valid reply came back.
bool readsOk(uint8_t addr, uint16_t reg) {
  uint8_t f[8] = {addr, 0x03, (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), 0x00, 0x02, 0, 0};
  sendFrame(f, 6);
  if (rxLen < 9) return false;
  if (rx[0] != addr || rx[1] != 0x03) return false;
  uint16_t calc = modbusCRC(rx, 7);
  uint16_t recv = ((uint16_t)rx[8] << 8) | rx[7];
  return calc == recv;
}

// Function 0x06 write of a single register.
bool writeReg(uint8_t addr, uint16_t reg, uint16_t val) {
  uint8_t f[8] = {addr, 0x06, (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF),
                  (uint8_t)(val >> 8), (uint8_t)(val & 0xFF), 0, 0};
  Serial.print(F("  writing 0x"));
  Serial.print(val, HEX);
  Serial.print(F(" to register 0x"));
  Serial.print(reg, HEX);
  Serial.print(F(" at address 0x"));
  Serial.println(addr, HEX);

  sendFrame(f, 6);

  Serial.print(F("  reply: "));
  if (rxLen == 0) {
    Serial.println(F("none - many devices go quiet after this, which is"));
    Serial.println(F("         normal. The re-scan below is what counts."));
    return true;
  }
  for (uint8_t i = 0; i < rxLen; i++) {
    if (rx[i] < 0x10) Serial.print('0');
    Serial.print(rx[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
  if (rx[1] & 0x80) {
    Serial.print(F("  MODBUS EXCEPTION 0x"));
    Serial.println(rx[2], HEX);
    Serial.println(F("  The device refused the write - wrong register, or"));
    Serial.println(F("  it is read-only on this model."));
    return false;
  }
  return true;
}

// Find the device again, wherever it went.
void reScan() {
  Serial.println(F("--------------------------------------------------"));
  Serial.println(F("RE-SCAN  every baud rate, addresses 0x01..0x10"));

  uint8_t hits = 0;
  for (uint8_t b = 0; b < N_BAUDS; b++) {
    rs485.end();
    rs485.begin(BAUDS[b]);
    delay(50);

    for (uint8_t a = 1; a <= 16; a++) {
      if (!readsOk(a, VERIFY_REG)) continue;
      hits++;
      Serial.print(F("  FOUND at "));
      Serial.print(BAUDS[b]);
      Serial.print(F(" baud, address 0x"));
      if (a < 0x10) Serial.print('0');
      Serial.println(a, HEX);
    }
  }

  if (hits == 0) {
    Serial.println(F("  NOT FOUND anywhere."));
    Serial.println(F("  If you changed the baud rate, it may have landed on"));
    Serial.println(F("  57600 or 115200, which SoftwareSerial cannot reach on"));
    Serial.println(F("  a 16 MHz AVR. A USB-RS485 adapter on a PC can still"));
    Serial.println(F("  talk to it and set it back."));
  }
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(RAIL33_EN, OUTPUT);
  pinMode(RAIL12_EN, OUTPUT);
  digitalWrite(RAIL33_EN, HIGH);
  delay(1200);
  digitalWrite(RAIL12_EN, HIGH);
  delay(2000);

  rs485.begin(AIM_BAUD);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" RS485 device configuration"));
  Serial.println(F("=================================================="));
  Serial.print(F(" "));
  Serial.print(AIM_BAUD);
  Serial.print(F(" baud, verifying on register 0x"));
  Serial.println(VERIFY_REG, HEX);

#if !OP_SET_ADDRESS && !OP_SET_BAUD
  Serial.println(F(" No operation enabled - this run only reports who is on"));
  Serial.println(F(" the bus. Set OP_SET_ADDRESS or OP_SET_BAUD to 1 to act."));
#endif
  Serial.println(F("--------------------------------------------------"));

  // ---- interlock: exactly one device, and we must know its address -------
  Serial.println(F("Who is on the bus?"));
  uint8_t found = 0, foundAt = 0;
  for (uint8_t a = 1; a <= 16; a++) {
    if (!readsOk(a, VERIFY_REG)) continue;
    found++;
    foundAt = a;
    Serial.print(F("  answers at address 0x"));
    if (a < 0x10) Serial.print('0');
    Serial.println(a, HEX);
  }

  Serial.println(F("--------------------------------------------------"));

  if (found == 0) {
    Serial.println(F("NOTHING ANSWERED. Check AIM_BAUD and VERIFY_REG, the"));
    Serial.println(F("12 V rail under load, and that the probe is wired."));
  } else if (found > 1) {
    Serial.print(F("REFUSED: "));
    Serial.print(found);
    Serial.println(F(" devices are on the bus."));
    Serial.println(F("A write goes to whoever is listening. Two devices given"));
    Serial.println(F("the same address cannot be separated again. Disconnect"));
    Serial.println(F("one probe's A/B and power, then run this once more."));
  } else {
#if OP_SET_ADDRESS
    Serial.println(F("OPERATION: set slave address"));
    if (writeReg(foundAt, ADDR_REGISTER, NEW_ADDRESS)) {
      delay(1500);
      reScan();
      Serial.println(F("Expected: the device now answers at the new address"));
      Serial.println(F("and no longer at the old one."));
    }
#elif OP_SET_BAUD
    Serial.println(F("OPERATION: set baud rate"));
    Serial.println(F("The value encoding is undocumented. If the re-scan"));
    Serial.println(F("finds it at an unexpected rate, that tells you the"));
    Serial.println(F("encoding - note it down before trying another value."));
    if (writeReg(foundAt, BAUD_REGISTER, BAUD_VALUE)) {
      delay(1500);
      reScan();
    }
#else
    Serial.println(F("Exactly one device. Nothing to do - no operation is"));
    Serial.println(F("enabled, which is the safe default."));
#endif
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
