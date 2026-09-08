/*
 * ============================================================================
 *  EZOBridge  --  talk to one Atlas EZO circuit over UART, no rewiring
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud, line ending "Carriage return" or "Both NL & CR"
 *
 *  WHY THIS WORKS WITHOUT MOVING A SINGLE WIRE
 *  The EZO pins are dual-function: the pin that is SDA in I2C mode is TX in
 *  UART mode, and SCL is RX. Your existing I2C wiring therefore already IS a
 *  UART connection - crossed the right way round, because the EZO's TX has to
 *  reach the MCU's RX. That is also why the scanner sees SDA being driven
 *  high: a UART transmitter idles HIGH.
 *
 *      WaziSense A4  <--  EZO TX/SDA     (we receive here)
 *      WaziSense A5  -->  EZO RX/SCL     (we transmit here)
 *
 *  ############################################################
 *  ##  CONNECT ONE CIRCUIT AT A TIME.                        ##
 *  ##                                                        ##
 *  ##  Both circuits share these two wires. With both wired  ##
 *  ##  they transmit on top of each other, and worse: any    ##
 *  ##  command you send is obeyed by BOTH. Sending I2C,99    ##
 *  ##  with both connected puts both at address 0x63, after  ##
 *  ##  which neither can be addressed individually and the   ##
 *  ##  only way back is the datasheet's hardware procedure   ##
 *  ##  (PRB shorted to TX while powering up) on both boards. ##
 *  ##                                                        ##
 *  ##  So this sketch REFUSES an I2C,<n> command unless a    ##
 *  ##  preceding `i` was answered by exactly one circuit.    ##
 *  ############################################################
 *
 *  WHAT TO TYPE, IN THIS ORDER
 *    C,0        stop the continuous reading stream, so the log is readable.
 *               UART mode streams one reading per second by default.
 *    i          device info. Proves communication, tells you WHICH circuit is
 *               connected (?I,pH,... or ?I,EC,...), and unlocks I2C,<n>.
 *    I2C,99     switch the pH circuit to I2C address 0x63   <-- pH
 *    I2C,100    switch the EC circuit to I2C address 0x64   <-- EC
 *    L,0        LED off (worth doing before deployment, saves a few mA)
 *
 *  After I2C,<n> the circuit reboots into I2C mode and goes SILENT on UART.
 *  That silence is the success signal, not a failure. Verify with I2CScan:
 *  the SDA line's driven-low value must drop to the same 10-20 mV as SCL,
 *  because an I2C pin is open-drain and never drives high.
 * ============================================================================
 */

#include <SoftwareSerial.h>

const int RAIL33_EN = 6;      // "Sensor Power 1" -> switched 3.3 V
const int ledPin    = 8;      // on-board LED1

const int EZO_RX_PIN = A4;    // we receive the EZO's TX here (your SDA wire)
const int EZO_TX_PIN = A5;    // we transmit to the EZO's RX here (your SCL wire)

const uint16_t RAIL_SETTLE_MS = 1500;    // EZO circuits need ~1 s to boot
const uint32_t INFO_VALID_MS  = 120000;  // how long an `i` result unlocks I2C,

SoftwareSerial ezo(EZO_RX_PIN, EZO_TX_PIN);

char     cmd[40];
uint8_t  cmdLen = 0;
uint32_t lastCharAt = 0;

// If characters have arrived but no CR or LF follows within this window, the
// buffer is dispatched anyway. Without it, a monitor set to "No line ending"
// never sends anything at all and the sketch looks dead - you type, nothing
// happens, and there is no clue why. 600 ms is slower than anyone types a
// short command, and a split shows up plainly as two "> " lines.
const uint16_t CMD_IDLE_MS = 600;

char    resp[56];             // one response line, to spot "?I," replies
uint8_t respLen = 0;

