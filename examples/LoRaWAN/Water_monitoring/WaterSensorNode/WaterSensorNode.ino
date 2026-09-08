/*
 * ============================================================================
 *  KijaniSpace Water Sensor Device  --  LoRaWAN water-quality node
 * ============================================================================
 *  Board : WaziSense v2  (ATmega328P + RFM95W), Arduino-programmable
 *
 *  Sensors
 *    RS485 / Modbus-RTU  (switched 12 V rail):
 *      - Dissolved oxygen   DFRobot SEN0680  (fluorescence, freshwater)  0x01
 *      - Turbidity          Yosemitech Y511-A (self-cleaning wiper)      0x03
 *    I2C  (switched 3.3 V; pull-ups are NOT on the board on this unit -
 *          I2C_J was found open, so 2 x 4.7 kOhm were fitted to X3-1):
 *      - pH                 Atlas EZO-pH   NO isolator fitted            0x63
 *      - Conductivity       Atlas EZO-EC   NO isolator fitted            0x64
 *
 *    The missing pH isolator is a deliberate, deferred decision, not an
 *    oversight: the EC probe drives AC through the same water and the pH cell
 *    reads it as signal. Read pH BEFORE EC (this sketch does) and decide by
 *    measurement whether the residual offset matters. See Drawing 09 in the
 *    wiring document.
 *    1-Wire (switched 3.3 V):
 *      - Temperature        DS18B20  <-- PRIMARY temperature source
 *
 *    Not fitted in this build: PT1000 3-wire RTD + MAX31865 on SPI.
 *    Set ENABLE_PT1000 to 1 and ENABLE_DS18B20 to 0 to go back to it; the
 *    PT1000 then takes channel 1 and the DS18B20 becomes a cross-check on
 *    channel 7. Nothing else has to change.
 *
 *  ---------------------------------------------------------------------------
 *  TWO SWITCHED RAILS  --  both via the WaziSense on-board high-side MOSFETs.
 *  The H1 jumper must be in the **3.3 V** position; it applies to BOTH
 *  "Sensor Power" blocks, so neither can carry battery voltage.
 *
 *    D6 -> "Sensor Power 1" -> switched 3.3 V
 *          feeds: RS485 transceiver, DS18B20 + its pull-up, the two
 *                 I2C pull-ups, RS485 fail-safe bias, both EZO circuits
 *    D7 -> "Sensor Power 2" -> switched 3.3 V used as a CONTROL SIGNAL
 *          into the opto-isolated high-side module that switches the 12 V rail
 *
 *  Both rails switch the POSITIVE line only; ground is continuous, so the
 *  RS485 common reference is never broken.
 *  ---------------------------------------------------------------------------
 *  WaziSense v2 PIN RESERVATIONS (from the WaziSense v2 Internal User Manual)
 *    D0/D1      USB serial (FTDI port)
 *    D2         RFM95W DIO0             <-- do not use
 *    D8         on-board LED (LED1)
 *    D9         RFM95W RESET            <-- do not use
 *    D10        RFM95W NSS
 *    D11/12/13  SPI MOSI/MISO/SCK       <-- radio only in this build
 *    A0         battery monitor (2x 470 kOhm divider, "BATT%" jumper)
 *    A4/A5      I2C. The manual says 4.7 kOhm pull-ups are fitted via the
 *               "I2C_J" jumper - on THIS board that jumper was open (5.9 MOhm
 *               to 3V3), so own 4.7 kOhm resistors go to X3-1. MEASURE before
 *               assuming: without a pull-up, Wire hangs rather than failing.
 *    D6/D7      "Sensor Power" terminal blocks (high-side MOSFETs, H1 jumper)
 *
 *  Used by us: D3, D4, D6, D7, A2   |   Spare: D5, A1, A3
 *
 *  A1 was the MAX31865 chip select and is free again now that the PT1000 is
 *  not fitted. SPI belongs entirely to the radio in this configuration, so
 *  the shared-bus / SPI-mode problem does not exist here at all.
 *  ---------------------------------------------------------------------------
 *
 *  ####################################################################
 *  ## Verify the MODBUS SENSOR MAP below against each sensor's own    ##
 *  ## datasheet. Register numbers, data format and scaling differ per ##
 *  ## vendor and firmware revision. The values here are PLACEHOLDERS. ##
 *  ####################################################################
 * ============================================================================
 */

#include <WaziDev.h>
#include <xlpp.h>
#include <LowPower.h>
#include <Vcc.h>
#include <Wire.h>
#include <SPI.h>
#include <SoftwareSerial.h>

