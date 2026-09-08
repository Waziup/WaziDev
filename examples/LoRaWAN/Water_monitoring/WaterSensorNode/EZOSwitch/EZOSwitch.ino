/*
 * ============================================================================
 *  EZOSwitch  --  switch ONE Atlas EZO circuit from UART to I2C, no typing
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud   (read-only - nothing needs to be typed)
 *
 *  Connect ONE circuit's SDA+SCL, upload, watch. It does this by itself:
 *
 *    1. powers the 3.3 V sensor rail (D6) and waits for the circuit to boot
 *    2. sends  C,0   to stop the continuous reading stream
 *    3. sends  i     and counts how many circuits answer
 *    4. reads the device type out of the reply and picks the address itself:
 *         ?I,pH,...  ->  I2C,99   (0x63)
 *         ?I,EC,...  ->  I2C,100  (0x64)
 *    5. sends that one command, once, and stops
 *
 *  IT REFUSES TO ACT unless exactly one circuit answers with a device type it
 *  recognises. That interlock is the whole point: both circuits share these
 *  two wires, so a command sent with both connected is obeyed by BOTH, which
 *  would put them on the same address and lock you out of each of them. The
 *  only way back from that is the datasheet's hardware procedure (PRB shorted
 *  to TX while powering up) on both boards.
 *
 *  It also never guesses which circuit it is talking to. The address comes
 *  from the circuit's own answer, not from which cable you think you plugged
 *  in - so a swapped cable cannot give the pH circuit the EC address.
 *
 *  AFTER IT SUCCEEDS the circuit reboots into I2C mode and goes silent on
 *  UART. Swap the two data wires to the other circuit, press reset, and it
 *  runs again. Then verify both with I2CScan: SDA driven-low must fall to the
 *  same 10-20 mV as SCL, and 0x63 + 0x64 must both ACK.
 * ============================================================================
 */

#include <SoftwareSerial.h>

const int RAIL33_EN = 6;      // "Sensor Power 1" -> switched 3.3 V
const int ledPin    = 8;      // on-board LED1

const int EZO_RX_PIN = A4;    // we receive the EZO's TX here (your SDA wire)
const int EZO_TX_PIN = A5;    // we transmit to the EZO's RX here (your SCL wire)

const uint16_t RAIL_SETTLE_MS = 1500;   // EZO circuits need ~1 s to boot
const uint16_t QUIET_MS       = 1200;   // after C,0, let the stream die down
const uint16_t INFO_WINDOW_MS = 2500;   // how long to listen for ?I, replies

SoftwareSerial ezo(EZO_RX_PIN, EZO_TX_PIN);

bool acted = false;           // set once we have sent an I2C, command

void drain() {
  uint32_t t0 = millis();
  while (millis() - t0 < 300) {
    while (ezo.available()) { ezo.read(); t0 = millis(); }
  }
}

void send(const char *s) {
  Serial.print(F(" > "));
  Serial.println(s);
  ezo.print(s);
  ezo.print('\r');
}

