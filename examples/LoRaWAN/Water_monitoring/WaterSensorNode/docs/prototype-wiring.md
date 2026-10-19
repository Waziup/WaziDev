# Water Sensor Node — Wiring

Assembly reference for the KijaniSpace LoRaWAN water-quality station: five probes across three buses, two GPIO-gated supply rails, WaziSenseV2 controller.

Once it is wired, [Water Node Bring-Up](bring-up.md) is the companion runbook: which script to run in which order, and the measurable result that has to be true at each stage before the next one can be trusted. [Parts list](parts-list.md) has every component, its measured values, and the ones that were tried and rejected.

- SITE  **Lake Victoria / fish farm**
- MCU   **WaziSenseV2 (ATmega328P)**
- RADIO **SX1276 · EU868 · SF12**
- SKETCH **WaterSensorNode.ino**
- REV   **low-side rails · #2810 gating**

> [!NOTE]
> ### Status · 7 October 2026
>
> **There is now a carrier board.** Everything below describes the jumper-wire build that the node was brought up on, and it stays accurate for that build. The same circuit has since been laid out as a PCB: [`pcb/out/FACTS.md`](../pcb/out/FACTS.md) is generated from the board file itself and **wins over this guide wherever the two disagree**. *(This paragraph named `pcb/README.md` until 13 October 2026. That file is ordinary prose, is not generated, and had drifted.)* The [Sensor Carrier](sensor-carrier.md) page carries the reasoning behind it.
>
> On the carrier, what this guide calls `D6(−)` is the net `SWGND`, and it is wired as its own net rather than left to a ground pour. The four grounds — `GND`, `SWGND`, `ISO1_GND`, `ISO2_GND` — must never meet, and `PGND` is not a ground at all but the probe’s reference electrode.
>
> Corrected on this guide since it was written: the `D6(−)`–GND reading (Drawing 00), the pin named in three drawing descriptions, the isolators’ current draw (Drawing 08), and the transceiver’s identity — the chip is marked **MAX13487**, not MAX3485.

## DRAWING 00What "Sensor Power" actually is

Read this before any other drawing. The two **"Sensor Power"** blocks are not what the manual's phrase "Dedicated VCC Mosfet" suggests, and the difference decides where every sensor ground in this build has to land.