// ---------------------------------------------------------------------------
//  Staged bring-up switches - test one sensor at a time first
// ---------------------------------------------------------------------------
// STAGE 1 of the bring-up. Connected: DS18B20, and both EZO circuits with no
// probes on them. Not connected: the two RS485 probes.
//
// The EZO circuits stay ENABLED on purpose. Without a probe their readings are
// meaningless, but the point of this stage is the bus, not the water: it proves
// the I2C wiring, both addresses, and that each circuit really was switched
// from UART to I2C. A circuit still in UART mode is silent on I2C and shows up
// here as FAIL.
//
// The RS485 sensors are switched OFF because an enabled-but-absent Modbus
// sensor does not fail fast: each read burns MODBUS_TIMEOUT_MS x
// MODBUS_RETRIES, and the turbidity wiper wait alone is 30 s.
#define ENABLE_PT1000     0     // MAX31865 + 3-wire PT1000 (SPI) - not fitted
#define ENABLE_DO         1     // RS485 0x01 - SEN0680, registers verified
#define ENABLE_TURBIDITY  0     // RS485  - probe not connected yet
// Both circuits were switched from UART to I2C on 2026-09-07 and verified
// with I2CScan: 0x63 and 0x64 both ACK, and SDA's driven-low residual fell
// from 2177 mV to 12 mV - an I2C pin is open-drain and never drives high,
// which is the objective proof the switch took (the LED colour was not).
//
// If either ever goes back to UART mode it will drive its TX pin onto a bus
// line and hang Wire outright on this AVR core, with the log simply ending.
// EZOSwitch/ re-does the switch; I2CScan/ tells you which state you are in.
#define ENABLE_PH         1     // I2C 0x63 - verified
#define ENABLE_EC         1     // I2C 0x64 - verified
#define ENABLE_DS18B20    1     // 1-Wire - connected, with its 4.7k pull-up

// The 12 V rail normally comes up only because an RS485 sensor needs it. Set
// this to 1 to raise it anyway - it lets you meter the boost + opto-switch
// chain end to end at X2-1 with nothing connected to it. Safe only once the
// XL6009 trimpot is actually set to 12 V; leave it at 0 otherwise.
#define RAIL12_FORCE_ON   1     // trimpot verified at 12 V

#if ENABLE_PT1000
  #include <Adafruit_MAX31865.h>
#endif

#if ENABLE_DS18B20
  #include <OneWire.h>
  #include <DallasTemperature.h>
#endif

// ---------------------------------------------------------------------------
//  LoRaWAN (ABP) - replace with the keys of THIS node
// ---------------------------------------------------------------------------
WaziDev wazidev;

unsigned char LoRaWANKeys[16] = {0x23, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF,
                                 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25};
unsigned char devAddr[4]      = {0x26, 0x01, 0x1B, 0xEE};

const int interval  = 3000;   // downlink RX window (ms)
int       sleep_sec = 900;    // 15 min base cycle; downlink-settable

// Turbidity is the slowest and hungriest reading (~30 s wiper sweep). The
// Requirements table asks for turbidity every 60 min on a 15 min base cycle,
// so it is read every 4th cycle. Set to 1 to read it every time.
const uint8_t TURBIDITY_EVERY_N_CYCLES = 4;
uint8_t       cycleCount = 0;

// ---------------------------------------------------------------------------
//  Pin map
// ---------------------------------------------------------------------------
const int RS485_RX_PIN = 3;    // transceiver module RXD / RO
const int RS485_TX_PIN = 4;    // transceiver module TXD / DI
const int RAIL33_EN    = 6;    // "Sensor Power 1" -> switched 3.3 V
const int RAIL12_EN    = 7;    // "Sensor Power 2" -> control of the 12 V module
const int ledPin       = 8;    // on-board LED1
const int batt_pin     = A0;   // on-board 470k/470k divider
#if ENABLE_PT1000
const int MAX31865_CS  = A1;   // analog pin driven as a digital output
#endif
#if ENABLE_DS18B20
const int ONE_WIRE_PIN = A2;   // needs an external 4.7 kOhm pull-up to X3-1
#endif

// NOTE: no DE/RE pin. The transceiver module is an AUTO-DIRECTION type with
// only RXD / TXD / VCC / GND, so it decides the bus direction itself.
SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

#if ENABLE_PT1000
Adafruit_MAX31865 rtd = Adafruit_MAX31865(MAX31865_CS);
#endif

#if ENABLE_DS18B20
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);
#endif

#if ENABLE_PT1000
// ---------------------------------------------------------------------------
//  MAX31865 / PT1000
//  RREF must match the reference resistor actually fitted on the breakout.
//  Boards sold for PT100 carry 430 Ohm - for PT1000 it must be 4300 Ohm
//  (Adafruit #3648, resistor marked 4301; #3328 is the PT100 board).
// ---------------------------------------------------------------------------
#define RREF        4300.0
#define RNOMINAL    1000.0
#endif

// ---------------------------------------------------------------------------
//  MODBUS SENSOR MAP   <<<<<  VERIFY AGAINST THE DATASHEETS  >>>>>
//    FMT_FLOAT_ABCD : IEEE-754 float, 2 registers, high word first
//    FMT_FLOAT_CDAB : IEEE-754 float, 2 registers, word-swapped (common)
//    FMT_INT_SCALED : single 16-bit register * SCALE
//  If a value is garbage but the CRC passes, the register is probably right
//  and the WORD ORDER is wrong -> try the other float format.
// ---------------------------------------------------------------------------
#define FMT_FLOAT_ABCD   0
#define FMT_FLOAT_CDAB   1
#define FMT_INT_SCALED   2

// 4800 is the SEN0680's FACTORY DEFAULT, not a choice - DFRobot ships it at
// 4800 8N1. The whole bus therefore runs at 4800 for now, which is fine: it
// only makes the timing analysis behind the single 120 Ohm terminator more
// forgiving (208 us per bit instead of 104).
//
// When the Y511-A arrives, one of the two has to move so both match. Either
// write 9600 into the SEN0680's register 0x07D1, or set the Y511-A to 4800 -
// check its factory default first and change whichever is the odd one out.
const uint32_t RS485_BAUD        = 4800;
const uint16_t MODBUS_TIMEOUT_MS = 1000;
const uint8_t  MODBUS_RETRIES    = 3;

