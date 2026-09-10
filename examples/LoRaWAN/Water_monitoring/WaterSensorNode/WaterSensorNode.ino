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
#define ENABLE_TURBIDITY  1     // RS485  - probe not connected yet
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
int       sleep_sec = 1800;   // 30 min base cycle; downlink-settable

// ---------------------------------------------------------------------------
//  ENERGY BUDGET, measured and calculated for the 30 min cycle
//
//    measurement window  ~50 s at ~90 mA   ->  1.25 mAh
//    LoRa transmit       2.4 s at ~120 mA  ->  0.08 mAh
//    sleep               1750 s            ->  1.94 mAh   <-- the big one
//                                              ---------
//    per cycle                                  3.3 mAh
//    48 cycles/day                            ~158 mAh/day
//
//  The sleep figure is NOT the MCU. LowPower.powerDown with ADC and BOD off
//  leaves the ATmega drawing microamps. It is the XL6009 boost converter,
//  which is not gated and therefore idles 24 h a day at roughly 3-5 mA. That
//  single component is about 59 % of the whole budget - more than every
//  measurement put together.
//
//  Software cannot fix it. The fix is a high-side switch in the boost's INPUT
//  (Pololu #2810, 2-20 V / 6 A) driven from D7, which also makes the
//  opto-isolated module on the output redundant: if the converter is off, the
//  12 V rail does not exist. That takes the budget to about 65 mAh/day.
//
//  What IS fixed in software below: three delays that cost awake time and buy
//  nothing (3 s before transmit, 200 samples for one battery reading, 2 s
//  around the sleep call).
// ---------------------------------------------------------------------------

// Turbidity is the slowest reading. The requirement is hourly, and the base
// cycle is now 30 min, so every 2nd cycle. Set to 1 to read it every time.
const uint8_t TURBIDITY_EVERY_N_CYCLES = 1;
uint8_t       cycleCount = 0;

// ---------------------------------------------------------------------------
//  Pin map
// ---------------------------------------------------------------------------
const int RS485_RX_PIN = 3;    // transceiver module RXD / RO
const int RS485_TX_PIN = 4;    // transceiver module TXD / DI
const int RAIL33_EN    = 6;    // "Sensor Power 1" -> switched 3.3 V
const int RAIL12_EN    = 7;    // "Sensor Power 2" -> control of the 12 V module
const int ledPin       = 8;    // on-board LED1
const int batt_pin     = A0;   // on-board 470k/470k divider, confirmed 2.00
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
//    FMT_FLOAT_CDAB : IEEE-754 float, 2 registers, word-swapped
//    FMT_FLOAT_DCBA : IEEE-754 float, 2 registers, ALL FOUR BYTES REVERSED
//    FMT_INT_SCALED : single 16-bit register * SCALE
//
//  If a value is garbage but the CRC passes, the register is probably right
//  and the byte order is wrong. Try all of them - and note that two devices
//  on the SAME bus can disagree: the SEN0680 is ABCD, the Y511-A is DCBA.
//
//  DCBA is why a Yosemitech reading looked like nonsense at first. Its bytes
//  arrive as 10 5C CA 41, which is the float 41 CA 5C 10 = 25.29 read
//  backwards. A vendor writing "little-endian" may mean word-swapped (CDAB)
//  or fully reversed (DCBA); only the plausible number tells you which.
// ---------------------------------------------------------------------------
#define FMT_FLOAT_ABCD   0
#define FMT_FLOAT_CDAB   1
#define FMT_INT_SCALED   2
#define FMT_FLOAT_DCBA   3

// TWO DEVICES, TWO BAUD RATES, NO RECONFIGURATION.
//
// The SEN0680 ships at 4800 8N1. The Y511-A is 9600 8N1 and stays silent at
// 4800 - tested, 45 s of nothing, then found immediately at 9600. So one of
// them would normally have to be reconfigured to match the other.
//
// It does not. SoftwareSerial can change rate at runtime, and the two sensors
// are never read at the same instant anyway, so the bus rate is switched
// before addressing each one. Both keep their factory settings, and in
// particular the SEN0680's baud register 0x07D1 is left alone - DFRobot does
// not document what value selects which rate, and a wrong guess could land it
// at 57600 or 115200 where SoftwareSerial on a 16 MHz AVR cannot reach it.
//
// The rate difference also separates the two devices even though both are on
// factory address 0x01: a frame sent at the wrong rate arrives as framing
// garbage with a broken CRC, and Modbus requires a valid CRC before a slave
// acts. Moving the Y511-A to its own address is still worth doing later as
// hygiene (register 0x3000, documented) - but nothing needs it to work.
const uint32_t DO_BAUD           = 4800;     // SEN0680, factory
const uint32_t TURB_BAUD         = 9600;     // Y511-A, fixed

uint32_t rs485CurrentBaud = 0;               // 0 = not begun yet

