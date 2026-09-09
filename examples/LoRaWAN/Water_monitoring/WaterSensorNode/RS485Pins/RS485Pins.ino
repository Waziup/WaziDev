/*
 * ============================================================================
 *  RS485Pins  --  which board pin is the transceiver's OUTPUT?
 * ============================================================================
 *  Board : WaziSense v2 (ATmega328P)
 *  Serial monitor: 38400 baud (read-only). No sensor, no meter, no rewiring.
 *
 *  THE PROBLEM THIS SOLVES
 *  A 4-pin auto-direction RS485 module labels its two logic pins RXD and TXD,
 *  and the label means different things on different modules: either "wire
 *  the MCU's RXD/TXD here" (straight by label) or "the module receives and
 *  transmits here" (crossed by label). Get it the wrong way round and the
 *  MCU's TX output sits on the module's receiver OUTPUT: nothing ever drives
 *  the RS485 driver, the bus stays at its idle bias voltage, and every device
 *  looks silent on every baud rate and every address.
 *
 *  Symptom that points here: A-B measures the bias value (~268 mV) and does
 *  NOT collapse while the sketch transmits.
 *
 *  WHY NOT JUST SWAP THE WIRES AND SEE
 *  Because if they are already wrong, an AVR output is fighting the module's
 *  output. Two 3.3 V push-pull drivers through roughly 25 ohms each is about
 *  66 mA - above the ATmega328P's 40 mA absolute maximum per pin. Do not add
 *  a second guess on top of the first. Find out first, then wire it once.
 *
 *  HOW IT WORKS
 *  Same trick that found the missing I2C pull-up: discharge the pin, release
 *  it, and see whether anything pulls it back up.
 *
 *    driven HIGH again  ->  something is actively driving this pin.
 *                           With a valid idle bus (A > B) the module's
 *                           receiver output RO sits at logic HIGH. So this
 *                           pin is RO - it must go to the MCU's RX (D3).
 *
 *    stays LOW          ->  nothing drives it; it floats on its own pin
 *                           capacitance. This is the module's input DI -
 *                           it must go to the MCU's TX (D4).
 *
 *  A valid idle bus is a precondition, so the bias network has to be right
 *  first. Confirm with RS485Drive: A ~1.78 V, B ~1.52 V, A-B ~268 mV.
 * ============================================================================
 */

const int PIN_D3    = 3;
const int PIN_D4    = 4;
const int RAIL33_EN = 6;
const int ledPin    = 8;

const uint16_t RAIL_SETTLE_MS = 1200;

// Discharge, release, then wait this long before reading. A driven pin
// recovers in nanoseconds; a floating one holds its charge far longer.
const uint16_t RELEASE_SETTLE_US = 200;

// True if something pulls the pin back up after we let go of it.
bool isDriven(int pin) {
  uint8_t highs = 0;

  // Several passes, because one sample could catch a data edge if the module
  // happens to be receiving something at that instant.
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);            // discharge
    delayMicroseconds(50);

    pinMode(pin, INPUT);             // release, internal pull-up OFF
    digitalWrite(pin, LOW);
    delayMicroseconds(RELEASE_SETTLE_US);

    if (digitalRead(pin)) highs++;
    delay(2);
  }

  Serial.print(F("  D"));
  Serial.print(pin);
  Serial.print(F(": recovered HIGH in "));
  Serial.print(highs);
  Serial.print(F(" of 8 passes  -> "));
  if (highs >= 6) {
    Serial.println(F("DRIVEN by the module"));
    return true;
  }
  if (highs == 0) {
    Serial.println(F("floating (an input)"));
    return false;
  }
  Serial.println(F("inconclusive - see note below"));
  return false;
}

void setup() {
  Serial.begin(38400);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(RAIL33_EN, OUTPUT);
  digitalWrite(RAIL33_EN, HIGH);     // the transceiver needs its 3.3 V
  delay(RAIL_SETTLE_MS);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F(" RS485 logic-pin identification"));
  Serial.println(F("=================================================="));
  Serial.println(F(" 3.3 V rail ON. Bus left at its idle bias, so the"));
  Serial.println(F(" module's receiver output should read logic HIGH."));
  Serial.println(F("--------------------------------------------------"));

  bool d3 = isDriven(PIN_D3);
  bool d4 = isDriven(PIN_D4);

  Serial.println(F("--------------------------------------------------"));

  if (d3 && !d4) {
    Serial.println(F("CORRECT AS WIRED."));
    Serial.println(F(" D3 is on the module's output (RO) -> MCU receive"));
    Serial.println(F(" D4 is on the module's input  (DI) -> MCU transmit"));
    Serial.println();
    Serial.println(F(" So the pin mapping is not the fault. If A-B still does"));
    Serial.println(F(" not collapse while transmitting, the module itself is"));
    Serial.println(F(" not driving: suspect the module, not the wiring."));
  } else if (!d3 && d4) {
    Serial.println(F("REVERSED. D3 and D4 are the wrong way round."));
    Serial.println(F(" D4 sits on the module's OUTPUT, so your transmit pin"));
    Serial.println(F(" has been fighting it and nothing ever reached DI."));
    Serial.println(F(" That is exactly why the bus never left its bias level."));
    Serial.println();
    Serial.println(F(" FIX IT IN SOFTWARE - no rewiring needed. In every"));
    Serial.println(F(" sketch swap the two constants:"));
    Serial.println(F("     const int RS485_RX_PIN = 4;"));
    Serial.println(F("     const int RS485_TX_PIN = 3;"));
    Serial.println(F(" and in RS485Scan / RS485Drive likewise."));
  } else if (!d3 && !d4) {
    Serial.println(F("NEITHER PIN IS DRIVEN."));
    Serial.println(F(" The module's receiver output is not producing a level,"));
    Serial.println(F(" so either the module has no 3.3 V, its A/B are not"));
    Serial.println(F(" connected, or it is faulty. Check its VCC pin against"));
    Serial.println(F(" GND first - it should read 3.3 V while this runs."));
  } else {
    Serial.println(F("BOTH PINS APPEAR DRIVEN."));
    Serial.println(F(" That should not happen with one transceiver. Either"));
    Serial.println(F(" both wires land on the same module pin, or something"));
    Serial.println(F(" else is connected to D3/D4."));
  }

  Serial.println(F("--------------------------------------------------"));
  Serial.println(F(" Note on 'inconclusive': a pin that recovers on some"));
  Serial.println(F(" passes but not others is usually RO while the module is"));
  Serial.println(F(" receiving noise. Re-run with nothing else on the bus."));
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