// Auto-direction modules usually tie RE permanently low, which means the
// receiver stays on while you transmit and you read your own frame back.
// Both behaviours exist, so this has to be established once on the bench:
//   - set to 1, and if every read times out, set it to 0 (and vice versa).
// A wrong setting looks like "no response" or a stream of CRC errors.
#define RS485_ECHOES_OWN_TX   1

// --- DFRobot SEN0680 dissolved oxygen -------------------------------------
//  VERIFIED against wiki.dfrobot.com/sen0680 - no longer placeholders.
//
//  Supply      DC 10-30 V, 0.2 W (~17 mA at 12 V). 12 V is well inside range.
//  Cable       brown = V+, black = GND, yellow = 485-A, blue = 485-B. 5 m.
//  Serial      4800 8N1, factory address 0x01
//  Registers   0x0000-1  dissolved oxygen SATURATION, %
//              0x0002-3  dissolved oxygen CONCENTRATION, mg/L   <-- we want this
//              0x0004-5  temperature, degC
//              0x1020    salinity compensation
//              0x1022    atmospheric pressure compensation
//              0x07D0    device address     0x07D1  baud rate
//  Format      IEEE-754 float, BIG ENDIAN -> FMT_FLOAT_ABCD
//  Range       0-20 mg/L, accuracy +/-3 % FS
//
//  Reading 0x0000 instead of 0x0002 would have looked plausible and been
//  wrong: saturation in % and concentration in mg/L are both small numbers.
const uint8_t  DO_ADDR           = 0x01;
const uint16_t DO_REG            = 0x0002;   // concentration in mg/L
const uint8_t  DO_FMT            = FMT_FLOAT_ABCD;
const float    DO_SCALE          = 1.0;      // already mg/L

//  ####################################################################
//  ##  ATMOSPHERIC PRESSURE COMPENSATION - register 0x1022           ##
//  ##                                                                ##
//  ##  Dissolved-oxygen saturation follows the partial pressure of   ##
//  ##  oxygen, so it depends on air pressure. Lake Victoria sits at  ##
//  ##  about 1135 m, where pressure is roughly 88 kPa instead of the ##
//  ##  101 kPa at sea level - about 13 % lower. Left at a sea-level  ##
//  ##  default, every DO reading is biased by far more than the      ##
//  ##  sensor's own +/-3 % FS accuracy.                              ##
//  ##                                                                ##
//  ##  Write the local pressure to 0x1022 before deployment. Look up ##
//  ##  the unit and scaling in DFRobot's protocol document first -   ##
//  ##  it is not stated on the specification page, so do NOT guess.  ##
//  ##  Salinity (0x1020) stays at 0 for fresh water.                 ##
//  ####################################################################

//  Datasheet response time is "<= 60 s", which describes tracking a step
//  change in DO - not how long the optics need after a cold power-up, which
//  DFRobot does not state. 30 s is a deliberately generous starting point.
//  Determine it properly on the bench: power the rail and log the reading
//  every 2 s until it stops moving, then set this just above that. Too short
//  and every uplink carries a value that is still settling; too long and you
//  are paying for awake time you do not need.
const uint16_t DO_WARMUP_MS      = 30000;

// --- Yosemitech Y511-A turbidity (self-cleaning) ---
//  STILL PLACEHOLDERS - not yet verified against the Y511-A manual. Check the
//  address, baud rate, register numbers and word order before enabling it,
//  exactly as was done for the SEN0680 above. Its factory baud rate also
//  decides whether it or the SEN0680 has to be reconfigured to match.
const uint8_t  TURB_ADDR         = 0x03;
const uint16_t TURB_REG          = 0x2600;
const uint8_t  TURB_FMT          = FMT_FLOAT_CDAB;
const float    TURB_SCALE        = 1.0;      // NTU
const uint8_t  TURB_NEEDS_START  = 1;
const uint16_t TURB_START_REG    = 0x2500;
const uint16_t TURB_START_VALUE  = 0x0001;
const uint16_t TURB_WARMUP_MS    = 30000;    // wiper sweep + optics settle

// Rail settling times.
//   3.3 V: the EZO circuits need roughly a second to boot after power-up.
//   12 V : boost start-up plus charging the 1000 uF bulk capacitor.
const uint16_t RAIL33_SETTLE_MS  = 1200;
const uint16_t RAIL12_SETTLE_MS  = 500;

// ---------------------------------------------------------------------------
//  Atlas Scientific EZO (I2C)
//  NOTE: EZO-EC outputs "EC,TDS,SAL,SG" as CSV by default; atof() takes the
//  first field (EC in uS/cm). Cleaner: disable the extras once with
//  "O,TDS,0" / "O,S,0" / "O,SG,0".
// ---------------------------------------------------------------------------
const uint8_t  EZO_PH_ADDR       = 0x63;
const uint8_t  EZO_EC_ADDR       = 0x64;
const uint16_t EZO_READ_DELAY_MS = 900;
const uint16_t EZO_CMD_DELAY_MS  = 300;