// Interlock state: how many circuits answered the last `i`.
bool     infoAsked  = false;
uint8_t  infoCount  = 0;
uint32_t infoAskedAt = 0;

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(RAIL33_EN, OUTPUT);
  digitalWrite(RAIL33_EN, HIGH);          // the circuit needs power to talk

  ezo.begin(9600);                        // EZO UART default is 9600 8N1

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" EZO UART bridge"));
  Serial.println(F("=================================================="));
  Serial.println(F(" D6 (3.3 V sensor rail): ON"));
  Serial.println(F(" A4 = RX (EZO TX/SDA)   A5 = TX (EZO RX/SCL)"));
  Serial.println(F(" 9600 8N1 towards the circuit"));
  Serial.println();
  Serial.println(F(" ONE CIRCUIT AT A TIME - both share these two wires."));
  Serial.println(F(" Two interleaved value streams below = both connected."));
  Serial.println(F(" I2C,<n> is REFUSED until one `i` is answered by one"));
  Serial.println(F(" circuit, because with both wired it would give them the"));
  Serial.println(F(" same address and lock you out of both."));
  Serial.println();
  Serial.println(F(" Type:  C,0       stop the reading stream"));
  Serial.println(F("        i         who is connected? also unlocks I2C,<n>"));
  Serial.println(F("        I2C,99    switch pH circuit  -> 0x63"));
  Serial.println(F("        I2C,100   switch EC circuit  -> 0x64"));
  Serial.println(F("        L,0       LED off"));
  Serial.println();
  Serial.println(F(" Line ending: CR is ideal, but any setting works -"));
  Serial.println(F(" a command with no terminator is sent after 600 ms idle."));
  Serial.print(F(" Waiting "));
  Serial.print(RAIL_SETTLE_MS);
  Serial.println(F(" ms for the circuit to boot..."));

  delay(RAIL_SETTLE_MS);

  Serial.println(F(" Ready. Anything from the circuit appears below."));
  Serial.println(F("--------------------------------------------------"));
  digitalWrite(ledPin, LOW);
}

// True for the single-character info command, not for "I2C,..."
bool isInfoCmd(const char *s) {
  return (s[0] == 'i' || s[0] == 'I') && s[1] == 0;
}

void handleCommand() {
  Serial.print(F("> "));
  Serial.println(cmd);

  if (strncasecmp(cmd, "I2C,", 4) == 0) {
    if (!infoAsked || (millis() - infoAskedAt) > INFO_VALID_MS) {
      Serial.println(F("-- REFUSED. Send `i` first."));
      Serial.println(F("   `i` identifies which circuit is connected and"));
      Serial.println(F("   proves only one of them is. Without that check this"));
      Serial.println(F("   command could address both at once."));
      return;
    }
    if (infoCount == 0) {
      Serial.println(F("-- REFUSED. Nothing answered `i`."));
      Serial.println(F("   No circuit is talking, so do not change addresses."));
      return;
    }
    if (infoCount > 1) {
      Serial.print(F("-- REFUSED. "));
      Serial.print(infoCount);
      Serial.println(F(" circuits answered `i`."));
      Serial.println(F("   Both are on the bus. This command would set BOTH to"));
      Serial.println(F("   the same address and lock you out of each of them."));
      Serial.println(F("   Disconnect one circuit's SDA+SCL, send `i` again,"));
      Serial.println(F("   and only continue when exactly one answers."));
      return;
    }

    ezo.print(cmd);
    ezo.print('\r');
    Serial.println(F("-- sent. The circuit now reboots into I2C mode and goes"));
    Serial.println(F("   silent on UART. That is success, not failure."));
    Serial.println(F("   Verify with I2CScan: SDA driven-low must fall to the"));
    Serial.println(F("   same 10-20 mV as SCL."));
    infoAsked = false;                    // the interlock is single-use
    return;
  }

  if (isInfoCmd(cmd)) {
    infoAsked   = true;
    infoCount   = 0;
    infoAskedAt = millis();
    Serial.println(F("-- counting how many circuits answer..."));
  }

  ezo.print(cmd);
  ezo.print('\r');
}

void loop() {
  // circuit -> monitor, buffering each line so "?I," replies can be counted
  while (ezo.available()) {
    char c = ezo.read();

    if (c == '\r') {
      resp[respLen] = 0;
      if (respLen > 0) {
        Serial.println();
        if (infoAsked && strncmp(resp, "?I,", 3) == 0) {
          infoCount++;
          Serial.print(F("-- info reply #"));
          Serial.print(infoCount);
          if (infoCount > 1) {
            Serial.println(F("  <-- MORE THAN ONE CIRCUIT IS CONNECTED"));
          } else {
            Serial.println();
          }
        }
      }
      respLen = 0;
      continue;
    }

    if (c == '\n') continue;

    Serial.write(c);
    if (respLen < sizeof(resp) - 1) resp[respLen++] = c;
  }

  // monitor -> circuit, one whole line at a time
  while (Serial.available()) {
    char c = Serial.read();
    lastCharAt = millis();

    if (c == '\r' || c == '\n') {
      if (cmdLen == 0) continue;          // ignore the second half of CRLF
      cmd[cmdLen] = 0;
      handleCommand();
      cmdLen = 0;
      continue;
    }

    if (cmdLen < sizeof(cmd) - 1) cmd[cmdLen++] = c;
  }

  // Fallback for a monitor that sends no line ending at all.
  if (cmdLen > 0 && (millis() - lastCharAt) > CMD_IDLE_MS) {
    cmd[cmdLen] = 0;
    handleCommand();
    cmdLen = 0;
  }
}
