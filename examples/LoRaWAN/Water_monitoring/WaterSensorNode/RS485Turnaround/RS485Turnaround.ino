/*
 * ============================================================================
 *  RS485Turnaround  --  how long does the module keep the bus after sending?
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud (read-only). No sensor needed.
 *
 *  THE LAST MEASURABLE UNKNOWN
 *  On this build everything else is proven: sensor power (10.6 mA at 12 V),
 *  the bias network (A 1.845 V, B 1.52 V, A-B 271 mV against a predicted
 *  268 mV), the 120 ohm terminator, the driver (A-B swings negative, which
 *  the passive network cannot do), the receiver (RO drives a valid HIGH from
 *  the idle bias), the D3/D4 mapping, and the yellow/blue assignment. Yet not
 *  one byte has ever been received, from any address, at any baud rate.
 *
 *  That leaves the turnaround. An auto-direction module has no DE pin, so a
 *  fixed RC decides how long it goes on driving after the last bit. Modbus
 *  RTU requires the master to release the bus within 3.5 character times -
 *  7.3 ms at 4800 baud. If this module's RC is longer than the slave's reply
 *  delay, our own driver sits on top of the reply and destroys it. Long
 *  enough, and the entire reply vanishes rather than arriving truncated,
 *  which is exactly what we see.
 *
 *  HOW IT IS MEASURED
 *  No echo has ever been received, so the module disables RO while it
 *  transmits. That gives a clean marker:
 *
 *      driver active   ->  RO high-impedance  ->  D3 floats
 *      receiver back   ->  RO driving         ->  D3 is driven
 *
 *  So: send a frame, then repeatedly discharge D3 and see whether anything
 *  pulls it back up. The moment it does is the end of the turnaround. Each
 *  pass costs about 90 us, which is ample resolution to tell 2 ms from 30 ms.
 *
 *  READING THE RESULT
 *  The test is NOT whether we come close to 3.5 character times as if that
 *  were a deadline to miss. It is the reverse: a slave only recognises
 *  end-of-frame after 3.5 characters of silence, so it CANNOT answer sooner.
 *  Anything below that has already released the bus in time.
 *
 *      3.5 characters:  7.3 ms at 4800   3.65 ms at 9600   1.8 ms at 19200
 *
 *      under 3.65 ms  fine at 4800 and 9600. The turnaround is not the fault,
 *                     so the sensor is not replying - and the evidence listed
 *                     above makes a solid fault report to send to DFRobot.
 *      3.65 - 7.3 ms  fine at 4800 only. Keep the bus at 4800.
 *      over 7.3 ms    THIS IS THE FAULT. The module cannot be a Modbus
 *                     master. Replace it with one that breaks out DE and let
 *                     the firmware control direction - D5 is free for it.
 * ============================================================================
 */

#include <SoftwareSerial.h>

const int RS485_RX_PIN = 3;      // module RO
const int RS485_TX_PIN = 4;      // module DI
const int RAIL33_EN    = 6;
const int ledPin       = 8;

const uint32_t RS485_BAUD = 4800;

const uint8_t  TRIALS       = 8;
const uint16_t GIVE_UP_US   = 60000;   // 60 ms is far past any sane RC
const uint16_t SETTLE_US    = 40;      // let a driven pin recover

SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

// One discharge-and-release pass on D3. True if something pulled it back up.
bool roIsDriven() {
  digitalWrite(RS485_RX_PIN, LOW);
  pinMode(RS485_RX_PIN, OUTPUT);          // discharge
  delayMicroseconds(20);
  pinMode(RS485_RX_PIN, INPUT);           // release, pull-up off
  digitalWrite(RS485_RX_PIN, LOW);
  delayMicroseconds(SETTLE_US);
  return digitalRead(RS485_RX_PIN);
}