// ---------------------------------------------------------------------------
//  Battery measurement
// ---------------------------------------------------------------------------
//const float VccCorrection = 3.85 / 7.5;   // 4.2 V Li-ion
const float VccCorrection = 5.0 / 2.6;      // 5 V supply
Vcc vcc(VccCorrection);

// ---------------------------------------------------------------------------
//  Readings  (NAN = read failed; that channel is omitted from the uplink)
// ---------------------------------------------------------------------------
float v_temp  = NAN;   // water temperature, degC (PT1000 via MAX31865)
float v_do    = NAN;   // dissolved oxygen, mg/L
float v_turb  = NAN;   // turbidity, NTU
float v_ph    = NAN;   // pH
float v_ec    = NAN;   // conductivity, uS/cm
float v_temp2 = NAN;   // second temperature, only used if BOTH are fitted
float v_batt  = NAN;   // battery, V

XLPP xlpp(64);

// ---------------------------------------------------------------------------
//  LED / sleep helpers
// ---------------------------------------------------------------------------
const int totalBlinks  = 20;
const int initialDelay = 100;
const int finalDelay   = 10;

void blink_led() {
  for (int i = 0; i < totalBlinks; i++) {
    digitalWrite(ledPin, HIGH);
    delay(map(i, 0, totalBlinks - 1, initialDelay, finalDelay));
    digitalWrite(ledPin, LOW);
    delay(map(i, 0, totalBlinks - 1, initialDelay, finalDelay));
  }
}

void sleep(int sec_to_sleep) {
  Serial.print(F("Will sleep now for approximately "));
  Serial.print(sec_to_sleep);
  Serial.println(F(" seconds."));
  delay(1000);

  for (int i = 0; i < sec_to_sleep / 8; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  delay(1000);
  Serial.println(F("--------------------------------------------------"));
  Serial.print(sec_to_sleep);
  Serial.println(F(" seconds have passed. Performing task..."));
}

// ---------------------------------------------------------------------------
//  Rail control.  3.3 V comes up first (the transceiver and the RTD front-end
//  live on it), then the 12 V rail for the probes. Powering down in reverse.
// ---------------------------------------------------------------------------
void rails33On() {
  Serial.println(F("3.3 V sensor rail ON  (D6)"));
  digitalWrite(RAIL33_EN, HIGH);
  delay(RAIL33_SETTLE_MS);        // EZO boot time
}

void rails12On() {
  Serial.println(F("12 V sensor rail ON   (D7)"));
  digitalWrite(RAIL12_EN, HIGH);
  delay(RAIL12_SETTLE_MS);        // boost start-up + bulk cap charge
}

void railsOff() {
  digitalWrite(RAIL12_EN, LOW);
  digitalWrite(RAIL33_EN, LOW);
  Serial.println(F("Both sensor rails OFF"));
}

// ===========================================================================
//  MODBUS-RTU MASTER  (function 0x03 read, 0x06 write)
//  Auto-direction transceiver: nothing to toggle, we just write and listen.
// ===========================================================================
uint16_t modbusCRC(const uint8_t *buf, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= (uint16_t)buf[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) { crc >>= 1; crc ^= 0xA001; }
      else              { crc >>= 1; }
    }
  }
  return crc;
}

void rs485Flush() {
  while (rs485.available()) rs485.read();
}

// Appends the CRC and transmits. Returns the total number of bytes put on the
// wire, which is also how many bytes come back if the module echoes.
uint8_t rs485Send(uint8_t *frame, uint8_t len) {
  uint16_t crc = modbusCRC(frame, len);
  frame[len]     = crc & 0xFF;          // CRC low byte first
  frame[len + 1] = (crc >> 8) & 0xFF;

  rs485Flush();
  rs485.write(frame, len + 2);
  rs485.flush();                        // wait for the last bit to leave
  delay(2);                             // let the module release the bus
  return len + 2;
}

// Swallows our own transmitted frame when the module loops it back.
void rs485DiscardEcho(uint8_t nBytes) {
#if RS485_ECHOES_OWN_TX
  uint8_t got = 0;
  unsigned long t0 = millis();
  while (got < nBytes && (millis() - t0) < 200) {
    if (rs485.available()) { rs485.read(); got++; }
  }
#else
  (void)nBytes;
#endif
}

bool modbusReadRegisters(uint8_t slave, uint16_t reg, uint8_t count, uint16_t *out) {
  for (uint8_t attempt = 0; attempt < MODBUS_RETRIES; attempt++) {

    uint8_t req[8];
    req[0] = slave;
    req[1] = 0x03;
    req[2] = (reg >> 8) & 0xFF;
    req[3] = reg & 0xFF;
    req[4] = 0x00;
    req[5] = count;
    uint8_t sent = rs485Send(req, 6);
    rs485DiscardEcho(sent);

    const uint8_t expected = 5 + count * 2;
    uint8_t resp[32];
    if (expected > sizeof(resp)) return false;

    uint8_t got = 0;
    unsigned long t0 = millis();
    while (got < expected && (millis() - t0) < MODBUS_TIMEOUT_MS) {
      if (rs485.available()) resp[got++] = rs485.read();
    }

    if (got < expected) {
      Serial.print(F("  Modbus timeout (slave 0x"));
      Serial.print(slave, HEX);
      Serial.println(F(")  -- check RS485_ECHOES_OWN_TX"));
      continue;
    }
    if (resp[0] != slave || resp[1] != 0x03) {
      Serial.println(F("  Modbus bad header -- check RS485_ECHOES_OWN_TX"));
      continue;
    }
    uint16_t crcCalc = modbusCRC(resp, expected - 2);
    uint16_t crcRecv = (uint16_t)resp[expected - 1] << 8 | resp[expected - 2];
    if (crcCalc != crcRecv) {
      Serial.println(F("  Modbus CRC error"));
      continue;
    }

    for (uint8_t i = 0; i < count; i++) {
      out[i] = ((uint16_t)resp[3 + i * 2] << 8) | resp[4 + i * 2];
    }
    return true;
  }
  return false;
}