void rs485SetBaud(uint32_t baud) {
  if (baud == rs485CurrentBaud) return;
  if (rs485CurrentBaud) rs485.end();
  rs485.begin(baud);
  rs485CurrentBaud = baud;
  delay(20);                                 // let the receiver settle
}
const uint16_t MODBUS_TIMEOUT_MS = 1000;
const uint8_t  MODBUS_RETRIES    = 3;

// SETTLED ON THE BENCH, 2026-09-08. This module does NOT echo: it disables
// its receiver while transmitting, so nothing comes back but the slave's
// reply. Raw scan showed exactly 9 bytes, "01 03 04 40 D4 C1 62 7F B2", with
// no copy of our own 8-byte request in front of it.
//
// This was set to 1 for two days and that is what hid the sensor: the discard
// window swallowed the reply as if it were an echo, and every read reported
// "Modbus timeout". Do not change it back without re-running RS485Scan.
#define RS485_ECHOES_OWN_TX   0

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

// Confirmed on the wire, not just from the wiki. A register sweep read two
// registers from every address, so the windows overlap and the raw values can
// be reconstructed:
//    0x0000-01  3F51 0872  ->  0.8165        saturation, as a FRACTION
//    0x0002-03  40D4 A131  ->  6.64 mg/L     concentration
//    0x0004-05  41CC F9FF  ->  25.62 degC    temperature
//    0x0006-09  ~17.2                        internal, undocumented
//    0x000A+    all zero                     unused
// Cross-check: saturation at 25.6 degC is about 8.2 mg/L, and 6.64/8.2 =
// 0.81 - the same 0.8165 the first register reports. The decode is
// self-consistent, so ABCD is right; CDAB gives -0.0000 or ovf on every
// register that holds a real value.

// The DO sensor measures water temperature anyway, at the probe tip. Free to
// read - it is two registers further on - and it cross-checks the DS18B20,
// which sits in a different spot. Two independent temperatures that disagree
// is a fault you want to see in the data rather than discover later.
#define ENABLE_DO_TEMP  1
const uint16_t DO_TEMP_REG       = 0x0004;

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

// --- Yosemitech Y511-A turbidity (self-cleaning) ---------------------------
//  Registers from the EnviroDIY YosemitechModbus library; byte order and the
//  value layout confirmed on the wire at 9600 8N1.
//
//  Supply      DC 12-24 V, source must manage >500 mA for the wiper motor.
//              0.2 W idle, 0.6 W wiping. Keep the boost at ~13.5 V: 12 V is
//              the MINIMUM and the 10 m cable drops some of it.
//  Registers   0x2600-01  temperature, degC        <-- 25.29 measured
//              0x2602-03  turbidity, NTU           <-- 6.13 measured
//              0x2500     start measurement        0x2E00  stop
//              0x3100     activate brush           0x3200  brush interval, min
//              0x3000     slave address            0x1100  calibration K and B
//  Format      float, ALL BYTES REVERSED -> FMT_FLOAT_DCBA. Not the same as
//              the SEN0680 on the same bus, which is ABCD.
//
//  Which pair is which is inferred, not documented: 25.29 matched the ambient
//  temperature that both DO units independently reported (25.05 and 25.62).
//  Confirm it in water: stir in a little milk or silt and turbidity must jump
//  while temperature stays put. If they swap, exchange the two registers.
// Left at the factory address. The baud difference is what separates it from
// the SEN0680, so no write is needed. If you do move it to 0x03 via register
// 0x3000 for cleanliness, change this to match.
const uint8_t  TURB_ADDR         = 0x01;
const uint16_t TURB_REG          = 0x2602;   // turbidity, NTU
const uint8_t  TURB_FMT          = FMT_FLOAT_DCBA;
const float    TURB_SCALE        = 1.0;      // already NTU
const uint16_t TURB_TEMP_REG     = 0x2600;   // its own temperature, degC

//  Command registers. These are used to build the frames in
//  turbActivateBrush() and turbStartMeasurement(), so changing one here does
//  change what goes on the wire - see the YOSEMITECH COMMAND FRAMES block
//  further down for the bytes, the replies, and why the framing is unusual.
const uint16_t TURB_BRUSH_REG    = 0x3100;   // activateBrush,    fn 0x10
const uint16_t TURB_START_REG    = 0x2500;   // startMeasurement, fn 0x03
const uint8_t  TURB_NEEDS_START  = 1;
const uint16_t TURB_WARMUP_MS    = 35000;    // brush sweep + optics settle