// Listens for windowMs, echoes everything, counts "?I," replies and copies
// the device type out of the first one. Returns the number of replies.
uint8_t collectInfo(uint16_t windowMs, char *device, uint8_t deviceSize) {
  uint8_t count = 0;
  char    line[56];
  uint8_t len = 0;
  device[0] = 0;

  uint32_t t0 = millis();
  while (millis() - t0 < windowMs) {
    while (ezo.available()) {
      char c = ezo.read();

      if (c == '\r' || c == '\n') {
        if (len == 0) continue;
        line[len] = 0;
        Serial.print(F("   < "));
        Serial.println(line);

        if (strncmp(line, "?I,", 3) == 0) {
          count++;
          if (count == 1) {
            const char *p = line + 3;
            uint8_t i = 0;
            while (*p && *p != ',' && i < deviceSize - 1) device[i++] = *p++;
            device[i] = 0;
          } else {
            Serial.println(F("   !! a second circuit answered"));
          }
        }
        len = 0;
        continue;
      }

      if (len < sizeof(line) - 1) line[len++] = c;
    }
  }
  return count;
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(RAIL33_EN, OUTPUT);
  digitalWrite(RAIL33_EN, HIGH);
  ezo.begin(9600);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" EZO auto-switch: UART -> I2C"));
  Serial.println(F("=================================================="));
  Serial.println(F(" Nothing to type. ONE circuit's SDA+SCL connected."));
  Serial.print(F(" 3.3 V rail ON, waiting "));
  Serial.print(RAIL_SETTLE_MS);
  Serial.println(F(" ms for boot..."));
  delay(RAIL_SETTLE_MS);
  Serial.println(F("--------------------------------------------------"));

  drain();

  Serial.println(F("Step 1  stop the continuous reading stream"));
  send("C,0");
  delay(QUIET_MS);
  drain();

  Serial.println(F("Step 2  ask who is there"));
  send("i");

  char device[12];
  uint8_t n = collectInfo(INFO_WINDOW_MS, device, sizeof(device));

  Serial.println(F("--------------------------------------------------"));
  Serial.print(F("Replies to `i`: "));
  Serial.println(n);

  if (n == 0) {
    Serial.println(F("NOTHING ANSWERED - no command sent."));
    Serial.println(F(" Either this circuit is ALREADY in I2C mode (then you"));
    Serial.println(F(" are done with it - verify with I2CScan), or its SDA/SCL"));
    Serial.println(F(" wires are not making contact. Reseat them and reset."));
    digitalWrite(ledPin, LOW);
    return;
  }

  if (n > 1) {
    Serial.println(F("MORE THAN ONE CIRCUIT ANSWERED - no command sent."));
    Serial.println(F(" Both are on the bus. Any command would be obeyed by"));
    Serial.println(F(" both and put them on the same address. Disconnect one"));
    Serial.println(F(" circuit's SDA+SCL and press reset."));
    digitalWrite(ledPin, LOW);
    return;
  }

  Serial.print(F("Device type: "));
  Serial.println(device);

  int16_t addr = -1;
  if      (strcasecmp(device, "pH") == 0) addr = 99;    // 0x63
  else if (strcasecmp(device, "EC") == 0) addr = 100;   // 0x64

  if (addr < 0) {
    Serial.println(F("UNRECOGNISED DEVICE TYPE - no command sent."));
    Serial.println(F(" This sketch only knows pH and EC. Anything else needs"));
    Serial.println(F(" an address chosen by hand via EZOBridge."));
    digitalWrite(ledPin, LOW);
    return;
  }

  char cmd[12];
  snprintf(cmd, sizeof(cmd), "I2C,%d", addr);

  Serial.print(F("Step 3  switching "));
  Serial.print(device);
  Serial.print(F(" to I2C address 0x"));
  Serial.println(addr, HEX);
  send(cmd);
  acted = true;

  delay(1500);
  drain();

  Serial.println(F("--------------------------------------------------"));
  Serial.println(F("DONE. The circuit has rebooted into I2C mode and is now"));
  Serial.println(F("silent on UART - that silence is the success signal."));
  Serial.println();
  Serial.println(F("Next: move SDA+SCL to the other circuit and press reset."));
  Serial.println(F("When both are done, connect both and run I2CScan:"));
  Serial.println(F("  SDA driven-low must fall to ~12 mV, same as SCL"));
  Serial.println(F("  0x63 and 0x64 must both ACK"));
  digitalWrite(ledPin, LOW);
}

void loop() {
  // Slow blink = finished and idle. Nothing else happens; the switch is a
  // one-shot action and must not repeat.
  digitalWrite(ledPin, acted ? HIGH : LOW);
  delay(acted ? 120 : 900);
  digitalWrite(ledPin, LOW);
  delay(acted ? 1880 : 900);
}