bool modbusWriteRegister(uint8_t slave, uint16_t reg, uint16_t value) {
  uint8_t req[8];
  req[0] = slave;
  req[1] = 0x06;
  req[2] = (reg >> 8) & 0xFF;
  req[3] = reg & 0xFF;
  req[4] = (value >> 8) & 0xFF;
  req[5] = value & 0xFF;
  uint8_t sent = rs485Send(req, 6);
  rs485DiscardEcho(sent);

  // A normal reply is an echo of the request; we only drain it.
  unsigned long t0 = millis();
  uint8_t got = 0;
  while (got < 8 && (millis() - t0) < MODBUS_TIMEOUT_MS) {
    if (rs485.available()) { rs485.read(); got++; }
  }
  return (got == 8);
}

float modbusReadValue(uint8_t slave, uint16_t reg, uint8_t fmt, float scale) {
  uint16_t r[2];

  if (fmt == FMT_INT_SCALED) {
    if (!modbusReadRegisters(slave, reg, 1, r)) return NAN;
    return (float)((int16_t)r[0]) * scale;
  }

  if (!modbusReadRegisters(slave, reg, 2, r)) return NAN;

  uint32_t raw;
  if (fmt == FMT_FLOAT_ABCD) raw = ((uint32_t)r[0] << 16) | r[1];
  else                       raw = ((uint32_t)r[1] << 16) | r[0];   // CDAB

  float f;
  memcpy(&f, &raw, 4);
  if (isnan(f) || isinf(f)) return NAN;
  return f * scale;
}

// ===========================================================================
//  I2C BUS STATE GUARD
//
//  Both lines idle HIGH on a healthy bus. If either is held low, the TWI
//  hardware will wait for it forever, so check before every transfer and
//  refuse rather than hang.
//
//  IMPORTANT - what this does NOT protect against: a device that drives the
//  bus INTERMITTENTLY. An EZO circuit left in UART mode streams a reading
//  about once a second, so between bursts both lines look idle, the check
//  passes, and Wire can still hang mid-transfer. The only real fix for that
//  is to switch the circuit to I2C mode. Treat a "bus stuck" line in the log
//  as proof of a wiring or protocol fault, not as something to work around.
// ===========================================================================
bool i2cBusIdle() {
  // A4 = SDA, A5 = SCL. digitalRead works even while TWI owns the pins.
  return (digitalRead(A4) == HIGH) && (digitalRead(A5) == HIGH);
}

// Standard recovery: clock SCL nine times so a slave stuck mid-byte can finish
// and release SDA, then issue a STOP condition by hand.
void i2cBusRecover() {
  TWCR = 0;                             // hand the pins back from the TWI unit

  pinMode(A4, INPUT);                   // SDA - the on-board pull-up holds it
  pinMode(A5, OUTPUT);                  // SCL - we drive the clock ourselves
  for (uint8_t i = 0; i < 9; i++) {
    digitalWrite(A5, LOW);  delayMicroseconds(5);
    digitalWrite(A5, HIGH); delayMicroseconds(5);
  }

  pinMode(A4, OUTPUT);                  // STOP: SDA low -> high while SCL high
  digitalWrite(A4, LOW);  delayMicroseconds(5);
  digitalWrite(A5, HIGH); delayMicroseconds(5);
  pinMode(A4, INPUT);     delayMicroseconds(5);
  pinMode(A5, INPUT);

  Wire.begin();                         // re-arm the TWI unit
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif
}

// ===========================================================================
//  ATLAS SCIENTIFIC EZO  (I2C, ASCII command set)
// ===========================================================================
bool ezoCommand(uint8_t addr, const char *cmd, char *resp, uint8_t respSize, uint16_t waitMs) {
  if (!i2cBusIdle()) {
    Serial.print(F("  I2C bus stuck (SDA/SCL held low) before 0x"));
    Serial.print(addr, HEX);
    Serial.println(F(" - attempting recovery"));
    i2cBusRecover();
    if (!i2cBusIdle()) {
      Serial.println(F("  recovery failed - is an EZO still in UART mode?"));
      if (resp && respSize) resp[0] = 0;
      return false;
    }
  }

  Wire.beginTransmission(addr);
  Wire.write(cmd);
  if (Wire.endTransmission() != 0) {
    if (resp && respSize) resp[0] = 0;
    return false;
  }

  delay(waitMs);
  if (!resp || respSize == 0) return true;

  Wire.requestFrom(addr, (uint8_t)(respSize + 1));
  if (!Wire.available()) { resp[0] = 0; return false; }

  uint8_t code = Wire.read();      // 1=ok 2=fail 254=pending 255=no data
  uint8_t i = 0;
  while (Wire.available()) {
    char c = Wire.read();
    if (c == 0) break;
    if (i < respSize - 1) resp[i++] = c;
  }
  resp[i] = 0;
  while (Wire.available()) Wire.read();

  return (code == 1);
}