//  ####################################################################
//  ##  THE BRUSH IS COMMANDED, NOT SCHEDULED                         ##
//  ##                                                                ##
//  ##  The Y511-A does have an automatic brush interval, and on this  ##
//  ##  unit register 0x3200 reads 0x001E - THIRTY MINUTES. (Note the  ##
//  ##  byte order: Yosemitech sends 16-bit values little-endian too,  ##
//  ##  so the bytes 1E 00 are 30, not 7680.)                          ##
//  ##                                                                ##
//  ##  It is useless here regardless, because that timer only counts  ##
//  ##  while the sensor is POWERED, and this node powers it for under ##
//  ##  a minute per hour - it would take 30 hours of service to       ##
//  ##  accumulate 30 minutes of power-on time. It is also the         ##
//  ##  explanation for the one sweep seen during bench work: the      ##
//  ##  probe sat continuously powered for well over 30 minutes during ##
//  ##  the RS485 debugging.                                           ##
//  ##                                                                ##
//  ##  So the sweep is commanded before every turbidity reading.      ##
//  ##  Anti-fouling is the entire reason this sensor was chosen over  ##
//  ##  a cheaper one, and a reading through a fouled window is worth  ##
//  ##  nothing - so the default is to brush every time rather than to ##
//  ##  save wiper cycles.                                            ##
//  ####################################################################
//  ####################################################################
//  ##  DISABLED 2026-09-09 - SUPPLY CANNOT DRIVE THE WIPER MOTOR     ##
//  ##                                                                ##
//  ##  With the brush command finally working, the first real sweep   ##
//  ##  showed the 12 V rail OSCILLATING: the boost LED pulses and the ##
//  ##  wiper turns continuously instead of making one stroke. That is ##
//  ##  the boost failing to supply the motor inrush - Yosemitech asks ##
//  ##  for a source able to deliver over 500 mA - so the rail         ##
//  ##  collapses, the motor stalls, the load drops, the boost         ##
//  ##  recovers, and it repeats.                                     ##
//  ##                                                                ##
//  ##  This is not merely ineffective, it is damaging. A motor        ##
//  ##  restarted repeatedly under brownout sits at high current with  ##
//  ##  little torque and heats its winding. Worse, the probe's own    ##
//  ##  microcontroller browns out on every cycle, and its EEPROM      ##
//  ##  holds the calibration constants and the slave address.         ##
//  ##                                                                ##
//  ##  Note this is why it never appeared before: until the framing   ##
//  ##  was corrected the brush command did nothing, so the motor was  ##
//  ##  never actually asked to run. And in TurbBrush/ it swept        ##
//  ##  cleanly because the 12 V rail was fed from a bench supply.     ##
//  ##                                                                ##
//  ##  RESOLVED: the rail was at 12 V, the Y511-A's own minimum, so    ##
//  ##  the motor load pulled it under. At 14 V the sweep is clean and  ##
//  ##  draws 0.8 W measured - about 57 mA, nowhere near the ">500 mA"  ##
//  ##  on the datasheet, which describes what the SOURCE must be able  ##
//  ##  to deliver rather than what the motor actually takes. Re-armed. ##
//  ##                                                                 ##
//  ##  If the oscillation ever returns, check the rail voltage under   ##
//  ##  the sweep before anything else: it is the rail sagging to the   ##
//  ##  sensor's minimum, not the command or the motor.                 ##
//  ####################################################################
#define TURB_BRUSH_BEFORE_READ   1

//  Diagnostic pause, off. It answered its question: the wiper does not move
//  while the rail simply sits powered, so power-up is not a trigger. Set it
//  to 1 again for the next sensor that needs the same question asked.
#define TURB_TRIGGER_PROBE       0
const uint16_t TURB_PROBE_MS     = 15000;

//  Every Nth turbidity reading gets a sweep. 1 = every reading, which at an
//  hourly turbidity interval is 24 sweeps a day. Energy is not the concern -
//  a 10 s sweep at 0.6 W is 6 J, so 24 of them are 0.04 Wh - but mechanical
//  wear might be, and Yosemitech does not publish a wiper lifetime. Raise
//  this only if you have asked them and know the number.
const uint8_t  TURB_BRUSH_EVERY_N = 2;
uint8_t        turbReadCount = 0;

// Rail settling times.
//   3.3 V: the EZO circuits need roughly a second to boot after power-up.
//   12 V : boost start-up plus charging the 1000 uF bulk capacitor.
const uint16_t RAIL33_SETTLE_MS  = 1200;
// 3000, not 500. At 500 ms the first Modbus command went out 522 ms after
// the rail came up (measured from the log timestamps) and the Y511-A had not
// finished booting - it answered NO REPLY to the brush command. TurbBrush.ino,
// where the wiper does sweep, waits 3000 ms. That is the number that works.
const uint16_t RAIL12_SETTLE_MS  = 3000;

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
//   battery = analogRead(A0) * (Vcc / 1023) * BATT_DIVIDER
//
// Vcc has to be measured rather than assumed, because Vcc IS the ADC's
// reference: the raw count alone says nothing about volts. It comes from the
// AVR's internal 1.1 V bandgap read back through the ADC, and every chip's
// bandgap is off by a percent or two - that is what VccCorrection trims.
//
// CALIBRATED ON THIS UNIT. Vcc untrimmed reads 3.3129 V, repeatable to four
// decimals, so the bandgap needs no trim and the correction stays at 1.0.
//
// The divider then follows from a run with the cell in place:
//   A0     = 627.7 counts
//   V(A0)  = 627.7 * 3.3129 / 1023 = 2.034 V
//   cell   = 4.07 V by meter       -> factor 2.00
//
// A FIRST ATTEMPT GOT 2.696, and the mistake is worth recording: the ADC
// count came from a VccCal run whose battery domain sat near 3.0 V, while the
// meter reading of 4.07 V was taken separately. Fitting one state's counts to
// another state's volts produces a plausible-looking number that is simply
// wrong - and it wrongly discredited the board's own documentation. Both
// halves of a calibration must come from the same instant.
//
// 2.00 is exactly the 470k/470k ratio the pin comment claimed all along, so
// there is no ADC source-impedance artefact to worry about either.
//
// The two constants are pure multipliers on the same result, so only their
// PRODUCT is fixed by that measurement. 3.3129 V is already within half a
// percent of a 3.30 V regulator, so the correction stays at 1.0 and the whole
// factor sits in the divider, where it physically belongs.
//
// The inherited values were both wrong, from opposite directions: 5.0/2.6
// reported 11.96 V for a 3.19 V cell, and 3.85/7.5 with a 3.83 divider
// reported 2.97 V for a 4.07 V one.
const float VccCorrection = 1.0;
Vcc vcc(VccCorrection);