![The Sensor Power topology. A permanent 3.3 volt node, selected by jumper H1, feeds the plus terminals of both Sensor Power blocks; the two plus terminals are the same node and measure 1.2 ohms apart. Each block's minus terminal is the drain of its own low-side MOSFET whose gate is driven by D6 or D7](img/prototype-wiring-01.svg)

permanent 3.3 V switched return gate drive

> [!NOTE]
> ### How this was established, and what it cost
>
> **The plus terminals are one permanent node.** `D6(+)` to `D7(+)` measures **1.2 Ω**. Both sit on the 3.3 V that `H1` selects, and neither is ever switched. There is no `3V3` pin and no separate terminal strip on this board; those two plus terminals *are* the accessible 3.3 V.
>
> **The switch is in the minus leg.** On a bare board `D6(−)` to `D7(−)` and `D6(−)` to GND both read **high** — which is what an open MOSFET looks like. **Not infinite:** on this board `D6(−)`–GND measures about **47 kΩ**, and that is the healthy reading. An earlier version of this guide said “open”, which sends you hunting for a short that is not there. Each terminal pair has its own low-side device, gated by the matching GPIO.
>
> **So a sensor's return must go to `D6(−)` and to nothing else.** On this build the transceiver's ground was also tied to the common-ground bus, because that is the reflex when wiring RS485. It shorted the MOSFET: `D6(−)` to GND measured 2 Ω with the harness attached and open without it. The three devices and the RS485 bias never switched off, and sleep current sat at **20.78 mA** instead of **3.2 mA** — 14 hours from a 2500 mAh cell instead of three weeks. The firmware was correct the whole time.
>
> **Where the sleep current stands now.** 1.08 mA with the WaziSense wired straight to the cell; **1.46 mA** on the present bench build. Disconnecting the isolator’s `VOUT` drops it to 1.36 mA and the module’s LED goes out — that 0.1 mA is back-feeding through the LED’s ESD diodes, which is why an isolator supply that is switched off is not necessarily a branch that draws nothing. The remaining **0.28 mA is not yet explained**. **Measure sleep current with the FTDI unplugged**; it feeds the battery domain and hides exactly this.
>
> **Where it goes, as far as it is traced.** The manual puts the bare board at **29 µA** asleep with the solar circuit active and **6.5 µA** without, so the WaziSense is 2 % of the measurement and the search belongs elsewhere. The I²C pull-ups are the first real item: **in sleep, SDA and SCL measure 2.25 V, not 3.3 V**. Unpowered modules on the bus clamp the lines, and the pull-ups hang on the permanently live `D6(+)`, so (3.3 − 2.25) / 4.7 kΩ = **223 µA per line, 447 µA for the pair**, around the clock. That leaves about **984 µA unexplained**. The **SMBJ5.0A** was the prime suspect and has since been **cleared**: from a bench supply the sleep current went 4.2 V → 1.46 mA, 3.0 V → 1.17 mA, where a purely resistive load would have given 1.04 mA. Avalanche leakage falls away far faster than linearly, so the TVS can be at most ~0.4 mA of the total and is probably much less.
>
> **Beware of your own meter here.** Near the bottom of the cell the node bootlooped with the multimeter in series and ran correctly without it — **burden voltage**, the meter's shunt eating the headroom the 160 mA peak needs. Measure sleep current with the meter in voltage mode across a 1 Ω shunt, or on a 10 A range, not in a milliamp range.
>
> **The carrier board does not have the pull-up problem.** There, `R11`/`R12` are unfitted and the only pull-ups are the isolators’ own, fed from `ISO_VIN`, which `A1` switches. Pull-ups and bus go dead together and there is no DC path left to clamp. **Do not retrofit the external 4.7 kΩ resistors onto the carrier, and leave `I2C_J` open.**
>
> The bus reference RS485 needs is established *through* the conducting MOSFET during a measurement: tens of milliohms against a −7 to +12 V common-mode range. During sleep nothing drives the bus, so there is nothing to reference.

> [!WARNING]
> ### Checking it on your own board
>
> Power off, meter on resistance. `D6(+)`–`D7(+)` should read near zero; `D6(−)`–GND should read about **47 kΩ**. If the second one reads a few ohms with the harness connected and open without it, you have the stray ground wire described above — find it by lifting each device ground from `D6(−)` in turn until the reading goes open.
>
> Never measure resistance on a powered board. The meter injects its own voltage, and on a live node the reading is meaningless — it reported this board's working MOSFET as "open when ON, low impedance when OFF", which is simply what an ohmmeter does when it is fighting a driven node.

> [!NOTE]
> ### What the WaziSense v2 manual adds — and what it leaves out
>
> **There is no protection on the battery input. None.** The manual documents a **CN3791** MPPT charger (whose “battery overvoltage protection” is on the *charging* side only) and an **HE2031A33MPR** regulator with a built-in current limiter that protects the regulator. No fuse, no reverse-polarity element, no TVS. **The external PTC + SS34 + TVS stage is therefore not redundant — it is the only protection in the node.**
>
> **The regulator sets the floor, and `H1` cannot move it.** HE2031A33MPR: input **3.5–25 V**, dropout 0.18 V at 100 mA, quiescent **2.0 µA**, output >400 mA. The ATmega and the radio sit behind it *whatever position `H1` is in* — `H1` only selects what appears at the Sensor Power terminals. So moving it to BATT cannot keep the node running one millivolt lower; it only keeps the sensors powered after the MCU has already stopped.
>
> **And on this build, BATT would be actively harmful.** The manual does advise it with a solar panel (“the solar circuit’s highs and lows during operation affects transmissions”), but that was written for a bare board driving simple sensors. Here the Sensor Power rail is the carrier’s `3V3REG`, and the **RS485 module’s `VCC` is on it**. The module’s `RO` is a *push-pull* output wired straight to `D3` with no series resistor, so at BATT and a full cell it would drive **4.2 V into a pin rated VCC+0.5 = 3.8 V**, current-unlimited. The DS18B20’s `DQ` would do the same into `A2`, though through 4.7 kΩ. **Keep `H1` on 3.3 V.**
>
> Two smaller things from the same document: `I2C_J` ships *closed* and pulls `A4`/`A5` to the **regulated** 3.3 V (it was open here, 5.9 MΩ); and the manual asks for **at least 1500 ms** after a `digitalWrite(mosfet_pin, HIGH)` before reading a sensor — `RAIL12_SETTLE_MS` is 3000, so that is covered.

## DRAWING 01System overview

The node splits into two voltage domains, and **both of them are gated**. Everything the MCU talks to directly runs at **3.3 V**, switched by D6; the two RS485 probes need **10–30 V** and are fed from a 12 V converter whose *input* is switched by D5 — the switch comes first, so the converter itself is unpowered between measurements rather than merely disconnected from its load. Between measurements both rails are dead — including the RS485 fail-safe bias, which would otherwise draw current around the clock. The RS485 bus and the I²C bus stay independent: a fault on one does not take the other down.

![Block diagram: solar and battery feed a protection stage, which powers the WaziSense board and a boost converter. Two on-board low-side switches gate the return of a 3.3 volt sensor rail from pin D6 and, in this build, a Pololu MOSFET switch on the battery side gates the boost converter from pin D5.](img/prototype-wiring-02.svg)

+12 V switched 3.3 V / logic data (RS485, I²C) GND

***D5** gates the +12 V chain through a Pololu #2810; **D6** gates the 3.3 V sensor rail *on its return* — see Drawing 00, it is a low-side switch. **A1** gates the isolators, and only for the seconds the I²C reads need. The probes' ground runs unbroken from the battery, so the RS485 common reference is never interrupted. The two RS485 slaves share address `0x01` and are separated by baud rate; the two EZO circuits sit at distinct I²C addresses.*

---

## DRAWING 02RS485 bus detail

Both Modbus probes share one half-duplex pair. The MCU has a single hardware UART (taken by the USB debug console), so the bus runs on `SoftwareSerial` — pins D3/D4, and **nothing else**. The transceiver module fitted here is the **auto-direction** type: it breaks out only `RXD`, `TXD`, `GND` and `3.3 V`, and raises the driver-enable internally from activity on `TXD`. There is no `DE` wire to run, and D5 stays free. **One 120 Ω terminator only, at the transceiver** — see the calculation below for why a second one is wrong on this bus.

![RS485 wiring detail: the auto-direction transceiver module has only four pins — RXD, TXD, GND and 3.3 volts — wired to board pins D3 and D4 and the switched 3.3 volt rail. It has no direction-control pin. Its ground returns to the switched D6 minus terminal, not to the main ground rail. The A and B ](img/prototype-wiring-03.svg)

+12 V switched 3.3 V / logic RS485 A / B GND cable shield

***One 120 Ω** resistor across A/B at the transceiver. The far end of the cable is left **unterminated**. The **680 Ω** fail-safe bias pair holds the idle bus at a defined level so noise is not decoded as data; fit it once, at the master end. The shield connects to enclosure ground at **one end only** — grounding both ends creates a ground loop that injects noise into the pair. **Note where the lower 680 Ω returns:** `D6(−)`, not ground — see the note below the drawing.*

> [!NOTE]
> ### Why the lower 680 Ω returns to `D6(−)`
>
> Because `D6(+)` is *permanently* live. Returning the pull-down to a permanent ground closes a chain that nothing can switch off:
>
> 3.3 V / (680 + 120 + 680) Ω = **2.23 mA, continuously**
>
> That is not a rounding error on this node. The whole sleeping node measured **3.2 mA** at the time, and the WaziSense v2 is specified at **29 µA** asleep with its solar circuit active. The bias chain alone is about seventy times the board's own figure, and most of what was measured.
>
> **Returned to `D6(−)` it costs nothing and behaves identically while running.** With the rail on, the MOSFET holds `D6(−)` at zero volts and the divider below is unchanged — still 268 mV of idle bias from the external divider alone. With the rail off the chain has no destination: the switched node floats up to `D6(+)`, the differential across the chain is zero, and no current flows. That is the whole point of a low-side switch, and it is free.
>
> The same reasoning puts every other device ground on this rail at `D6(−)` — the transceiver's and the DS18B20's included. See Drawing 00.
>
> ### Why only one terminator on this bus
>
> Termination and fail-safe bias form one voltage divider, so they have to be chosen together. The idle differential voltage is
>
> U\_diff = 3.3 V × R\_term / (2 × R\_bias + R\_term)
>
> An RS485 receiver needs more than **200 mV** to see a defined idle state:
>
> | Configuration | R\_term seen by the bias | U\_diff idle |  |
> | --- | --- | --- | --- |
> | 2 × 120 Ω + 680 Ω bias | 60 Ω (parallel) | 139 mV | ✗ below threshold |
> | **1 × 120 Ω + 680 Ω bias** | 120 Ω | **268 mV** | ✓ used here |
> | 2 × 120 Ω + 390 Ω bias | 60 Ω (parallel) | 236 mV | ✓ valid alternative |
>
> **Why the second terminator is not needed anyway.** At 9600 baud one bit lasts **104 µs**, while a reflection on a 2 m stub has settled after roughly **20 ns** — a ratio of about 1:5000. The receiver samples mid-bit, long after any ringing has died away. Termination matters on multi-megabit buses; on a few metres at 9600 baud it buys nothing and only drags the bias level down.
>
> **Measured, after this was written.** Three readings settle it on the real hardware. The module’s own on-board terminator, jumper `120_S`, is **open** — 10.8 kΩ across A/B with the module unplugged and unpowered. The probe cable reads **116.5 Ω** with the external 120 Ω fitted and no sensors attached, so the probes do not terminate either. And with the module in circuit, `A`−`B` idles at **0.36 V** rather than the 268 mV above: the transceiver carries its own fail-safe bias, which adds to the external network. **One terminator exists on this bus and it is the external 120 Ω** — on the carrier board, `RT1`, fitted.
>
> So: **do not add a terminator at the probes.** If you ever raise the baud rate or extend the run substantially, fit the second 120 Ω *and* drop the bias resistors to ~390 Ω in the same step — never one without the other.

---

## DRAWING 03Power chain and protection

One battery domain, two output rails. The protection parts sit between the battery and everything else, so a surge coming down the solar cable or a reversed connector is stopped before it reaches the boost converter or the MCU.

![Power chain: solar panel through a TVS diode to the MPPT charge controller, then the 18650 battery, then a PTC fuse and SS34 Schottky diode with a TVS across the rail. The protected rail feeds the board directly at 3.7 volts and, on a second branch, a Pololu number 2810 MOSFET switch controlled by p](img/prototype-wiring-04.svg)

power path 3.3 V / control GND

*The **PTC** is resettable, so an overcurrent event does not need a field visit to replace a fuse. The **SS34** costs about 0.5 V of forward drop in exchange for reverse-polarity immunity — an acceptable trade at this power level. The 12 V branch is gated by a **Pololu #2810 MOSFET switch** driven from `D5`. Its `ON` pin carries an internal pull-down, so low or disconnected both mean off and no external gate resistor is needed. **Its mechanical slide switch must be in the OFF position**, or it overrides `ON` and the converter runs permanently.*

---

## TABLE 01Pin assignment — WaziSense v2

Taken from the *WaziSense v2 Internal User Manual* and matching the constants at the top of `WaterSensorNode.ino`. **D2, D9, D10, D11, D12, D13 belong to the RFM95W radio** and are not available; I²C is fixed to A4/A5 in hardware.

| Pin | Sketch constant | Connects to | Notes |
| --- | --- | --- | --- |
| `D3` | `RS485_RX_PIN` | RS485 module `RXD` | SoftwareSerial receive |
| `D4` | `RS485_TX_PIN` | RS485 module `TXD` | SoftwareSerial transmit |
| `D6` | `RAIL33_EN` | gate of the **"Sensor Power 1"** MOSFET | **LOW-SIDE switch — it gates `D6(−)`, not `D6(+)`.** See Drawing 00. Feeds the transceiver, the RS485 bias and the DS18B20. |
| `D7` | — | gate of the **"Sensor Power 2"** MOSFET — **and nothing else** | **There is no D7 pin on any header.** It exists only as the terminal pair, so it cannot carry a logic level to an external switch. The whole "Sensor Power 2" block is unused in this build. |
| `A2` | `ONE_WIRE_PIN` | DS18B20 `DQ` | **The temperature sensor in this build.** Needs an external 4.7 kΩ pull-up to `D6(+)` — this one you *do* fit. |
| `A0` | `batt_pin` | — (on board) | On-board 2 × 470 kΩ divider, **ratio measured as 2.00** — `BATT_DIVIDER` in the sketch. `VccCorrection` is 1.0: this chip's bandgap reads 3.3129 V untrimmed. |
| `A4` | — (`Wire`) | SDA → both EZO circuits | Pull-up **via `I2C_J` — verify with a meter**, it has been found open. ~4.7 kΩ to 3V3 = fine; megohms = fit your own to `D6(+)`. |
| `A5` | — (`Wire`) | SCL → both EZO circuits | Same check. Without a pull-up the bus does not merely fail — it **hangs** the Wire library. |
| `D8` | `ledPin` | — (on board LED1) | Boot blink pattern |
| `D11/12/13` | — (`SPI`) | RFM95W radio only | Nothing else shares SPI in this build. Re-enabling the PT1000 puts a MAX31865 here with `CS` on `A1`. |
| `D2 · D9 · D10` | — | RFM95W DIO0 · RESET · NSS | **Reserved — do not use** |
| `D5` | `RAIL12_EN` | Pololu #2810 (A) `ON` — the 12 V chain | Freed when the auto-direction transceiver removed the need for a `DE` line. The #2810 switches on above ~1 V, so 3.3 V is ample. |
| `A1` | `ISO_EN` | Pololu #2810 (B) `ON` — the pH isolator | Was reserved for the MAX31865 chip select; the PT1000 is not fitted. Analog pins are full digital outputs on this part. |
| `A3` | — | — free — | The only spare left. |

> [!NOTE]
> ### What the WaziSense manual changes
>
> **1. The I²C pull-ups are supposed to be on the board — verify that they are.** The manual describes two 4.7 kΩ resistors to 3.3 V, enabled by the `I2C_J` jumper and closed from the factory, and says you cut it only to use A4/A5 as plain analog inputs. **On the board actually built here it was open:** `A4` and `A5` both measured **5.9 MΩ** to 3V3, not 4.7 kΩ.
>
> That is worth knowing because a missing I²C pull-up does not degrade gracefully. The TWI hardware releases SCL and waits for it to rise before it can issue a START condition; with no pull-up it never rises, so the hardware waits forever. On AVR cores before 1.8.1 there is no timeout, so `Wire.endTransmission()` never returns and the sketch stops dead with nothing in the log. "The log just ends after the last sensor" is the signature.
>
> **So: measure, then decide.** ~4.7 kΩ means fit nothing — a second pair in parallel gives ~2.35 kΩ and overloads the bus. Megohms means fit **2 × 4.7 kΩ from `A4` and `A5` to `D6(+)`**. That terminal is permanently live, so the pull-ups stay powered while the circuits do not — which is harmless here, because a low-side switch lets each circuit's own ground float up to the same 3.3 V. No differential, no current, nothing fed into an unpowered chip.
>
> **2. D6 and D7 are LOW-SIDE switches, whatever the block diagram suggests.** The manual calls it a "Dedicated VCC Mosfet", which reads as high-side switching. It is not, and the difference cost days on this build — Drawing 00 and the measurements under it are the record. The catch about **H1 selecting the voltage for *both* blocks simultaneously** still holds, so they cannot be split between BATT and 3.3 V. **Set H1 to 3.3 V.** The transceiver decides it: 3.0–3.6 V is a hard limit for the part, and a full 18650 at 4.2 V sits outside it. D6 then carries the switched 3.3 V sensor rail, and D7 becomes a logic signal into the external 22 A module that gates the 12 V line.
>
> **2b. What H1 = 3.3 V costs — and how it was paid.** The boost converter needs a battery-voltage input, and with H1 on 3.3 V the "Sensor Power" terminals no longer carry one. Earlier revisions of this document therefore ran the converter continuously and switched only its 12 V output, through an opto module. **That is no longer the build.** A **Pololu #2810** now sits between `X1-1` and the converter's input, driven from **`D5`**. It cannot be driven from `D7`: that signal exists only inside the board, as the gate of the "Sensor Power 2" MOSFET, and is not brought out to any header. The converter is genuinely unpowered between measurements, the opto module is gone, and the board's own MOSFET never has to carry a battery rail — it switches only the 3.3 V sensor return.
>
> **3. Battery sensing is on-board.** A0 sits on a 2 × 470 kΩ divider; you do not add your own.

## TABLE 02Bus addresses

Every device on a shared bus needs a unique address — except that **these two do not have one.** Both RS485 probes ship as `0x01` and neither was reconfigured. They are separated by **baud rate** instead: the firmware switches the port to 4800 before addressing the oxygen probe and to 9600 before the turbidity probe, and a frame arriving at the wrong rate fails its CRC rather than being answered.

That is deliberate. Setting a Modbus address is a one-time operation needing a tool and a single-probe bus, and it is easy to get wrong in the field. Switching a UART's baud rate costs two lines of code and cannot be forgotten. The values below match the sketch — change either side to suit, but keep them in sync.

| Device | Bus | Address | Supply | Warm-up before read |
| --- | --- | --- | --- | --- |
| DS18B20 temperature | **1-Wire** | single device — index 0, no addressing | `D6(+)` | 750 ms conversion (12-bit) |
| DO — DFRobot SEN0680 | RS485 **@ 4800** | `0x01` | +12 V rail | **15 s** optics warm-up, then a readiness check on the probe's own temperature register |
| Turbidity — Y511-A | RS485 **@ 9600** | `0x01` | +12 V rail | **35 s** — wiper sweep plus optics; read every cycle |
| Atlas EZO-pH | I²C | `0x63` | 3.3 V | 900 ms per `R` |
| Atlas EZO-EC | I²C | `0x64` | 3.3 V | 900 ms per `R` |

## DRAWING 04Protection — how PTC, SS34 and SMBJ connect

Two parts sit **in series** in the positive line, one sits **in parallel** across the rail. Polarity of the two diodes is the whole point — fit either one backwards and you get no output at all, or no protection at all.

![Protection schematic. From the battery positive terminal, a PTC fuse and then an SS34 Schottky diode are wired in series, the diode cathode facing away from the battery toward the load. At the output node an SMBJ5.0A transient voltage suppressor is wired in parallel to ground, with its cathode facin](img/prototype-wiring-05.svg)

***Series** — PTC then SS34, in that order: the fuse closest to the energy source protects the diode behind it. **Parallel** — the SMBJ sits across the rail after the diode, so it clamps whatever reaches the boost converter and the MCU. If the SMBJ ever conducts hard, the PTC upstream trips, which is the intended chain of events.*

> [!NOTE]
> ### Getting the two diodes the right way round
>
> **SS34 (series).** The printed ring marks the cathode. It faces the *load*. Fitted backwards, nothing downstream powers up at all — an easy fault to spot. Test: with the battery connected the correct way, you should measure roughly battery voltage minus 0.5 V at `+VBATT`.
>
> **SMBJ5.0A (parallel).** The ring marks the cathode and faces *+VBATT*. Fitted backwards it becomes a plain forward-biased diode across your battery — it will conduct immediately, get hot, and trip the PTC. Test: in circuit and un-powered, a diode-mode meter should read open from `+VBATT` to GND in one direction and a diode drop in the other.
>
> **PTC.** Not polarised — fit it either way round.

---

## DRAWING 05Full power schematic

Discrete parts drawn as component symbols, ready-made modules as boxes with named pins. Every positive and negative conductor is drawn. **Wires that cross without a filled dot are not connected** — only a dot means a junction.

![Full power schematic in classic symbols. A photovoltaic panel feeds a charge controller with separate positive and negative wires, with a transient suppressor to ground on the positive line. The battery hangs between the positive node and ground. In series follow a PTC fuse and an SS34 diode to the ](img/prototype-wiring-06.svg)

positive negative / GND control / 3.3 V ● junction — crossing without a dot is *not* connected

*The two suppressors are drawn as avalanche diodes (the bent cathode bar) because that is what they are: reverse-biased and non-conducting until the rail exceeds their breakdown. The bulk capacitor is drawn polarised — straight plate positive, curved plate negative. The 12 V chain is gated at its *input*: a boost converter passes its input through to its output when disabled, measured here as 7.2 V on the rail with 2.836 V going in, so switching the output would have left the rail alive.*

---

## DRAWING 06Terminal-block view

The same circuit as Drawing 05, redrawn the way it is physically built: as blocks landing on two terminal strips. Use this one at the bench for routing and labelling — **X1** for the battery domain, **X2** for the switched 12 V / RS485 domain, **X3** for the switched 3.3 V domain.

![Detailed power schematic. Solar panel positive goes through a TVS node to the charge controller, then to the battery, then in series through a PTC fuse and an SS34 diode to the protected VBATT node. VBATT feeds the WaziSense board VIN and, through a Pololu number 2810 MOSFET switch, the U3V9F12 boos](img/prototype-wiring-07.svg)

positive (+) negative (−) / GND 3.3 V / control

*Series order on the positive side is fixed: **battery → PTC → SS34 → VBATT**. The PTC sits first so it protects everything behind it; the TVS sits after the diode so it clamps what reaches the boost input. The 12 V switch is a **Pololu #2810** and takes a single control wire from `D5`: its `ON` pin is referenced to the module's own ground, which is already the common one. **Set the module's slide switch to OFF** — in the ON position it ignores `ON` entirely and the converter runs day and night.*

---

## DRAWING 07Sensor bus fan-out — X2

Each RS485 probe takes **four separate conductors**: two for power, two for data. All eight land on the same four terminals of block X2.

![Terminal block X2 has four terminals: plus 12 volt switched, ground, RS485 A and RS485 B. Eight wires fan out from these four terminals, four to each of the two probes: dissolved oxygen and Y511-A turbidity. Temperature is not on this bus. No termination resistor is fitted at the probes; the single ](img/prototype-wiring-08.svg)

V+ (12 V) V− (GND) RS485 A / B

*Eight conductors total from X2 to the probes. **No terminator goes at the probes** — the single 120 Ω sits at the transceiver (Drawing 02). Because the probe leads are moulded pigtails they are electrically stubs whichever way you route them; at 9600 baud that is harmless, so bring them all back to X2 in the dry enclosure rather than building a junction near the water.*

---

## DRAWING 08I²C chain — EZO-pH and EZO-EC

**Two isolators are fitted; only the pH one is required.** A galvanic loop needs *two* grounded ends, so floating the pH side already opens it — an open loop does not become more open by cutting it a second time. The EC circuit is the *aggressor* in that pairing anyway: it drives the AC into the water and its own reading barely cares about the resulting potentials.

**Keep both, and drop the EC one first if anything ever forces a choice.** The pH pair draws **57 mA** and the EC pair **82 mA** at 3.3 V per the datasheet, and **130 mA was measured for the two together** — an earlier version of this guide gave 130 mA as the figure for *each*. The second one costs about 2 mA of daily average and takes autonomy from roughly 19 days to 14 — against a 5–10 W panel that is headroom, not a constraint. The better argument for removing it is not energy: *every* isolator on the bus is one more device that clamps `SDA`/`SCL` when unpowered, and one more floating ground that must never touch anything.

**Neither runs off `D6(+)`, and that part is not optional.** The two together draw **130 mA** measured — switching that onto the board's own 3.3 V regulator, the one feeding the ATmega, browns the MCU out at every rail switch-on. Its input is specified for 3.0–5.0 V, so it runs from the cell behind a **Pololu #2810**, gated by A1. Both isolators share that one switch — its 3 A rating makes 130 mA nothing.

**Feeding it from the raw cell is safe, and that was checked rather than assumed.** The isolated output was metered at **3.825 V** with *either* 3.0 V or 3.7 V on the input: the isolated side is regulated, so a cell that sags across its discharge does not drag the EZO supply down with it. The `#2810` in that path is there to switch, not to regulate.

**The isolated side carries its own I²C pull-ups.** That was an open question while this guide was first written and it is now settled — which is why `R11` and `R12` are left deliberately unfitted on the carrier board. Do not add pull-ups beyond the barrier.

**Known and accepted: the bus idles above the MCU’s supply.** Those MCU-side pull-ups go to the isolator’s own `VCC`, which is `ISO_VIN` — the raw cell behind the switch, up to 4.2 V. The ATmega’s absolute maximum on a pin is VCC+0.5 = **3.8 V**, so with a full cell `A4`/`A5` sit above it and the input clamp diodes conduct, limited by the isolator’s 4.7 kΩ to roughly **64 µA per line**. That is an order of magnitude inside the ATmega’s DC injection tolerance, it only happens during the ~2 s the isolators are powered, and the bench build has run this way for weeks. It is nevertheless outside absolute maximum on paper, so: **do not add a third device to that bus that pulls harder, and do not raise `ISO_VIN`.** This is a consequence of feeding the isolators from the cell, which is itself unavoidable — 130 mA cannot come from the MCU’s regulator.

*Building without isolation at all? Drawing 09 and Table 08b are that variant — use one set or the other, never both.*

![The I2C chain as built. Four shared nets leave the WaziSense board: SDA from pin A4, SCL from pin A5, permanent 3.3 volts from the D6 plus terminal, and the switched return to the D6 minus terminal. The EZO-EC takes all four directly, with no isolator. The pH channel instead runs through one Basic E](img/prototype-wiring-09.svg)

SDA / SCL +3.3 V switched GND ● junction — a crossing without a dot is *not* connected

***An unpowered isolator on a live bus is not a neutral state.** Its I²C lines are open-drain on both sides, and with no supply they pull `SDA` and `SCL` down — which takes the EZO-EC with them even though EC has nothing to do with the isolator. That was measured here: with the isolator's cell disconnected, *both* circuits reported FAIL; with it connected, both answered. That is why A1 and the I²C traffic are now one window: the sketch raises A1, waits 1.5 s for the EZO circuits to boot, reads pH and EC, and drops it again — about four seconds instead of the fifty the cycle lasts, which is worth roughly 100 mAh a day. While they are off the bus sits clamped, and nothing minds, because nothing else is on it. The pH probe's **PGND** lands in the isolated domain, not on `X1-2` — bonding it to enclosure ground undoes the isolation you just paid for. Pin names differ between isolator boards; match them to your silkscreen and keep Table 08 as the reference.*

> [!NOTE]
> ### EZO circuits arrive in UART mode, not I²C
>
> This is the step that costs people an evening. **Every EZO circuit ships configured for UART.** Wired onto an I²C bus in that state it is simply silent — no ACK, no error, nothing that distinguishes it from a broken solder joint or a dead board. Switch each circuit over *before* it goes anywhere near the isolator.
>
> **Hardware route.** Atlas documents this in every EZO datasheet under "Manual switching to I2C": power the circuit down, disconnect the data lines, short the `PRB` pad to `TX`, power up, wait for the LED to change colour, power down, remove the short. Follow the steps in *your* circuit's datasheet rather than from memory — pad naming and LED colours differ between EZO generations.
>
> **Software route.** With a USB-serial adapter at `9600 8N1`, send `I2C,99` to the pH circuit and `I2C,100` to the EC circuit. Each switches immediately and stops answering on UART, so there is no going back without the hardware procedure.
>
> **The LED tells you which mode a circuit is in.** **Green = UART**, **blue = I²C** — that is the colour change the switching procedure asks you to wait for. A *blinking* green is worse than a steady one: it means the circuit is in the factory continuous-streaming mode and is transmitting about once a second. Because the EZO's `TX` pin *is* one of the two bus lines, a single circuit left in UART mode drags the clock line down at every burst and jams I²C for everything else. Both circuits must show blue before you enable `ENABLE_PH` / `ENABLE_EC`. Confirm the colours against your own datasheet's LED table — they differ between EZO generations. Once the node is deployed, turn the LEDs off with `L,0`: a few mA each is worth having back on a solar node.
>
> 99 and 100 decimal are `0x63` and `0x64` — the factory I²C defaults, already distinct, and already what the sketch expects. There is no address to change and no conflict to resolve. **Do one circuit at a time**, with only that circuit connected.
>
> **While the UART is open anyway**, turn off EZO-EC's extra output fields: `O,TDS,0`, `O,S,0`, `O,SG,0`. By default it answers `EC,TDS,SAL,SG` as CSV and the sketch's `atof()` silently takes the first field. That happens to be the one you want, so it works either way — but a single-value response is one less thing to reason about when a reading looks wrong.

---

## DRAWING 09I²C chain *without* isolation — variant

The same two circuits with the isolators left out. The wiring gets simpler: both EZO boards hang straight off A4, A5, `D6(+)` and `X1-2`, and the netlist loses eight wires. What you buy with that saving is the loop drawn in red below — **read this drawing for the loop, not for the wiring**, because the wiring is the easy part.

![The non-isolated variant. Both EZO circuits connect directly to board pins A4 and A5 and to the D6(+) and X1-2 supply terminals, with no isolation barrier anywhere. A closed interference loop is highlighted in red and numbered in four steps: the EC electrodes drive alternating current into the water](img/prototype-wiring-10.svg)

SDA / SCL +3.3 V switched GND —— the interference loop

*Compare this against Drawing 08: the wiring is **strictly simpler** — nothing but four shared nets and two probe connections — and that is the entire attraction. The cost is that `ISO_GND` no longer exists: both EZO grounds *are* `X1-2`, so the path from the EC electrodes back to the pH reference is closed through the board. The pH circuit measures ~59 mV per pH decade at an electrode impedance of 100 MΩ to 1 GΩ, which is why it is the victim and EC is not.*

> [!NOTE]
> ### Running it this way on purpose
>
> This variant is a legitimate starting point — you have to wire it anyway to test it, and the isolator drops in later without touching the probe side or the layout. Two things make it as good as it can be:
>
> **Order the reads.** pH before EC, always. The sketch already does this in `readAllSensors()`, so there is nothing to change — just do not reorder them.
>
> **Put EC to sleep for the pH read.** Send `Sleep` to `0x64`, read pH, then wake EC with any command and read it. That removes the driven AC term — step ① above — which is the dominant one.
>
> **What stays.** Even unpowered, the EC electrodes remain galvanically tied to `X1-2`, so the loop still exists; it is simply no longer driven. What survives is a DC galvanic offset between dissimilar metals in the water — Ag/AgCl against graphite or steel. Smaller than the AC term, and only real isolation removes it.
>
> **Decide it with a measurement, not a guess.** Both probes in one beaker at roughly 100 µS/cm (dilute tap water — neat tap water is often 300–600 µS/cm and an unfairly harsh test). Take 20 pH readings in each of three states: EC probe *out* of the water; EC probe in, circuit asleep; EC probe in, circuit measuring in a tight loop. The first-to-second gap is the galvanic offset, the second-to-third gap the AC term. Under ~0.05 pH total, ignore it. Over ~0.1 pH, fit the isolator.
>
> **One rule either way:** calibrate in the same configuration you measure in. Calibrating unisolated and then fitting an isolator (or the reverse) folds the offset into the calibration, where it is much harder to see.
>
> **Both EZO circuits share `X3`, and that is not a limitation here.** The obvious thought is to cut power to the EC circuit while pH is read — impossible, since one `D6` gates the whole rail. It also would not help: unpowered, the EC electrodes stay galvanically tied to `X1-2` through the dead front-end, so step ④ survives either way. Power-down and `Sleep` both remove the drive and neither breaks the loop, and `Sleep` is the better of the two because it wakes on the next transaction instead of costing another ~1 s boot inside every measurement window. In I²C mode the circuit cannot initiate anything at all, so reading pH before the EC circuit is ever addressed is already most of the answer — the continuous-streaming default in the datasheet is a UART feature and does not apply. If you ever *did* want the EC supply switched separately, `A3` is the last free pin and a small high-side load switch in its `VCC` leg costs about €2. It is not worth fitting.

---

## TABLE 03Netlist — power generation and protection

Two terminal blocks of your own: **X1** = battery domain (1 = +VBATT, 2 = GND), **X2** = switched 12 V and RS485 (1 = +12 V, 2 = GND, 3 = A, 4 = B). The 3.3 V domain needs no strip — it lands directly on the board's own `D6(+)` and `D6(−)` terminals.

**`D6(+)` and `D6(−)` are the two screw terminals of the "Sensor Power 1" block**, and they behave very differently — see Drawing 00. `D6(+)` is permanently live; `D6(−)` is the switched node. Neither is the D6 *pin*, which is the GPIO driving the MOSFET gate inside the board; that pin appears on the DIGITAL PINS header and is where a logic-level control signal belongs.

| Net | From | To | Wire | Note |
| --- | --- | --- | --- | --- |
| `SOL+` | Solar panel **+** | Charge controller `SOLAR+` | red 0.75 mm² | TVS to GND at this node |
| `SOL−` | Solar panel **−** | Charge controller `SOLAR−` | black 0.75 mm² | common with GND |
| — | TVS cathode (banded end) | `SOL+` | — | anode → GND |
| `BAT+` | Charge controller `BAT+` | Battery pack **+** | red 0.75 mm² |  |
| `BAT−` | Charge controller `BAT−` | Battery pack **−** | black 0.75 mm² | = GND star point |
| `VB1` | Battery **+** | PTC terminal 1 | red | PTC first in the chain |
| `VB2` | PTC terminal 2 | SS34 **anode** | red | diode direction matters |
| `VBATT` | SS34 **cathode** (banded) | `X1-1` | red | protected rail |
| — | SMBJ5.0A cathode | `X1-1` | — | anode → `X1-2` |
| `GND` | Battery **−** | `X1-2` | black | single star point |

## TABLE 04Netlist — board and 3.3 V domain

| Net | From | To | Wire | Note |
| --- | --- | --- | --- | --- |
| `VBATT` | `X1-1` | WaziSense `VIN` | red |  |
| `GND` | `X1-2` | WaziSense `GND` | black |  |
| `3V3P` | `D6(+)` | transceiver, RS485 bias, DS18B20, I²C pull-ups | orange | **Permanently live.** Not switched by `D6` — Drawing 00. |
| `3V3RTN` | those devices' **GND** | `D6(−)` | black | **This is the switched node, and the only place these grounds may land.** A second wire from here to any GND pin shorts out the MOSFET. |
| `C_BULK` | 470 µF electrolytic **+** | `D6(+)` | — | **−** to a GND pin. Across the *rail*, never across the terminals — on the terminals it would add to the inrush instead of absorbing it. Fitting this stopped a cold-start brownout outright. |
| `C_FAST` | 10 µF X7R + 100 nF ceramic | `D6(+)` → GND pin | — | Short leads. The electrolytic's ESR cannot supply the first microseconds of a load step; ceramic can. An extra electrolytic is *not* a substitute. |
| `BOOT_PU` | 10 kΩ | `RXI` → `D6(+)` | — | **Fit this before deploying.** With no FTDI attached the bootloader's receive pin floats, picks up phantom start bits and waits for an upload forever. Symptom: the node dies after a reset, the RESET pin does nothing, and only removing power revives it. |
| `VSENSE` | — nothing to wire — | WaziSense `A0` | — | **on-board** 2 × 470 kΩ divider (`BATT%` jumper). Do not add your own. |
| `ANT` | WaziSense antenna port | SMA bulkhead → λ/2 dipole | coax pigtail | seal the outside joint with self-amalgamating tape |

## TABLE 05Netlist — 12 V rail and gating

| Net | From | To | Wire | Note |
| --- | --- | --- | --- | --- |
| `VBATT` | `X1-1` | #2810 (A) `VIN` | red | the switch goes **before** the converter, so the converter is truly off between measurements |
| `GND` | `X1-2` | #2810 (A) `GND` | black |  |
| `V12IN` | #2810 (A) `VOUT` | U3V9F12 `VIN` | red |  |
| `GND` | `X1-2` | U3V9F12 `GND` | black |  |
| `V12SW` | U3V9F12 `VOUT` | `X2-1` | red | fixed **12 V**, ±4 % |
| `GND` | `X1-2` | `X2-2` | black | the probes' return — this one really is unswitched |
| `CAP+` | 1000 µF **+** | `X2-1` — the U3V9F12’s `VOUT` | red | Polarity — reversed electrolytics vent. This is a **second** capacitor: the 470 µF in Table 04 sits on the 3.3 V rail and must stay there, or the cold-start brownout returns. |
| `CAP−` | 1000 µF **−** | `X2-2` | black |  |
| `RAIL12_EN` | WaziSense **`D5`** | #2810 (A) `ON` | white | **Not D7 — that pin is not brought out.** The #2810 turns on above ~1 V; 3.3 V is ample, and its 3 A rating is far above the ~400 mA the converter draws. **Its slide switch must be in the OFF position**, or it overrides the `ON` pin and the converter runs permanently. |
| **Set the output to 12 V, not 14 V.** The 14 V in earlier revisions of this document was a workaround for a failing converter — the same unit later put out **30 V from a 3.6 V input**, which is inside the SEN0680's 10–30 V window and well outside the Y511-A's. The turbidity probe went silent while the DO probe kept answering, and that pattern — a fault that tracks the two sensors' voltage ratings — is a supply fault, not a protocol one. A converter that holds 12 V under load does not sag under the wiper motor, and 12 V is what both probes are specified for. |
| **The opto module is gone.** It existed only because a boost converter passes its input through to its output when disabled, via the inductor and diode — measured here as 7.2 V on the rail with 2.836 V at the input. Switching the converter's *input* removes that path completely, so the series switch behind it has nothing left to do. |

## TABLE 06Netlist — RS485 transceiver

Four wires to the module, four out of it. This is an **auto-direction** board — there is no `DE` or `RE` pin to wire, and the sketch has no direction-control code.

| Net | From | To | Wire | Note |
| --- | --- | --- | --- | --- |
| `RS_RX` | WaziSense `D3` | Module `RXD` | green | SoftwareSerial RX |
| `RS_TX` | WaziSense `D4` | Module `TXD` | blue | SoftwareSerial TX |
| — | **No `DE` / `RE` wire.** The module breaks out only `RXD`, `TXD`, `GND` and `3.3 V`, and switches the driver internally from activity on `TXD`. That is what freed `D5`, which now gates the 12 V chain. |
| `3V3SW` | `D6(+)` | Module `3.3 V` | orange | **3.3 V part — 3.0–3.6 V is a hard limit.** Never feed it from the battery rail. |
| `3V3RTN` | Module `GND` | `D6(−)` | black | **Not `X1-2`.** This device sits on the switched rail, so its return is the switched node. A wire from here to any GND pin shorts the MOSFET out and the rail never switches off — that cost 20.78 mA of sleep current on this build. See Drawing 00. |
| `RS485_A` | Module `A` | `X2-3` | yellow |  |
| `RS485_B` | Module `B` | `X2-4` | green |  |
| — | 120 Ω | `X2-3` ↔ `X2-4` | — | **the only terminator** — none at the probes |
| — | 680 Ω | `X2-3` → `D6(+)` | — | fail-safe bias, pull-up — matched to a single 120 Ω |
| — | 680 Ω | `X2-4` → `D6(−)` | — | **To `D6(−)`, not `X1-2`. This one wire is worth 2.2 mA.** The pull-up sits on the permanently live `D6(+)`, so a pull-down on the permanent ground closes a chain that never opens: 3.3 V / (680 + 120 + 680) Ω = **2.23 mA, day and night, sleep included** — against a measured 3.2 mA sleep current for the whole node. Returned to `D6(−)` instead, the chain has no path while the MOSFET is open: the switched node simply floats up to `D6(+)`, no differential, no current. Idle bias while running is unchanged at 268 mV. This is what the overview on Drawing 01 already claims happens. |
| — | Cable shield | Enclosure ground | drain wire | **enclosure end only**, far end open |

> [!NOTE]
> ### The one thing an auto-direction module costs you
>
> Losing the `DE` wire is convenient, but it moves a decision from software into a circuit you cannot see. Two designs are sold under the same four-pin pinout:
>
> **Receiver permanently enabled.** `RE` is tied to ground, so while the module transmits it also hands your own frame straight back to `RXD`. Every Modbus request is followed by an 8-byte echo before the slave's reply arrives, and the master has to swallow it.
>
> **Receiver muted while transmitting.** `RE` follows `DE`, no echo appears, and swallowing 8 bytes that never come would eat the start of the real reply instead.
>
> Both are common and the silkscreen rarely says which you have, so the sketch exposes it as a single flag: `#define RS485_ECHOES_OWN_TX 1`. Establish it once on the bench — if every read times out or the log fills with "bad header", flip the flag and try again. This is the *only* difference between the two module types as far as the firmware is concerned.
>
> **If replies come back truncated** rather than absent, the module's turnaround RC is holding the driver on into the first bits of the response. At 9600 baud that is unlikely, but the fix is a module with `DE` broken out. `D5` is no longer free for that, so it would mean moving the 12 V switch to `A3`.

## TABLE 07Netlist — probes on the bus

Repeat these four wires for **each** of the two RS485 probes — eight conductors in total. Temperature is **not on this bus** — the DS18B20 has its own 1-Wire line, see Table 09.

| Net | From | To — DO (4800) | To — Y511-A (9600) |
| --- | --- | --- | --- |
| `V12SW` | `X2-1` | probe `V+` | probe `V+` |
| `GND` | `X2-2` | probe `V−` | probe `V−` |
| `RS485_A` | `X2-3` | probe `A` | probe `A` |
| `RS485_B` | `X2-4` | probe `B` | probe `B` |
| — | termination | **none at the probes** — the single 120 Ω sits at the transceiver (Table 06) |

## TABLE 07bNetlist — PT1000 via MAX31865 (SPI) *— alternative, not fitted*

**This is not the temperature path in the current build.** The node uses a DS18B20 on 1-Wire instead — see **Table 09**. Keep this table for the upgrade: the PT1000 probe is already on hand, only the MAX31865 board is missing.

The probe supplied is a **bare 3-wire RTD** (2 × red + 1 × white), not a Modbus device, so it gets its own front-end on the SPI bus. The RTD itself needs no supply — it is a resistor. That also means it does *not* belong on the RS485 bus, and it does not take a Modbus address.

**Order the PT1000 board, not the PT100 board.** The two are physically identical and differ only in the reference resistor: **Adafruit #3648** carries the 4300 Ω (marking `4301`) that a PT1000 needs, **#3328** carries 430 Ω for a PT100. DigiKey stocks #3648 as `1528-2432-ND` at roughly $15, and RS Components carries it too — both ship into Germany. Fitting the wrong one puts every reading out by about a factor of ten, and no software setting rescues it.

| Net | From | To | Wire | Note |
| --- | --- | --- | --- | --- |
| `3V3SW` | `D6(+)` | MAX31865 `VIN` | orange | 3.3 V part, matches WaziSense logic |
| `GND` | `X1-2` | MAX31865 `GND` | black |  |
| `SCK` | WaziSense `D13` | MAX31865 `CLK` | short lead | shared with the radio |
| `MISO` | WaziSense `D12` | MAX31865 `SDO` | short lead | shared with the radio |
| `MOSI` | WaziSense `D11` | MAX31865 `SDI` | short lead | shared with the radio |
| `RTD_CS` | WaziSense `A1` | MAX31865 `CS` | white | its own chip select |
| `RTD_A` | PT1000 **red** | terminal at the paired end (1 of 2) | — | Set the breakout's solder jumper to **3-wire**. The two same-colour leads go to the two terminals at one end, the odd-coloured lead to the other. |
| `RTD_A2` | PT1000 **red** | terminal at the paired end (2 of 2) | — |
| `RTD_B` | PT1000 **white** | terminal at the single end | — |

> [!NOTE]
> ### Two ways to get zero readings out of a perfectly good RTD
>
> **Reference resistor.** `RREF` must be **4300 Ω** for a PT1000. Breakouts sold for PT100 carry **430 Ω** — with that fitted the numbers are wrong by roughly a factor of ten and no amount of software will fix it. Either buy the PT1000 variant or swap the resistor, and set `RREF 4300.0` / `RNOMINAL 1000.0` in the sketch to match.
>
> **Shared SPI, different modes.** The RFM95W runs SPI mode 0, the MAX31865 mode 1, on the same three wires. The Adafruit driver wraps its access in an SPI transaction so the two coexist — but if temperatures come back as nonsense *while LoRa still works*, a stuck SPI mode is the first thing to suspect. Verify with the multimeter first: **red ↔ red ≈ 0 Ω**, **red ↔ white ≈ 1078 Ω at 20 °C**.
>
> **And a third, specific to the gated rail.** The MAX31865 now sits on the switched 3.3 V, so it loses its configuration register every cycle. `rtd.begin(MAX31865_3WIRE)` therefore runs inside `readPT1000()` after the rail comes up, *not* once in `setup()` — initialising it in `setup()` would configure a chip that is not powered yet, and every reading afterwards would be a fault code.

## TABLE 08Netlist — I²C, isolators and EZO circuits

**As built: two isolators, though only the pH one is needed.** Wire this block once per isolator. If you are building fresh, fit the pH one and skip the EC one — floating one end of the loop is what opens it. Table 08b remains the variant for no isolation at all.

**And it is powered from the battery, not from `D6(+)`.** That is the part worth reading twice; the note below has the measurements behind it.

> [!NOTE]
> ### The isolators are a separate purchase — and one of the two is mandatory
>
> EZO circuits do not ship with isolation. You need **one isolator per channel you want protected**, and the two channels are not equally at risk:
>
> **pH — mandatory.** Atlas's own pH datasheet is explicit: the circuit is sensitive enough to read micro-voltages bleeding into the water from other equipment, "which is why the power and data lines need to be electrically isolated". A pH channel sharing a ground with an EC probe in the same water is the textbook case.
>
> **EC — optional.** The EC circuit is the *aggressor*, not the victim: it drives AC excitation current through the water, and its own reading barely cares about the resulting potentials. Isolating the pH already breaks the galvanic loop between the two. A second isolator on EC is belt-and-braces, not a prerequisite.
>
> Nothing else in this build adds a galvanic path through the water — the DO probe is a sealed optical RS485 device and the DS18B20 is a sealed digital probe, so neither electrode-couples to the pH cell. The same held for the PT1000 it replaces.
>
> **Which part.** The **Basic EZO Inline Voltage Isolator (BE-IVI)** is the one this drawing assumes: 14 × 20 mm, 3.0–5.0 V in, an isolated 3.9 V out, UART/I²C/SMBus to 1 MHz, ~20 mA at 3.3 V. It sits purely between the MCU and the EZO circuit and *does not touch the probe connection at all*.
>
> **Watch the probe connector if you buy a carrier board instead.** The **Electrically Isolated EZO Carrier Board (ISCCB-2)** bundles carrier + isolation on one 56 × 32 mm board, but its probe connector is a **female SMA**. Atlas pH probes are normally **BNC**, so that route can need an adapter the inline isolator never asks for. Check your own probe's connector before ordering.
>
> **Budget the current — it is larger than the datasheet suggests.** Measured on this build: **97.1 mA** with the isolator and EZO-pH connected against **15 mA** without, so the pair pair draws **130 mA** measured, the two together. The datasheet's figure for isolator-plus-pH is 57 mA at 3.3 V. Either way it dwarfs everything else on that rail, which carried about 15 mA before.
>
> **That is why it does not go on `D6(+)`.** Switching a 130 mA load with its own DC-DC converter onto the board's 3.3 V regulator — the same regulator that feeds the ATmega — browns the MCU out at every rail switch-on, reproducibly, with or without USB attached. Ground the isolator permanently and the brownout disappears; put it back on the switched return and it comes back.
>
> Two fixes were tried and rejected by arithmetic. A series resistor big enough to limit the inrush drops 1.3 V at 130 mA and is unusable. The `EN` pin shuts the isolator to 3.8 mA rather than zero, and driving it from a GPIO while the isolator's own ground floats would put `EN` below that ground.
>
> **The isolator's input is specified for 3.0–5.0 V, so it runs straight off the cell.** A second Pololu #2810 gates it from `A1`. The 130 mA and its inrush then never touch the MCU's supply at all, which is a structural fix rather than a damped symptom — and the sleep current stays at zero for that branch.

| Net | From | To | Wire | Note |
| --- | --- | --- | --- | --- |
| `VBATT` | `X1-1` | #2810 (B) `VIN` | red | the isolator branch, off the cell |
| `ISO_EN` | WaziSense **`A1`** | #2810 (B) `ON` | white | Slide switch OFF here too. **`A1` must be high before `D6` and go low after it** — see the caption on Drawing 08. |
| `ISO_VIN` | #2810 (B) `VOUT` | Isolator `VCC` (MCU side) | red | 3.0–5.0 V input range covers a full and an empty cell |
| `3V3RTN` | Isolator `GND` (MCU side) | `D6(−)` | black | **Not `X1-2`, even though the supply comes from the cell.** Measured: moving this one wire to board ground **added 0.4 mA of sleep current**. The I²C pull-ups sit on the permanently live `D6(+)` — unavoidable with low-side switching, since there is no switched positive rail on the MCU side. With the isolator unpowered and its ground held at 0 V, those pull-ups drive current in through its ESD clamps. Returned to `D6(−)` the ground floats up with them and the differential is zero. **General rule: every device on the MCU-side I²C bus returns to `D6(−)`, whatever supplies it.** |
| `EN` | Isolator `EN` | — leave open — | — | open means enabled; the #2810 is the switch |
| `SDA` | WaziSense `A4` | Isolator `SDA` (MCU side) | blue | both isolators share this net |
| `SCL` | WaziSense `A5` | Isolator `SCL` (MCU side) | white | both isolators share this net |
| — | **I²C pull-ups: measure first, do not assume.** The manual says WaziSense v2 carries 2 × 4.7 kΩ to 3.3 V via the `I2C_J` jumper, closed from the factory — **but this has been found OPEN on a real board** (`A4`/`A5` to 3V3 measured 5.9 MΩ). Check it: `A4`–`3V3` and `A5`–`3V3` must read **~4.7 kΩ**. If they read megohms, fit your own **2 × 4.7 kΩ from `A4` and `A5` to `D6(+)`**. If they read 4.7 kΩ, fit nothing — a second pair in parallel gives ~2.35 kΩ and overloads the bus. |
| `ISO_VCC` | Isolator `VCC` (isolated side) | EZO `VCC` | short lead | floating supply |
| `ISO_GND` | Isolator `GND` (isolated side) | EZO `GND` | short lead | **must not touch `X1-2`** |
| `ISO_A` | Isolator `data A` (isolated side) | EZO `TX` | short lead | **The EZO board is silkscreened `TX`/`RX`, not `SDA`/`SCL`** — it ships in UART mode and those same two pins carry I²C afterwards. The isolator labels both channels "RX, TX, SDA, SCL" for the same reason. Carry `A4` and `A5` straight through: **A→A, B→B**. Which one is SDA does not matter; swapping them does. |
| `ISO_B` | Isolator `data B` (isolated side) | EZO `RX` | short lead |
| — | **Pull-ups on the isolated side.** The ADM3260 inside the isolator has open-drain I²C on *both* sides. Power the isolator with the EZO unplugged and measure the isolated data lines against **isolated GND**: ~3.9 V means they are fitted, near zero means add **2 × 4.7 kΩ to the isolated 3.9 V**. Get this wrong and `Wire` hangs with no message at all. |
| `PH_IN` | pH probe BNC centre | EZO-pH probe `IN` | coax core | keep this lead short |
| `PH_SHD` | pH probe BNC/SMA shell | EZO-pH `PGND` | coax shield | **`PGND` is the reference electrode, not a ground.** Never bond it to `X1-2`. Identify the connector board's pads by continuity to the socket's centre pin and shell rather than by its silkscreen. |
| — | **`PRB` is a gigaohm source.** Keep the lead to centimetres, clean the flux off completely, and do not touch the pad — a fingerprint is a few hundred megohms and shifts the reading visibly. |
| `EC_1` | EC probe conductor 1 | EZO-EC probe pin 1 | — | EC probe leads are not polarised |
| `EC_2` | EC probe conductor 2 | EZO-EC probe pin 2 | — |  |

## TABLE 08bNetlist — I²C *without* isolation (variant)

Use this **instead of** Table 08, not alongside it. **Twelve wires instead of twenty** — eight MCU-side taps plus four probe conductors, against the isolated version's eight MCU-side, eight isolated-side and four probe. The probe connections are identical either way, so fitting an isolator later means inserting one module into the four MCU-side nets of that channel and nothing else. See Drawing 09 for what you are accepting in exchange.

| Net | From | To | Wire | Note |
| --- | --- | --- | --- | --- |
| `3V3SW` | `D6(+)` | EZO `VCC` — **both** | orange | EZO circuits accept 3.3–5 V |
| `GND` | `X1-2` | EZO `GND` — **both** | black | **the shared node the loop closes through.** There is no `ISO_GND` in this variant. |
| `SDA` | WaziSense `A4` | EZO `SDA` — **both** | blue |  |
| `SCL` | WaziSense `A5` | EZO `SCL` — **both** | white |  |
| `SDA_PU` | 4.7 kΩ | `A4` → `D6(+)` | — | **Only if `I2C_J` is open** — measure `A4`/`A5` to 3V3 first, as in Table 08. Megohms means no pull-up and I²C cannot work at all: the TWI hardware waits for SCL to rise before it can even issue a START, so a missing pull-up hangs the Wire library outright. |
| `SCL_PU` | 4.7 kΩ | `A5` → `D6(+)` | — |
| `PH_IN` | pH probe BNC centre | EZO-pH probe `IN` | coax core | keep this lead short |
| `PH_SHD` | pH probe BNC shell | EZO-pH probe `GND` | coax shield | in this variant that *is* `X1-2` — the difference from Table 08 is here |
| `EC_1` | EC probe conductor 1 | EZO-EC probe pin 1 | — | EC probe leads are not polarised |
| `EC_2` | EC probe conductor 2 | EZO-EC probe pin 2 | — |  |

Everything else in this document is unchanged: the rail gating on `D6`, the ~1 s EZO boot time in `RAIL33_SETTLE_MS`, and the UART → I²C protocol switch each circuit still needs before it will answer at all.

## TABLE 09Netlist — DS18B20 temperature *— fitted*

**This is the temperature sensor in the current build.** Four wires, no front-end board, no chip select, nothing on SPI. Water temperature lands on **uplink channel 1** either way, so swapping back to the PT1000 later needs no change on the WaziCloud side.

| Net | From | To | Wire | Note |
| --- | --- | --- | --- | --- |
| `3V3SW` | `D6(+)` | DS18B20 `VDD` | red |  |
| `3V3RTN` | DS18B20 `GND` | `D6(−)` | black | **Not `X1-2`.** This device sits on the switched rail, so its return is the switched node. A wire from here to any GND pin shorts the MOSFET out and the rail never switches off — that cost 20.78 mA of sleep current on this build. See Drawing 00. |
| `OW_DATA` | WaziSense `A2` | DS18B20 `DQ` | yellow | D9 is the radio's RESET — not usable |
| — | 4.7 kΩ | `OW_DATA` → `D6(+)` | — | **mandatory** 1-Wire pull-up — unlike the I²C ones, this resistor you *do* fit. To `D6(+)`, not the board's permanent 3V3, or it feeds a dead sensor between measurements. |

> [!NOTE]
> ### Three ways a DS18B20 reads wrong
>
> **No pull-up, no bus.** Without the 4.7 kΩ the data line never returns high and the sensor is simply absent — the library reports `-127 °C` (`DEVICE_DISCONNECTED_C`). That is the first thing to check if temperature fails while everything else works.
>
> **Exactly 85.0 °C is not a temperature.** It is the DS18B20's power-on reset value, and you read it when the conversion has not finished or the supply is marginal. Since the rail is gated by `D6`, a too-short settling time produces precisely this. The sketch's sanity band (−20 °C to +60 °C) rejects both −127 and +85, so a rejected reading shows as `FAIL` rather than as plausible-looking data.
>
> **Do not use parasite power.** Cheap sealed probes are sometimes wired for it, with `VDD` tied to `GND`. Give the sensor its own `VDD` from `D6(+)`: parasite mode is marginal during the 750 ms conversion and pointless here, since you are already running a ground wire to the probe.
>
> **Check the lead colours against a meter, not against a colour convention.** Sealed DS18B20 probes commonly use red = `VDD`, black = `GND`, yellow = `DQ`, but blue/white variants exist. If the probe has a bare drain wire, land it on `X1-2` at the enclosure end only, like the RS485 shield.
>
> **What you give up against the PT1000:** ±0.5 °C instead of ±0.1 °C. Irrelevant for the pH temperature compensation; for EC it is roughly ±1 % of reading, since conductivity drifts about 2 % per °C. Acceptable for a test deployment — worth revisiting before the data is used for anything regulatory.

> [!NOTE]
> ### The isolated grounds are not grounds
>
> Each EZO circuit sits behind its own isolator, so `ISO_GND` for pH and `ISO_GND` for EC are **two separate floating nets** — neither of them is `X1-2`. Connecting either one to the main ground, or to each other, restores exactly the path the isolators exist to break, and the pH reading will start drifting whenever EC measures. When you check continuity during assembly, an open circuit between `ISO_GND` and `X1-2` is the *correct* result.

## NOTESBefore you power it up

- 01

**Leave the RS485 addresses alone.** Both probes arrive on `0x01` and stay there; the firmware separates them by baud rate. If you ever *do* change an address, connect one probe at a time — two devices answering at once produces corrupted frames that look exactly like a wiring fault.
- 02

**The board switches the GROUND leg, and that is fine.** This is the opposite of what earlier revisions of this document said, and of what the manual's "Dedicated VCC Mosfet" implies — see Drawing 00 for the measurements. The bus reference the RS485 probes need is established through the conducting MOSFET while a measurement runs, and in sleep nothing drives the bus. What you must *not* do is give any device on `D6(−)` a second path to a GND pin: that shorts the switch out and the rail never turns off.
- 02b

**Bring the rails up in order: 3.3 V first, then 12 V.** The transceiver has to be alive and its fail-safe bias established before the probes start driving the bus. Power down in reverse. The sketch does this in `rails33On()` / `rails12On()` / `railsOff()`; the 3.3 V settling delay is **1.5 s**. The EZO circuits need about a second to boot, and the WaziSense manual asks for “at least 1500 ms” after switching a sensor rail on, so the longer figure satisfies both.
- 03

**Confirm the converter's input range.** The U3V9F12 starts at 2.6 V, which covers a flat cell. A generic module may not — the one replaced here stopped regulating below 3.2 V and simply passed its input through. You are feeding it from a 3.0–4.2 V cell, so verify the low end and set the output trimpot to 12 V *before* connecting any probe.
- 04

**Two isolators are fitted; only the pH one is required.** A loop needs two grounded ends, so floating the pH side already opens it and EC needs none — the EC one is the first thing to remove if autonomy ever binds. Power both from the battery through #2810 (B), *not* from `D6(+)`: the pH pair draws **57 mA** and the EC pair **82 mA** at 3.3 V (datasheet), **130 mA measured together**, and switching that onto the board's own 3.3 V regulator browns the MCU out at every rail switch-on.
- 05

**Ground the cable shield at one end only.** The enclosure end. Grounding both ends turns the shield into a ground loop.
- 06

**Desolder the module power LEDs — the ungated ones especially.** Anything on X3 is dead between measurements, so its LED costs almost nothing. The **boost converter** used to be the one that mattered: before the #2810 was fitted it ran continuously. It has company now — **each #2810 carries its own LED, and both were observed lit right through sleep** with the switch off, so they sit on the input side. The datasheet's ~210 µA/V puts them at roughly 0.8 mA each at 3.9 V. Confirm against a sleep-current measurement before desoldering anything.
- 07

**Bring the buses up separately.** Use the `ENABLE_*` flags at the top of the sketch to test one sensor at a time before wiring the full bus.
- 08

**Set the RS485 echo flag before you trust a timeout.** With an auto-direction module, `RS485_ECHOES_OWN_TX` being wrong looks exactly like a broken bus. Settle it on the first probe you connect — see the note under Table 06.
- 09

**Measure the I²C pull-ups before you trust them.** `A4`–`3V3` and `A5`–`3V3` must read **~4.7 kΩ**. The manual says `I2C_J` ships closed, but it has been found **open** — 5.9 MΩ on both lines. A missing pull-up is not a soft failure: the TWI hardware releases SCL and waits for it to rise before it can issue a START, so it waits forever and the sketch stops dead with no message. If the jumper is open, fit **2 × 4.7 kΩ from `A4`/`A5` to `D6(+)`**. That terminal is permanently live, which is harmless here: a low-side switch lets each circuit's ground float up to the same 3.3 V, so there is no differential and nothing is fed into an unpowered input.
- 10

**Power gating replaces the EZO sleep command.** The circuits lose power entirely between readings, so nothing needs to be told to sleep. Their calibration lives in their own EEPROM and survives — but each one needs its ~1 s boot time before it will answer.

> [!NOTE]
> ### Register maps — settled, and how
>
> Both probes now read correctly, and the byte orders are *not* the same. The SEN0680 sends plain big-endian floats (`FMT_FLOAT_ABCD`) at register `0x0002`, with its own water temperature at `0x0004`. The **Y511-A is fully byte-reversed** (`FMT_FLOAT_DCBA`) at `0x2602`, and its 16-bit values are little-endian too — reading `1E 00` the Modbus way gives 7680 where the answer is 30.
>
> That cost real time, because a scanner that prints only ABCD and CDAB hides a DCBA device completely: every candidate looks like noise and the sensor appears dead. **Print all four permutations** when identifying an unknown register. `RS485Scan/` does.
>
> The Y511-A also needs frames that are not valid Modbus by the letter of the specification: its register-count field is `0x0000`. The wiper sweep is `01 10 31 00 00 00 00 74 94` — verified on the bench, and the only command that actually moves it.
>
> **That command is no longer sent.** The probe sweeps itself whenever the 12 V rail comes up, and the commanded sweep — arriving 3 s later, into a running motor — was measurably *truncating* the automatic one: two strokes when commanded against three when not. `TURB_BRUSH_BEFORE_READ` is `0`. The frame is documented here because the code is still behind that flag, for a future probe that does not clean itself on power-up.