void ezoSetTemperature(uint8_t addr, float tempC) {
  if (isnan(tempC)) return;
  char cmd[16], tbuf[8];
  dtostrf(tempC, 4, 2, tbuf);
  snprintf(cmd, sizeof(cmd), "T,%s", tbuf);
  ezoCommand(addr, cmd, NULL, 0, EZO_CMD_DELAY_MS);
}

float ezoRead(uint8_t addr) {
  char resp[24];
  if (!ezoCommand(addr, "R", resp, sizeof(resp), EZO_READ_DELAY_MS)) return NAN;
  if (resp[0] == 0) return NAN;
  return atof(resp);     // stops at the first comma -> EC field for EZO-EC
}

// ===========================================================================
//  SENSOR READ FUNCTIONS
// ===========================================================================
float readVolts() {
  int j = 0;
  float vcc_reg = 0;
  for (j = 0; j < 100; j++) { vcc_reg += vcc.Read_Volts(); delay(5); }
  vcc_reg /= j;

  int i = 0;
  float last_vcc = 0;
  for (i = 0; i < 100; i++) {
    last_vcc += ((analogRead(batt_pin) * (vcc_reg / 1023.0)) * 3.83);
    delay(5);
  }
  last_vcc /= i;

  Serial.print(F("Battery: "));
  Serial.print(last_vcc, 2);
  Serial.println(F(" V"));
  return last_vcc;
}

#if ENABLE_PT1000
// The MAX31865 sits on the SWITCHED 3.3 V rail, so it loses its configuration
// every cycle - begin() has to be called again after each power-up, not once
// in setup().
//
// The RFM95W runs SPI mode 0, the MAX31865 mode 1, on the same three wires.
// Adafruit's driver wraps its access in an SPI transaction so the two coexist,
// but if temperatures come back as nonsense WHILE LoRa still works, a stuck
// SPI mode is the first thing to suspect.
float readPT1000() {
  rtd.begin(MAX31865_3WIRE);            // 2x red + 1x white PT1000

  float t = rtd.temperature(RNOMINAL, RREF);

  uint8_t fault = rtd.readFault();
  if (fault) {
    Serial.print(F("  MAX31865 fault 0x"));
    Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) Serial.println(F("  RTD high / open"));
    if (fault & MAX31865_FAULT_LOWTHRESH)  Serial.println(F("  RTD low / shorted"));
    if (fault & MAX31865_FAULT_REFINLOW)   Serial.println(F("  REFIN- < 0.85*Vbias"));
    if (fault & MAX31865_FAULT_RTDINLOW)   Serial.println(F("  RTDIN- < 0.85*Vbias"));
    if (fault & MAX31865_FAULT_OVUV)       Serial.println(F("  over/under voltage"));
    rtd.clearFault();
    return NAN;
  }
  if (t < -50 || t > 100) return NAN;   // sanity band for lake water
  return t;
}
#endif