// The on-board 470k/470k pair. Still worth one confirmation against the
// meter at around 3.7 V as the cell discharges - a resistor ratio is linear,
// so it should track, and if it ever does not the cause would be the 235k
// source impedance starving the ADC's sample-and-hold (cure: 100 nF from A0
// to GND). Note the value with FTDI and cell in the SAME state both times.
const float BATT_DIVIDER = 2.0;

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

// ===========================================================================
//  SLEEP HOUSEKEEPING
//
//  Measured sleep current before any of this: 20.78 mA, of which 14.2 mA is
//  the three devices on the switched 3.3 V rail still running (EZO-pH
//  5.58 mA, EZO-EC 4.58 mA, transceiver plus bias 4.04 mA). Those are normal
//  operating currents, not leakage - so the rail itself is live and NO amount
//  of pin housekeeping will fix that. See RailTest/ for the hardware side.
//
//  What the code CAN fix is everything on the MCU's own side:
// ===========================================================================
void parkPinsForSleep() {
  // 1. SoftwareSerial arms a PIN-CHANGE interrupt on its receive pin, and
  //    pin-change is precisely the interrupt class that wakes an ATmega out
  //    of powerDown. Left armed it can fire on noise or on a floating pin,
  //    and each wake runs the core at 16 MHz - roughly 10 mA - for a moment.
  //    It looks exactly like a quiescent drain somewhere else entirely.
  rs485.end();
  rs485CurrentBaud = 0;

  // 2. TWI off at the register. The Arduino Wire library enables the AVR's
  //    INTERNAL pull-ups on A4/A5, and those sit on the MCU's permanently
  //    powered 3.3 V - so once the EZO circuits lose their supply, those
  //    pull-ups push current through the circuits' ESD clamps into a dead
  //    rail. A pull-up is tens of kilohms so it is only microamps, but it is
  //    the wrong direction to leave current flowing, and it can hold an
  //    unpowered chip in an undefined state.
  TWCR = 0;

  // 3. Park each pin according to WHAT IS AT THE OTHER END. An earlier
  //    version of this drove them all LOW, which doubled sleep current from
  //    20.8 to 44.8 mA - because D3 sits on the transceiver's RO OUTPUT, so
  //    an AVR output driving low fought a transceiver output driving high.
  //    That is the same push-pull contention that makes swapping D3/D4 by
  //    hand a bad idea; there is no reason it becomes safe in software.
  //
  //    D3  transceiver RO -> MCU input.  NEVER drive this pin. Input with the
  //        internal pull-up: it cannot fight RO while the rail is live, and
  //        it cannot float once the rail is dead and RO goes high-impedance.
  pinMode(RS485_RX_PIN, INPUT_PULLUP);

  //    D4  MCU output -> transceiver DI. Safe to drive, but pointless to
  //        drive low: on an auto-direction module a low on DI looks like a
  //        start bit and asserts the driver, which then sits on the bus
  //        burning current. UART idle is a mark, so leave it pulled high.
  pinMode(RS485_TX_PIN, INPUT_PULLUP);

  //    A4/A5  open-drain I2C with external 4.7k pull-ups on the SWITCHED
  //        rail. Inputs with the internal pull-up OFF: while the rail is live
  //        the external resistors hold them high, and once it is dead those
  //        same resistors tie them to 0 V - so they never float either way,
  //        and nothing is driven into an unpowered EZO circuit. Driving them
  //        low would instead sink the pull-up current continuously and hold
  //        both circuits' bus permanently busy.
  pinMode(A4, INPUT);
  digitalWrite(A4, LOW);
  pinMode(A5, INPUT);
  digitalWrite(A5, LOW);

#if ENABLE_DS18B20
  //    A2  same reasoning: open-drain 1-Wire with its 4.7k pull-up on the
  //        switched rail.
  pinMode(ONE_WIRE_PIN, INPUT);
  digitalWrite(ONE_WIRE_PIN, LOW);
#endif

  // 4. Both rail enables low. Already done by railsOff(), repeated here so
  //    this function is correct on its own and does not depend on call order.
  digitalWrite(RAIL12_EN, LOW);
  digitalWrite(RAIL33_EN, LOW);

  // 5. The status LED. Cheap to forget, 2-3 mA if left on.
  digitalWrite(ledPin, LOW);
}