// Returns microseconds from end-of-transmission until RO drives again,
// or 0 if it never did inside GIVE_UP_US.
uint32_t measureOnce() {
  uint8_t frame[8] = {0x01, 0x03, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00};
  // CRC for that request, so it looks like real traffic on the wire.
  frame[6] = 0x65;
  frame[7] = 0xCB;

  rs485.write(frame, 8);
  rs485.flush();                          // returns once the last bit is out

  uint32_t t0 = micros();
  while ((micros() - t0) < GIVE_UP_US) {
    if (roIsDriven()) return micros() - t0;
  }
  return 0;
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(RAIL33_EN, OUTPUT);
  digitalWrite(RAIL33_EN, HIGH);
  delay(1200);

  rs485.begin(RS485_BAUD);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" RS485 turnaround measurement"));
  Serial.println(F("=================================================="));
  Serial.print(F(" "));
  Serial.print(RS485_BAUD);
  Serial.println(F(" baud. Modbus allows 3.5 character times to"));
  Serial.println(F(" release the bus - 7.3 ms at 4800."));
  Serial.println(F("--------------------------------------------------"));

  uint32_t sum = 0, lo = 0xFFFFFFFF, hi = 0;
  uint8_t  ok = 0;

  for (uint8_t i = 0; i < TRIALS; i++) {
    uint32_t us = measureOnce();

    Serial.print(F("  trial "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    if (us == 0) {
      Serial.println(F("RO never came back within 60 ms"));
    } else {
      Serial.print(us / 1000.0, 2);
      Serial.println(F(" ms"));
      sum += us;
      if (us < lo) lo = us;
      if (us > hi) hi = us;
      ok++;
    }
    delay(200);
  }

  Serial.println(F("--------------------------------------------------"));

  if (ok == 0) {
    Serial.println(F("RO NEVER RE-ENABLED. Either the module holds the bus"));
    Serial.println(F("for longer than 60 ms - hopeless for Modbus - or it"));
    Serial.println(F("keeps RO disabled permanently after transmitting."));
    Serial.println(F("Either way: replace it with a module that breaks out"));
    Serial.println(F("DE, and drive direction from D5 in firmware."));
  } else {
    uint32_t avg = sum / ok;
    Serial.print(F("min "));
    Serial.print(lo / 1000.0, 2);
    Serial.print(F(" ms   avg "));
    Serial.print(avg / 1000.0, 2);
    Serial.print(F(" ms   max "));
    Serial.print(hi / 1000.0, 2);
    Serial.println(F(" ms"));
    Serial.println();

    // The criterion is NOT how close we are to 3.5 character times as if it
    // were a deadline we might miss. It is the other way round: a Modbus
    // slave only recognises end-of-frame after 3.5 characters of silence, so
    // it CANNOT reply sooner than that. Any turnaround shorter than 3.5
    // characters is therefore fully in the clear.
    //   4800 baud  -> 7.3 ms      9600 -> 3.65 ms      19200 -> 1.8 ms
    if (hi < 3650) {
      Serial.println(F("VERDICT: turnaround is fine."));
      Serial.println(F(" Shorter than 3.5 character times, and a slave cannot"));
      Serial.println(F(" answer before that has elapsed - so the module has"));
      Serial.println(F(" long released the bus by the time a reply arrives."));
      Serial.println(F(" Good for 4800 and 9600; 19200 would be tight."));
      Serial.println(F(" The module releases the bus well inside the Modbus"));
      Serial.println(F(" budget, so it is not swallowing the reply. Every"));
      Serial.println(F(" part of the node side is now proven, and the sensor"));
      Serial.println(F(" still never answers. Time to treat the sensor as"));
      Serial.println(F(" faulty and send DFRobot the measurements."));
    } else if (hi < 7300) {
      Serial.println(F("VERDICT: fine at 4800, too slow above it."));
      Serial.println(F(" Inside the 7.3 ms of 3.5 characters at 4800 baud, so"));
      Serial.println(F(" replies get through - but it would clip them at 9600"));
      Serial.println(F(" and above. Keep the bus at 4800 with this module."));
    } else {
      Serial.println(F("VERDICT: THIS IS THE FAULT."));
      Serial.print(F(" The module keeps driving for "));
      Serial.print(hi / 1000.0, 1);
      Serial.println(F(" ms after the last"));
      Serial.println(F(" bit, past the 7.3 ms Modbus allows. It sits on top"));
      Serial.println(F(" of the reply and destroys it, which is why nothing"));
      Serial.println(F(" has ever been received from any device."));
      Serial.println(F(" Fix: a transceiver module with DE broken out, and"));
      Serial.println(F(" firmware direction control on D5."));
    }
  }
  Serial.println(F("=================================================="));

  digitalWrite(RAIL33_EN, LOW);
  digitalWrite(ledPin, LOW);
}

void loop() {
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  delay(2900);
}