void readAllSensors(bool doTurbidity) {
  v_temp = v_do = v_turb = v_ph = v_ec = v_temp2 = NAN;

  rails33On();      // transceiver, DS18B20 + pull-up, RS485 bias, EZO circuits

#if (ENABLE_DO || ENABLE_TURBIDITY || RAIL12_FORCE_ON)
  rails12On();      // the two RS485 probes (or a bench check of the rail)
#else
  Serial.println(F("12 V rail left OFF - nothing needs it"));
#endif

  // --- kick off turbidity first: its wiper is by far the slowest step ---
#if ENABLE_TURBIDITY
  unsigned long turbStart = 0;
  if (doTurbidity && TURB_NEEDS_START) {
    Serial.println(F("Turbidity: start measurement + wiper"));
    modbusWriteRegister(TURB_ADDR, TURB_START_REG, TURB_START_VALUE);
    turbStart = millis();
  }
#endif

  // --- temperature first: it compensates pH and EC ---
#if ENABLE_PT1000
  v_temp = readPT1000();
  Serial.print(F("Temperature (PT1000/MAX31865): "));
  if (isnan(v_temp)) Serial.println(F("FAIL"));
  else { Serial.print(v_temp, 2); Serial.println(F(" degC")); }
#endif

#if ENABLE_DS18B20
  // Power-gated like everything else on X3, so the bus is re-enumerated each
  // cycle. requestTemperatures() blocks for the 750 ms 12-bit conversion.
  ds18b20.begin();
  ds18b20.requestTemperatures();
  float t2 = ds18b20.getTempCByIndex(0);

  // The sanity band deliberately excludes two specific failure values:
  //   -127.0  DEVICE_DISCONNECTED_C - nothing answered on the bus
  //    +85.0  the DS18B20's power-on reset value, which is what you read if
  //           the conversion has not finished or the supply is marginal.
  // Neither is a plausible lake-water temperature, so one band catches both.
  if (t2 > -20.0 && t2 < 60.0) v_temp2 = t2;

  Serial.print(F("Temperature (DS18B20): "));
  if (isnan(v_temp2)) Serial.println(F("FAIL (check the 4.7k pull-up to X3-1)"));
  else { Serial.print(v_temp2, 2); Serial.println(F(" degC")); }

  if (isnan(v_temp)) {
    // No PT1000 fitted (or it failed): the DS18B20 IS the water temperature,
    // so it belongs on channel 1. Clearing v_temp2 keeps us from sending the
    // same number twice - airtime at SF12 is too expensive for that.
    v_temp  = v_temp2;
    v_temp2 = NAN;
  }
#endif

  // --- dissolved oxygen ---
#if ENABLE_DO
  Serial.println(F("DO: warming up optics..."));
  delay(DO_WARMUP_MS);
  v_do = modbusReadValue(DO_ADDR, DO_REG, DO_FMT, DO_SCALE);
  Serial.print(F("Dissolved oxygen: "));
  if (isnan(v_do)) Serial.println(F("FAIL"));
  else { Serial.print(v_do, 2); Serial.println(F(" mg/L")); }
#endif

  // --- pH and EC, temperature-compensated ---
#if ENABLE_PH
  ezoSetTemperature(EZO_PH_ADDR, v_temp);
  v_ph = ezoRead(EZO_PH_ADDR);
  Serial.print(F("pH: "));
  if (isnan(v_ph)) Serial.println(F("FAIL")); else Serial.println(v_ph, 2);
#endif

#if ENABLE_EC
  ezoSetTemperature(EZO_EC_ADDR, v_temp);
  v_ec = ezoRead(EZO_EC_ADDR);
  Serial.print(F("Conductivity: "));
  if (isnan(v_ec)) Serial.println(F("FAIL"));
  else { Serial.print(v_ec, 1); Serial.println(F(" uS/cm")); }
#endif

  // --- turbidity: read once the wiper sweep has finished ---
#if ENABLE_TURBIDITY
  if (doTurbidity) {
    if (TURB_NEEDS_START) {
      long remaining = (long)TURB_WARMUP_MS - (long)(millis() - turbStart);
      if (remaining > 0) {
        Serial.print(F("Turbidity: waiting "));
        Serial.print(remaining / 1000);
        Serial.println(F(" s for wiper/optics"));
        delay(remaining);
      }
    }
    v_turb = modbusReadValue(TURB_ADDR, TURB_REG, TURB_FMT, TURB_SCALE);
    Serial.print(F("Turbidity: "));
    if (isnan(v_turb)) Serial.println(F("FAIL"));
    else { Serial.print(v_turb, 2); Serial.println(F(" NTU")); }
  } else {
    Serial.println(F("Turbidity: skipped this cycle (energy saving)"));
  }
#endif

  // Cutting power replaces the EZO "Sleep" command - nothing on either rail
  // draws current between measurements, including the RS485 fail-safe bias.
  railsOff();
}

// ===========================================================================
//  LORAWAN UPLINK
//
//  XLPP / Cayenne-LPP channel map:
//    ch 1  Temperature      addTemperature   0.1 degC
//    ch 2  Battery          addVoltage
//    ch 3  pH               addAnalogInput   direct (0..14)
//    ch 4  Dissolved oxygen addAnalogInput   direct mg/L (0..20)
//    ch 5  Conductivity     addAnalogInput   *** sent in mS/cm ***
//    ch 6  Turbidity        addAnalogInput   *** sent as NTU/10 ***
//    ch 7  2nd temperature  addTemperature   only sent when BOTH a PT1000
//                                             and a DS18B20 are fitted, as a
//                                             cross-check. Absent in this build.
//
//  !! LPP_ANALOG_INPUT is a signed 2-byte value with 0.01 resolution, so its
//  !! range is only +/-327.67. EC in uS/cm and turbidity up to 1000 NTU would
//  !! OVERFLOW - hence the scaling. The WaziCloud decoder must undo it:
//  !!    EC_uS = ch5 * 1000 ;  NTU = ch6 * 10
//  Channels whose read failed (or that were skipped) are simply omitted.
//  Channel 1 always carries the water temperature, whichever sensor produced
//  it - so swapping PT1000 for DS18B20 does not change the decoder.
// ===========================================================================
uint8_t uplink() {
  xlpp.reset();

  if (!isnan(v_temp))  xlpp.addTemperature(1, v_temp);

  v_batt = readVolts();
  if (!isnan(v_batt))  xlpp.addVoltage(2, v_batt);

  if (!isnan(v_ph))    xlpp.addAnalogInput(3, v_ph);
  if (!isnan(v_do))    xlpp.addAnalogInput(4, v_do);
  if (!isnan(v_ec))    xlpp.addAnalogInput(5, v_ec / 1000.0);   // uS/cm -> mS/cm
  if (!isnan(v_turb))  xlpp.addAnalogInput(6, v_turb / 10.0);   // NTU  -> NTU/10
  if (!isnan(v_temp2)) xlpp.addTemperature(7, v_temp2);

  serialPrintf(("LoRaWAN send ... "));
  delay(3000);
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);
  if (e != 0) {
    serialPrintf(("Err %d\n"), e);
    delay(interval);
    return e;
  }
  Serial.println(F("OK\n"));
  return 0;
}