void unparkPinsAfterSleep() {
  // ------------------------------------------------------------------------
  //  D4 MUST BE RESTORED TO AN OUTPUT HERE, BY HAND.
  //
  //  SoftwareSerial sets its pin directions in the CONSTRUCTOR, not in
  //  begin(): begin() only computes the bit delays and calls listen(). So
  //  once parkPinsForSleep() has turned D4 into an input, nothing ever turns
  //  it back - the constructor ran once, at global init, and will not run
  //  again. write() manipulates the port register directly, and with the DDR
  //  bit clear that only toggles the pull-up. Not one bit reaches the bus.
  //
  //  The symptom is distinctive and cost a full debugging round: cycle 1
  //  after any reset works completely, and every cycle after the first sleep
  //  reports "Modbus timeout" for BOTH slaves, because the master has gone
  //  mute rather than the slaves having gone deaf. It also explains months
  //  of "an occasional DO reading" - exactly one working cycle per reset,
  //  and the brownouts were supplying the resets.
  //
  //  Level first, direction second: pinMode(OUTPUT) on a pin whose PORT bit
  //  is 0 drives it low for a few cycles, and a low on DI is a start bit.
  digitalWrite(RS485_TX_PIN, HIGH);       // UART idle is a mark
  pinMode(RS485_TX_PIN, OUTPUT);

  //  RX just needs the pull-up SoftwareSerial's setRX() would have applied.
  pinMode(RS485_RX_PIN, INPUT);
  digitalWrite(RS485_RX_PIN, HIGH);
  // ------------------------------------------------------------------------

  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif
#if ENABLE_DS18B20
  pinMode(ONE_WIRE_PIN, INPUT);
#endif
  // rs485CurrentBaud was cleared, so the next rs485SetBaud() runs begin()
  // again - which re-arms the receive interrupt and the bit timing, but NOT
  // the pin directions. Hence the block above.
}

void sleep(int sec_to_sleep) {
  Serial.print(F("Will sleep now for approximately "));
  Serial.print(sec_to_sleep);
  Serial.println(F(" seconds."));

  // Serial.flush() instead of delay(1000): it waits for exactly as long as
  // the last characters need to leave the UART, rather than a fixed second.
  Serial.flush();

  parkPinsForSleep();

  for (int i = 0; i < sec_to_sleep / 8; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }

  unparkPinsAfterSleep();
  Serial.print(F("--- awake after "));
  Serial.print(sec_to_sleep);
  Serial.println(F(" s"));
}

// ---------------------------------------------------------------------------
//  Rail control.  3.3 V comes up first (the transceiver and the RTD front-end
//  live on it), then the 12 V rail for the probes. Powering down in reverse.
// ---------------------------------------------------------------------------
// Switching a rail on with digitalWrite() reset this board. Measured: the
// first rails33On() after a cold start browned the MCU out (MCUSR = BORF)
// before its own log line could leave the UART buffer. Closing the low-side
// MOSFET charges two EZO boards' and the transceiver's bulk capacitors in one
// step, and the "Sensor Power" plus terminal is fed by the SAME 3.3 V
// regulator as the ATmega - so the inrush hits the MCU's own supply. The peak
// exceeded 500 mA (a bench supply capped there stayed in constant-current).
//
// D6 and D7 are both Timer0 PWM outputs, so the switch-on can be ramped with
// no extra hardware: 64 steps of rising duty, roughly one 490 Hz period each,
// then a hard HIGH. Whether that walks the gate through its linear region or
// simply chops the load depends on the board's gate network, which is not
// documented - either way the AVERAGE inrush is limited and the rail recovers
// between pulses, which is what the BOD cares about.
//
// A 470 uF bulk capacitor across the 3.3 V rail fixed the cold start on its
// own. This ramp is what makes it independent of residual charge: after a
// 30-minute sleep the device capacitors have bled down, so every cycle would
// otherwise face the same full inrush the cold start did.
//
// digitalWrite() on a PWM pin calls turnOffPWM() itself, so the final HIGH
// releases the timer compare unit. Timer0 keeps running: millis() is intact.
void railOnSoft(uint8_t pin) {
  for (uint8_t d = 4; d < 252; d += 4) {
    analogWrite(pin, d);
    delay(2);
  }
  digitalWrite(pin, HIGH);
}

void rails33On() {
  Serial.println(F("3.3 V sensor rail ON  (D6)"));
  Serial.flush();                 // a brownout here must not eat the evidence
  railOnSoft(RAIL33_EN);
  delay(RAIL33_SETTLE_MS);        // EZO boot time
}