// ===========================================================================
//  LORAWAN DOWNLINK
//    ch 0 : new sampling interval in SECONDS (60..3600)
// ===========================================================================
uint8_t downlink_with_logs(uint16_t timeout) {
  uint8_t e;

  Serial.print(F("Waiting for RX1, for a time of(in ms): "));
  Serial.println(timeout);

  long startSend = millis();
  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, timeout);
  long endSend = millis();

  if (e == ERR_LORA_TIMEOUT) {
    Serial.println(F("RX1 Timeout. Waiting for RX2..."));
    delay(2000);
    e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, timeout);
    if (e == ERR_LORA_TIMEOUT) {
      Serial.println(F("RX2 Timeout. No downlink received."));
      return ERR_LORA_TIMEOUT;
    }
    Serial.println(F("Downlink received in RX2."));
  } else {
    Serial.println(F("Downlink received in RX1."));
  }

  serialPrintf("Time On Air: %d ms\n", endSend - startSend);
  serialPrintf("LoRa SNR: %d\n", wazidev.loRaSNR);
  serialPrintf("LoRa RSSI: %d\n", wazidev.loRaRSSI);
  serialPrintf("LoRaWAN frame size: %d\n", xlpp.offset + xlpp.len);
  serialPrintf("LoRaWAN payload len: %d\n", xlpp.len);
  serialPrintf("Payload: ");

  if (xlpp.len == 0) {
    Serial.println(F("(no payload received)"));
    return 1;
  }

  printBase64(xlpp.getBuffer(), xlpp.len);
  Serial.println();

  int end = xlpp.len + xlpp.offset;
  while (xlpp.offset < end) {
    uint8_t chan = xlpp.getChannel();
    serialPrintf("Chan %2d: ", chan);
    uint8_t type = xlpp.getType();
    serialPrintf("Type %2d: ", type);

    switch (chan) {
      case 0:
        switch (type) {
          case LPP_ANALOG_OUTPUT:
          case LPP_ANALOG_INPUT: {
            float newInterval = xlpp.getAnalogOutput();
            if (newInterval >= 60 && newInterval <= 3600) {
              sleep_sec = (int)newInterval;
              Serial.print(F("Sampling interval changed to "));
              Serial.print(sleep_sec);
              Serial.println(F(" s"));
            } else {
              Serial.println(F("Interval out of range (60..3600 s), ignored."));
            }
            break;
          }
          default:
            Serial.println(F("Other unknown type."));
            return 1;
        }
        break;

      default:
        Serial.println(F("Unknown channel."));
        return 1;
    }
  }
  return 0;
}

// ===========================================================================
//  SETUP / LOOP
// ===========================================================================
void setup() {
  Serial.begin(38400);

  pinMode(ledPin, OUTPUT);

  pinMode(RAIL33_EN, OUTPUT);
  pinMode(RAIL12_EN, OUTPUT);
  digitalWrite(RAIL33_EN, LOW);           // both rails OFF at boot
  digitalWrite(RAIL12_EN, LOW);

#if ENABLE_PT1000
  pinMode(MAX31865_CS, OUTPUT);
  digitalWrite(MAX31865_CS, HIGH);        // deselect before the radio inits
#endif

  rs485.begin(RS485_BAUD);

  Wire.begin();                           // A4/A5, pull-ups are on-board

  // The AVR Wire library has NO timeout before core 1.8.1: if a device holds
  // SDA or SCL low, endTransmission() blocks in a hardware wait loop and the
  // sketch stops dead - no message, no reset, the log just ends. Use the
  // native timeout where the core provides it; i2cBusIdle() below is the
  // fallback, and it is a net, not a cure (see its comment).
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000 /* us */, true /* reset the bus on timeout */);
  Serial.println(F(" I2C timeout guard: native (25 ms)"));
#else
  Serial.println(F(" I2C timeout guard: pre-flight check only (old AVR core)"));
#endif

  blink_led();

  Serial.println(F("=================================================="));
  Serial.println(F(" KijaniSpace water sensor node  --  WaziSense v2"));
  Serial.println(F("=================================================="));
  Serial.println(F(" H1 jumper must be in the 3.3 V position."));

#if ENABLE_PT1000
  // SPI is shared: the MAX31865 is initialised per cycle in readPT1000(),
  // because its rail is switched off in between.
#else
  Serial.println(F(" Temperature: DS18B20 on A2 (PT1000 not fitted)."));
  Serial.println(F(" Needs a 4.7 kOhm pull-up from A2 to X3-1."));
#endif
  wazidev.setupLoRaWAN(devAddr, LoRaWANKeys);
  sx1272.setSF(SF_12);                    // gateway accepts SF12 only
}

void loop(void) {
  uint8_t e;

  bool doTurbidity = (TURBIDITY_EVERY_N_CYCLES <= 1) ||
                     (cycleCount % TURBIDITY_EVERY_N_CYCLES == 0);

  // 1. read the sensors (both rails are switched on/off inside)
  readAllSensors(doTurbidity);

  // 2. LoRaWAN uplink
  e = uplink();

  if (!e) {
    // 3. LoRaWAN downlink
    downlink_with_logs(interval);
    cycleCount++;
    sleep(sleep_sec);
  } else {
    Serial.print(F("Error: "));
    Serial.println(e);
    sleep(60);                            // short retry backoff
  }
}