// D7 gets a HARD switch-on, deliberately. The soft-start reasoning does not
// transfer: D7 only drives the opto module's input, and everything behind it
// is fed by the boost converter, which is tapped ahead of the board. An
// inrush there cannot reach the ATmega's supply, so there is nothing to
// protect - and ramping it does harm. 128 ms of 490 Hz chopping makes the
// boost's output pulse, and the Y511-A boots into a pulsing 14 V rail with a
// motor attached. That is the same symptom seen when the boost was set to
// 12 V: the LED pulsing and the wiper creeping round.
void rails12On() {
  Serial.println(F("12 V sensor rail ON   (D7)"));
  Serial.flush();
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
      Serial.print(F("  Modbus timeout 0x"));
      Serial.println(slave, HEX);
      continue;
    }
    if (resp[0] != slave || resp[1] != 0x03) {
      Serial.println(F("  Modbus bad header"));
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

// ===========================================================================
//  YOSEMITECH COMMAND FRAMES
//
//  Several Yosemitech actions are triggered by frames that are NOT valid
//  Modbus: the register-COUNT field is 0x0000. A standard read or write to
//  the same register is a different frame on the wire and gets ignored, which
//  is exactly why earlier attempts at 0x3100 did nothing at all.
//
//  Verified on the wire, 2026-09-09, with the wiper watched:
//
//    activateBrush     01 10 31 00 00 00 00 74 94
//                   -> 01 10 31 00 00 00 CE F5      MOVES THE WIPER
//
//    startMeasurement  01 03 25 00 00 00 4E C6
//                   -> 01 03 00 20 F0               accepted, no sweep
//
//  So the register was right all along and only the framing was wrong. And
//  the guess that startMeasurement sweeps as a side effect was wrong: it does
//  not. The sweep seen during earlier bench work was almost certainly the
//  sensor's own interval timer in 0x3200, which could actually elapse while
//  the probe sat powered on the desk for minutes at a time - something it
//  never gets to do in service.
// ===========================================================================
bool rs485RawCommand(const uint8_t *payload, uint8_t len) {
  uint8_t f[12];
  memcpy(f, payload, len);
  uint16_t crc = modbusCRC(f, len);
  f[len]     = crc & 0xFF;
  f[len + 1] = (crc >> 8) & 0xFF;

  rs485Flush();
  rs485.write(f, len + 2);
  rs485.flush();
  delay(2);
  rs485DiscardEcho(len + 2);

  // The acknowledgement's shape differs per command, so this only checks
  // that something came back rather than parsing it.
  uint8_t got = 0;
  unsigned long t0 = millis();
  while (got < 24 && (millis() - t0) < 300) {
    if (rs485.available()) { rs485.read(); got++; }
  }
  return got > 0;
}

bool turbActivateBrush() {
  // addr, fn 0x10, register, count 0x0000, byte-count 0x00
  const uint8_t f[7] = {TURB_ADDR, 0x10,
                        (uint8_t)(TURB_BRUSH_REG >> 8),
                        (uint8_t)(TURB_BRUSH_REG & 0xFF),
                        0x00, 0x00, 0x00};
  return rs485RawCommand(f, 7);
}

bool turbStartMeasurement() {
  // addr, fn 0x03, register, count 0x0000
  const uint8_t f[6] = {TURB_ADDR, 0x03,
                        (uint8_t)(TURB_START_REG >> 8),
                        (uint8_t)(TURB_START_REG & 0xFF),
                        0x00, 0x00};
  return rs485RawCommand(f, 6);
}

float modbusReadValue(uint8_t slave, uint16_t reg, uint8_t fmt, float scale) {
  uint16_t r[2];

  if (fmt == FMT_INT_SCALED) {
    if (!modbusReadRegisters(slave, reg, 1, r)) return NAN;
    return (float)((int16_t)r[0]) * scale;
  }

  if (!modbusReadRegisters(slave, reg, 2, r)) return NAN;

  uint32_t raw;
  if (fmt == FMT_FLOAT_ABCD) {
    raw = ((uint32_t)r[0] << 16) | r[1];
  } else if (fmt == FMT_FLOAT_DCBA) {
    // Every byte reversed, not just the words. The registers arrive
    // big-endian on the wire, so pull them apart and rebuild backwards.
    uint8_t b0 = (uint8_t)(r[0] >> 8), b1 = (uint8_t)(r[0] & 0xFF);
    uint8_t b2 = (uint8_t)(r[1] >> 8), b3 = (uint8_t)(r[1] & 0xFF);
    raw = ((uint32_t)b3 << 24) | ((uint32_t)b2 << 16) |
          ((uint32_t)b1 << 8)  |  (uint32_t)b0;
  } else {
    raw = ((uint32_t)r[1] << 16) | r[0];                            // CDAB
  }

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
// 16 samples each, not 100. The original 200 samples with 5 ms between them
// cost a full second of awake time to average away noise that 16 samples
// already handle - the ADC's own repeatability is far better than the
// uncalibrated divider factor below, so more averaging buys nothing.
const uint8_t BATT_SAMPLES = 16;

float readVolts() {
  float vcc_reg = 0;
  for (uint8_t j = 0; j < BATT_SAMPLES; j++) { vcc_reg += vcc.Read_Volts(); delay(2); }
  vcc_reg /= BATT_SAMPLES;

  float last_vcc = 0;
  for (uint8_t i = 0; i < BATT_SAMPLES; i++) {
    last_vcc += ((analogRead(batt_pin) * (vcc_reg / 1023.0)) * BATT_DIVIDER);
    delay(2);
  }
  last_vcc /= BATT_SAMPLES;

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
  if (doTurbidity) {

#if TURB_TRIGGER_PROBE
    // Nothing is sent during this window. If the wiper moves here, the rail
    // coming up is what triggers it and no command is needed at all.
    Serial.print(F("Turbidity: silent pause, no command sent - "));
    Serial.print(TURB_PROBE_MS / 1000);
    Serial.println(F(" s. Does the wiper move NOW?"));
    Serial.flush();
    delay(TURB_PROBE_MS);
    Serial.println(F("Turbidity: pause over, sending commands from here on"));
#endif

    rs485SetBaud(TURB_BAUD);

#if TURB_BRUSH_BEFORE_READ
    if (TURB_BRUSH_EVERY_N <= 1 || (turbReadCount % TURB_BRUSH_EVERY_N) == 0) {
      Serial.print(F("Turbidity: brush sweep ("));
      Serial.print(turbActivateBrush() ? F("acknowledged") : F("NO REPLY"));
      Serial.println(F(") - ~10 s"));
    }
    turbReadCount++;
#endif

    if (TURB_NEEDS_START) {
      Serial.print(F("Turbidity: start measurement ("));
      Serial.print(turbStartMeasurement() ? F("acknowledged") : F("NO REPLY"));
      Serial.println(F(")"));
    }
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
  rs485SetBaud(DO_BAUD);
  v_do = modbusReadValue(DO_ADDR, DO_REG, DO_FMT, DO_SCALE);
  Serial.print(F("Dissolved oxygen: "));
  if (isnan(v_do)) Serial.println(F("FAIL"));
  else { Serial.print(v_do, 2); Serial.println(F(" mg/L")); }

#if ENABLE_DO_TEMP
  float t3 = modbusReadValue(DO_ADDR, DO_TEMP_REG, DO_FMT, 1.0);
  if (t3 > -20.0 && t3 < 60.0) v_temp2 = t3;      // same sanity band as above
  Serial.print(F("Temperature (DO probe): "));
  if (isnan(v_temp2)) Serial.println(F("FAIL"));
  else {
    Serial.print(v_temp2, 2);
    Serial.print(F(" degC"));
    if (!isnan(v_temp)) {
      Serial.print(F("   (DS18B20 differs by "));
      Serial.print(fabs(v_temp2 - v_temp), 2);
      Serial.print(F(" K)"));
    }
    Serial.println();
  }
#endif
#endif

  // --- pH and EC, temperature-compensated ---
#if ENABLE_PH
  ezoSetTemperature(EZO_PH_ADDR, v_temp);
  v_ph = ezoRead(EZO_PH_ADDR);

  // ezoRead() only returns NAN when the I2C exchange itself failed. If the
  // circuit answers with something atof() cannot parse, atof() returns 0.0 -
  // and 0.0 is a number, so it would go out as a valid pH reading. It is not
  // one: the EZO measures from 0.001 upwards and lake water never comes near
  // it. Same idea as the DS18B20 bands, which reject -127 and +85.
  if (!isnan(v_ph) && (v_ph < 0 || v_ph > 14.0)) {
    Serial.print(F("pH: rejected "));
    Serial.print(v_ph, 2);
    Serial.println(F(" - outside 0.5..14, treating as no reading"));
    v_ph = NAN;
  }

  Serial.print(F("pH: "));
  if (isnan(v_ph)) Serial.println(F("FAIL (no probe attached?)"));
  else Serial.println(v_ph, 2);
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
    rs485SetBaud(TURB_BAUD);
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
//    ch 5  Conductivity     addAnalogInput   *** sent as uS/cm / 100 ***
//    ch 6  Turbidity        addAnalogInput   *** sent as NTU/10 ***
//    ch 7  2nd temperature  addTemperature   the SEN0680's own temperature,
//                                             measured at the DO probe tip.
//                                             A cross-check on ch 1, which is
//                                             the DS18B20 in a different spot.
//
//  !! LPP_ANALOG_INPUT is a signed 2-byte value with 0.01 resolution, so its
//  !! range is only +/-327.67. EC in uS/cm and turbidity up to 1000 NTU would
//  !! OVERFLOW - hence the scaling. The WaziCloud decoder must undo it:
//  !!    EC_uS = ch5 * 100 ;  NTU = ch6 * 10
//  !!
//  !! The two divisors are chosen from the SENSORS' accuracy, not for tidiness.
//  !!   EC  /100  -> 1 uS/cm steps, up to 32767 uS/cm. Dividing by 1000 instead
//  !!               would give 10 uS/cm steps, and Lake Victoria sits near
//  !!               100 uS/cm where the EZO-EC is accurate to about 2 uS/cm -
//  !!               the format would have been 5x coarser than the sensor.
//  !!   NTU /10   -> 0.1 NTU steps, up to 3276 NTU. Already finer than the
//  !!               Y511-A's +/-5 % or 0.3 NTU, so no reason to go further.
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
  if (!isnan(v_ec))    xlpp.addAnalogInput(5, v_ec / 100.0);    // uS/cm / 100
  if (!isnan(v_turb))  xlpp.addAnalogInput(6, v_turb / 10.0);   // NTU  -> NTU/10
  if (!isnan(v_temp2)) xlpp.addTemperature(7, v_temp2);


  // The 3 s delay that used to sit here came from the WaziDev example and
  // served nothing: the radio is already initialised and the rails are
  // already down by this point, so it was three seconds of the MCU simply
  // waiting. Removed.
  serialPrintf(("LoRaWAN send ... "));
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
//  WHY DID IT RESTART?
//
//  A boot loop looks identical whether the cause is a brownout, the watchdog,
//  a floating RESET pin or a genuine power cycle - the log just begins again.
//  The AVR records the reason in MCUSR, but that register must be read before
//  anything else touches it, so this runs from .init3, ahead of main().
//
//    PORF  power-on      normal at first boot, or a supply that fully collapsed
//    EXTRF external      the RESET pin - a loose DTR line or the FTDI
//    BORF  brown-out     the supply sagged past the BOD threshold UNDER LOAD
//    WDRF  watchdog      code hung and the watchdog fired
//
//  BORF repeating is the answer to "why does it reset during radio init":
//  the SX1276 draws its peak there and the rail cannot hold it.
// ===========================================================================
uint8_t resetFlags __attribute__((section(".noinit")));

void captureResetFlags(void) __attribute__((naked, used, section(".init3")));
void captureResetFlags(void) {
  resetFlags = MCUSR;
  MCUSR = 0;
}

// A WATCHDOG WAS FITTED HERE AND REMOVED AGAIN. It is still the right idea -
// this AVR core's Wire has no timeout, so a device holding SDA low blocks
// forever - but the 8 s window was fed per PHASE and that is not often
// enough. Measured from Felix's own logs: 5.57 s from the top of loop() to
// the DO warm-up on a good cycle, and 7.54 s from the end of that warm-up to
// the EC reading on a cycle where Modbus timed out. Against an 8 s window
// that resets the node in exactly the situation the dog was meant to survive.
//
// The next attempt must call wdt_reset() inside the low-level primitives -
// modbusTransaction(), ezoCommand(), readOneWire() - not around the phases
// that call them. Then the interval is one operation, not one group.
void reportResetCause() {
  Serial.print(F(" Restart cause: MCUSR=0x"));
  Serial.print(resetFlags, HEX);
  if (resetFlags & _BV(PORF))  Serial.print(F("  POWER-ON"));
  if (resetFlags & _BV(EXTRF)) Serial.print(F("  EXTERNAL-RESET"));
  if (resetFlags & _BV(BORF))  Serial.print(F("  BROWN-OUT"));
  if (resetFlags & _BV(WDRF))  Serial.print(F("  WATCHDOG"));
  if (resetFlags == 0)         Serial.print(F("  ?"));
  Serial.println();
}

// ===========================================================================
//  SETUP / LOOP
// ===========================================================================
void setup() {
  Serial.begin(38400);
  reportResetCause();

  pinMode(ledPin, OUTPUT);

  pinMode(RAIL33_EN, OUTPUT);
  pinMode(RAIL12_EN, OUTPUT);
  digitalWrite(RAIL33_EN, LOW);           // both rails OFF at boot
  digitalWrite(RAIL12_EN, LOW);

#if ENABLE_PT1000
  pinMode(MAX31865_CS, OUTPUT);
  digitalWrite(MAX31865_CS, HIGH);        // deselect before the radio inits
#endif

  rs485SetBaud(DO_BAUD);        // rate is re-selected before each sensor

  Wire.begin();                           // A4/A5, pull-ups are on-board

  // The AVR Wire library has NO timeout before core 1.8.1: if a device holds
  // SDA or SCL low, endTransmission() blocks in a hardware wait loop and the
  // sketch stops dead - no message, no reset, the log just ends. Use the
  // native timeout where the core provides it; i2cBusIdle() below is the
  // fallback, and it is a net, not a cure (see its comment).
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000 /* us */, true /* reset the bus on timeout */);
  Serial.println(F(" I2C guard: native 25 ms"));
#else
  Serial.println(F(" I2C guard: pre-flight only (old core)"));
#endif

  blink_led();

  Serial.println(F(" KijaniSpace water node - WaziSense v2"));

#if ENABLE_PT1000
  // SPI is shared: the MAX31865 is initialised per cycle in readPT1000(),
  // because its rail is switched off in between.
#endif
  Serial.println(F(" Radio init (peak current)"));
  Serial.flush();
  wazidev.setupLoRaWAN(devAddr, LoRaWANKeys);
  sx1272.setSF(SF_12);                    // gateway accepts SF12 only
  Serial.println(F(" Radio OK"));
  Serial.flush();
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
