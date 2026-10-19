# KijaniSpace Sensor Carrier

A carrier board for the Lake Victoria water-quality node: the WaziSense v2, five modules and six probes reduced to plug-in connectors, with the four traps this build has already paid for designed out in copper.

- Design Rev E · board silkscreen reads **Rev C** · as built, 13 October 2026 · routed, verified, fabrication files released

This board exists because the wiring is now well understood and the mistakes are documented. Every trap below was found the expensive way on the hand-wired node. A PCB is the one place where a mistake you have already made can be made impossible rather than merely written down.

> [!TIP]
> **As built — 7 October 2026**
>
> The board exists. It is placed, routed, verified and released: **253 track segments, 37 vias, 36 nets, 0 unconnected, 0 electrical errors, 0 silkscreen errors** with the copper pours filled. Gerbers, drill file, BOM and placement files are in [`pcb/`](../pcb/), and [`pcb/out/FACTS.md`](../pcb/out/FACTS.md) is the file that is kept current — where this guide and that file disagree, FACTS.md is right, because `gen/facts.py` reads it straight out of `KijaniCarrier.kicad_pcb`. **Correction:** this paragraph previously named `pcb/README.md` as the generated file. It is not generated — nothing in `gen/` writes it — and it had drifted.
>
> Three checks run on every rebuild and are the reason the numbers above can be trusted: `barrier.py` proves no track, via or pour crosses an isolation slot; `check.py` proves every track endpoint lands on a pad of its own net; `pour.py` floods the filled ground and proves every GND pad is reachable. The last one found three pads walled into pockets by their own neighbours, and KiCad’s own filler found a fourth that the raster had let through.

> [!WARNING]
> **What changed between Rev D and the board that was built**
>
> **The mirrored pad order.** Every module footprint had its pad rows reversed. It was caught by laying a module on the table as it will sit on the board — component side up, pins down — and reading the silkscreen left to right. `VCC` swapped with `GND` destroys the transceiver and both EZO circuits on first power-up, so this is the single most expensive error the project avoided.
>
> **`H1` stays on 3.3 V, and `U2` is not fitted.** Trap 5 below argues for moving `H1` to BATT once an LDO exists. The carrier removed the reason instead: the heavy loads — the 12 V chain and both isolators — now hang on `+VBATT` through their own high-side switches, not on `D6(+)`. What is left on that rail is the transceiver and the DS18B20, a few milliamps. The WaziSense’s regulator is no longer loaded, so there is nothing to move away from, and keeping `H1` on 3.3 V preserves the low end: with `H1` on BATT and a 3.0 V cell, the rail would sit at 2.7 V after the Schottky and the DS18B20’s own minimum is 3.0 V. `JP1` is fitted in `U2`’s place. Both footprints are on the board, so the decision can still be reversed without a new layout.
>
> **The terminator moved out of the BOM and back in, twice.** It is fitted. The measurements that settle it: the RS485 module’s own 120 Ω sits on an open `120_S` jumper (10.8 kΩ across A/B), and the probes bring none of their own. `RT1` is therefore the only terminator in the system.

> [!WARNING]
> **Rev C assembly note: one placement fault, now caught by the build**
>
> Rev C is designed to be ordered fully assembled: SMT parts, terminals, electrolytics, SMA jacks and all module sockets are placed by the assembler. Nothing is left to hand-solder.
>
> **The placement file put `U1`’s 3-pin socket 5.5 mm off its holes** (Y −67.00 instead of −72.50). The generator took the centre of the whole Pololu module’s outline, but the assembler fits only the pin socket, which sits on the pad row at the module’s edge. The assembler’s placement preview showed it, and its own corrected preview still had it wrong. Boards made from those early files may carry the socket in the wrong place.
>
> What now stops it recurring: `gen/cplcheck.py` runs inside `fab.sh` and fails the build if any through-hole placement point is more than 0.05 mm off the centre of its own holes, or any SMT point lies outside its pads. Run against the early files, it flags exactly `U1` and nothing else. **In JLCPCB’s placement render, check position as well as rotation: every socket and terminal must cover its holes, and no plated hole may show white.**

## Section 01What the board replaces

The hand-wired node carries roughly forty conductors between the WaziSense, five modules and six probes, plus eight discrete components on flying leads. Most of those are fixed by the design and have exactly one correct destination, which is the definition of something that belongs on a PCB.

| Today | On the carrier |  |
| --- | --- | --- |
| ~40 flying leads | **14 conductors** in four plugs — see Section 04 | `new` |
| PTC + SS34 + SMBJ on leads, currently **bypassed** | Designed in, always present | `new` |
| 2 × Pololu #2810 (€5 each, LED lit in sleep) | Discrete P-FET high-side switches, €0.40 each | `changed` |
| Pololu U3V9F12 on leads | Same module, on a socket | `kept` |
| MAX3485 module on leads | Same module, on a socket | `kept` |
| 2 × Atlas isolator, 2 × EZO circuit on leads | Sockets, on an isolated island with a routed slot | `changed` |
| 120 Ω + 2 × 680 Ω bias on leads | Designed in, **pull-down returned to the switched node** | `changed` |
| 10 kΩ `RXI` pull-up — not yet fitted | Designed in | `new` |
| Probe leads into screw blocks shared between probes | Two ganged 2-way blocks per Modbus probe, separate | `changed` |

## Section 02The four traps, designed out

This is the part of the document worth reading even if the board is never built. Each of these cost real time on the hand-wired node, and each becomes structurally impossible on a PCB rather than a note somebody has to remember.

### Trap 1 — the stray ground that shorts out the rail switch

`D6` on the WaziSense is a **low-side** switch. It gates `D6(−)`, the return; `D6(+)` is permanently live. Every device on that rail must return to `D6(−)` and nowhere else. One extra wire from any of those grounds to a GND pin bypasses the MOSFET and the rail never switches off. That happened through the MAX3485's ground and cost **20.78 mA of sleep current** before it was found.

On the board this becomes a **separate copper pour**, `SWGND`, with exactly one connection to the outside world: `SW_1` pin 2. There is no via, no stitch, no path to `GND` anywhere on either layer. A wiring error that takes twenty minutes with a meter to find becomes a design-rule violation you cannot draw.

> [!TIP]
> **The vendor’s own CAD source now confirms this**
>
> The low-side topology was established here by measurement, against a manual that reads like high-side switching. The EasyEDA source settles it beyond argument:
>
> - `U3` and `U4` are **RUF025N02** — N-channel MOSFETs, 20 V, 2.5 A. N-channel means low side.
> - `SW_1` pin 1 and `SW_2` pin 1 are **the same net**, `SW_OUT`. That is the 1.2 Ω measured between the two “+” terminals.
> - `SW_1` pin 2 goes to `U3` pin 3 and `SW_2` pin 2 to `U4` pin 3 — each block’s minus terminal is its own MOSFET’s drain.
> - `SW_OUT` comes from `H1` pin 2, which is why one jumper sets both blocks.

![Two separate ground pours. The main GND pour carries the battery return, the 12 volt converter and the probe terminals. A second, physically separate pour called SWGND carries only the RS485 transceiver, the DS18B20 and the bias pull-down, and connects to the outside world only through the D6 minus ](img/sensor-carrier-01.svg)

*The central idea of the board. Two pours that never touch, and one deliberate exit. The wiring error that cost 20.78 mA has no route to exist here.*

> [!WARNING]
> **Tell the assembler before they "fix" it**
>
> A design-rule check will report `SWGND` and the two isolated grounds as separate, unconnected nets. **That is the intended result, not an error.** Anyone who stitches them to `GND` to make the DRC quiet destroys both the rail switching and the galvanic isolation. Put this on the silkscreen and in the fabrication notes.

### Trap 2 — the fail-safe bias that never sleeps

The RS485 bias as currently specified runs from the permanently live `D6(+)`, through 680 Ω, the 120 Ω terminator and a second 680 Ω, to permanent ground. That is a closed chain with nothing to switch it:

3.3 V / (680 + 120 + 680) Ω = **2.23 mA, continuously**

Against the whole-node sleep current of 3.2 mA measured at the time, that is most of it. (It was found: the pull-down now returns to `D6(−)`, and with the radio also put to sleep the node reached **1.08 mA**.) The fix costs nothing: return the pull-down to `D6(−)` instead of `GND`. With the rail on, the MOSFET holds `D6(−)` at zero and the bias works exactly as before. With the rail off the chain has no destination — the switched node floats up to `D6(+)`, the differential is zero, and no current flows.

> [!TIP]
> **This one is worth doing on the hand-wired node today**
>
> Move one end of one resistor: the 680 Ω between `X2-4` and `X1-2` goes to `D6(−)` instead. Then measure sleep current again. If it drops by roughly 2.2 mA, this document just paid for itself before the board was ordered.

### Trap 3 — the slide switch that overrides the microcontroller

Each Pololu #2810 has a mechanical slide switch that, in the ON position, ignores the `ON` pin entirely. A module fitted with that switch in the wrong position runs its load permanently and the firmware has no way to notice. Its indicator LED is a second, smaller version of the same problem: observed lit right through sleep with the switch off, so it sits on the input side and costs roughly 0.8 mA per module around the clock.

> [!NOTE]
> **Measured afterwards, and the mechanism turned out to be different**
>
> The LED draws **0.1 mA**, not 0.8, and it is not the module leaking — it is being back-fed. The I²C pull-ups sit on a permanently live rail; the isolator’s A/B inputs have ESD diodes to its own `VCC` pin; with the isolator switched off that pull-up current flows in through those diodes and out at the regulator’s output, where the LED hangs. Disconnect `VOUT` and the LED goes out.
>
> **On the carrier the path has no load at its end.** `ISO_VIN` connects to exactly three pads: `Q3`’s drain and the two isolators’ `VCC`. No LED, no regulator. The node can be pulled up to about 2.6 V and then nothing more flows, and nothing reaches the cell either: `Q3` is a P-channel part whose body diode would need 4.4 V on the drain to conduct.
>
> **What that same net does cost, and it is accepted rather than fixed.** The isolators’ MCU-side pull-ups go to their own `VCC`, which is `ISO_VIN` — the raw cell behind `Q3`, up to 4.2 V. The ATmega’s absolute maximum on a pin is VCC+0.5 = **3.8 V**, so with a full cell `A4` and `A5` sit above it and the input clamp diodes conduct, limited by the isolator’s own 4.7 kΩ to about **64 µA per line**. That is well inside the part’s DC injection tolerance, it lasts only the ~2 s the isolators are powered, and the hand-wired node has run this way for weeks. It is still outside absolute maximum on paper, and the alternative — feeding the isolators from the MCU’s own regulator — is the brownout this whole switch exists to avoid. **Consequence for anyone changing this board: do not raise `ISO_VIN`, and do not hang a third device off that bus.**

The board replaces both modules with a discrete high-side switch: a P-channel MOSFET, an N-channel level shifter and three resistors. No mechanical switch to get wrong, no LED, about €0.40 in parts instead of €5, and one capacitor buys a soft start that the module never offered.

![Discrete high-side switch schematic. A P-channel MOSFET has its source on the protected battery rail and its drain feeding the load. A 100 kilohm resistor from gate to source holds it off. A 2N7002 N-channel MOSFET pulls the gate down when its own gate is driven high from the microcontroller through](img/sensor-carrier-02.svg)

*Five parts per switch, all in JLCPCB's basic library. The soft start is the part the #2810 could not do, and it is aimed straight at the rail sag measured during the wiper sweep.*

  ### Trap 4 — the floating `RXI` that hangs the bootloader

With no FTDI attached, the bootloader's receive pin floats, picks up phantom start bits and waits for an upload that never comes. Neither `RESET` nor a watchdog clears it; only removing power does.

> [!TIP]
> **This one turns out not to need the carrier at all**
>
> The FTDI header is `DTR · TXD · RXD · 3.3V · GND · GND`, so **`RXD` and `3.3V` are pins 3 and 4 — adjacent, 2.54 mm apart.** The entire fix is one 10 kΩ resistor soldered across those two pins of the header. It does not fight an attached FTDI, because a pull-up cannot fight a push-pull driver.
>
> Do that now, on the hand-wired node. It is a two-minute job and it removes a failure mode that needs a power cycle in the field to clear.

### Trap 5 — `H1` is in the wrong position for a solar deployment

> [!TIP]
> **Superseded — read this before the section below**
>
> The reasoning here is sound but its premise is gone. It assumes the sensor load sits on `D6(+)` and therefore on the WaziSense’s own regulator. **On the carrier it does not.** The 12 V chain and both isolators are switched high-side off `+VBATT`; what is left on `D6(+)` is the transceiver and the DS18B20, a few milliamps. There is no longer a load to move away, so `H1` stays on **3.3 V**, `JP1` is fitted and `U2` is not.
>
> Moving `H1` to BATT would now cost rather than gain: at a 3.0 V cell the rail reaches 2.7 V after the Schottky, the MCP1700 drops out, and the DS18B20’s own minimum is 3.0 V. Both footprints are on the board, so this is a jumper-and-one-part decision, not a respin.

The WaziSense v2 manual says this twice, and the second time it says why:

> “when deploying the WaziSense v2 with solar panel, it’s advised to move the `H1` jumper to BATT position.”
>
> “It appears the solar circuit’s highs and lows during operation affects transmissions.”

This build has `H1` on **3.3 V**, and for a good reason: the MAX3485 is a 3.0–3.6 V part and a full cell at 4.2 V would destroy it. But that choice puts the whole sensor load on the WaziSense’s own HE2031A33 regulator — the same one feeding the MCU and the radio. **That is the cold-start brownout this build already fought and patched with 470 µF.** The manual’s advice and that fault are the same phenomenon seen from two sides.

The carrier resolves it instead of choosing between the two. `H1` goes to **BATT**, so `D6(+)` carries cell voltage and the WaziSense’s regulator carries nothing but the MCU and the radio. A small LDO on the carrier then makes the clean 3.3 V that the transceiver, the DS18B20 and the pull-ups need.

> [!NOTE]
> **Why the LDO’s ground goes to GND and not to the island**
>
> It looks wrong at first: the LDO feeds devices that sit on `SWGND`, so why not reference it there too? Because the I²C and `RXI` pull-ups need that clean 3.3 V as well, and they belong to the MCU’s ground domain — on the island they would float during the 200 ms window where `A1` is already high and `D6` is not.
>
> Referencing the LDO to `GND` costs nothing, because the low-side principle still does the work: with `D6` open the transceiver’s supply is held at 3.3 V while its *ground* floats up to meet it. Zero volts across the part, no current, exactly as before. The only permanent draw is the LDO’s own quiescent current — **1.6 µA** for an MCP1700, against the 2.23 mA the bias network is giving back.
>
> **The LDO is also what makes `H1` on BATT safe at all,** which is not obvious. Today the transceiver is fed from `SW_OUT` directly, so when `D6` opens its ground floats up to *whatever `SW_OUT` is*. At 3.3 V that matches the parked state of `D3`/`D4` and nothing flows. Move `H1` to BATT without the LDO and that floating ground rises to 4.2 V while `D3`/`D4` sit at 3.3 V — almost a volt backwards into the transceiver’s I/O pins, through its ESD clamps. With the LDO in front, `SW_OUT` feeds only the LDO, and the island floats to the LDO’s 3.3 V instead. The differential is zero again.

> [!WARNING]
> **Do not move H1 before the LDO exists**
>
> The LDO cannot make 3.3 V from a cell below about 3.35 V; it drops out and passes its input minus a few tens of millivolts. That is acceptable — the MAX3485 works down to 3.0 V, and the LDO’s real job is clamping the **top** end, where a full cell at 4.2 V would otherwise reach the transceiver. But it does mean **`H1` must stay on 3.3 V on the hand-wired node.** Moving it to BATT today puts 4.2 V on a 3.6 V part.

> [!NOTE]
> ### What the WaziSense v2 manual settles about this board
>
> **The WaziSense has no protection at all on its battery input** — no fuse, no reverse-polarity element, no TVS. Its CN3791 charger protects only while charging, and its HE2031A33MPR regulator’s current limiter protects the regulator. So `F1`, `D1` and `D2` on this board are not a second line of defence; they are the only one.
>
> **`H1` stays on 3.3 V**, although the manual advises BATT when deploying with a solar panel. Two reasons. The hard one: `H1` sets this board’s `3V3REG`, and `U3.1` — the RS485 module’s `VCC` — is on it. The module’s `RO` is push-pull and runs to `J4.2` → `D3` with **no series resistor** (`R2` and `R5` are gate resistors, not signal ones), so at BATT a full cell drives 4.2 V into a 3.8 V pin, current-unlimited. The one that removes the incentive: the ATmega and the radio sit behind the on-board regulator whatever `H1` does, so BATT cannot extend the node’s operating range — it only keeps sensors alive after the MCU has stopped.
>
> > [!WARNING]
> >  ### The Schottky would have stopped the node charging at all
> >
> >  Confirmed on the prototype: **the solar panel hangs directly on the WaziSense**, so the CN3791 charges through `BAT_IN`. On this board there is exactly one path between that pin and the cell:
> >
> >  X1 (cell) → F1 → D1/Q5 → +VBATT → J6 → WaziSense BAT\_IN
> >
> >  Charge current has to run that path *backwards*. **A Schottky blocks it.** Rev A, as drawn, would have powered the node perfectly and never put a milliamp back into the cell — a fault that shows up as "the battery goes flat in a fortnight despite full sun", weeks after deployment, with nothing in any log to point at it.
> >
> >  **`Q5` removes the problem as a side effect.** A MOSFET channel that is switched on conducts in *both* directions — the known limitation of a simple gate-to-ground ideal diode, and here it is exactly what is needed. The reverse-polarity protection still works, because with the cell reversed the FET is off and its body diode is the one that blocks.
> >
> >  So the swap was decided on voltage headroom and turns out to have been necessary for a second, larger reason. **If you ever go back to a series diode in that position, the panel has to move to an external charger wired directly at the cell.**
> >
> >
>
> **And the reason the manual gives is already dealt with here.** Its complaint is that the solar circuit’s ripple reaches the radio through a loaded regulator, so moving the sensor load off that regulator cleans up transmissions. On this board the sensor load is *already* off it: the isolators’ 130 mA go through `Q3` and the 12 V chain’s ~400 mA through `Q1`, both straight from `+VBATT`. What is left on `3V3REG` is the RS485 module, the DS18B20 and the bias network — under 10 mA, against a regulator rated above 400 mA. **The carrier achieves what BATT was for, using two MOSFETs instead of a jumper.**
>
> **What actually costs the bottom of the cell is `D1`.** Measured on the bench: the node transmits normally at a 3.5 V cell, loses pH, EC and turbidity at 3.4 V, and will not start below that. What drops out at 3.4 V are exactly the three measurements with a heavy supply behind them — and all of that current crosses the series SS34, which drops **0.45–0.5 V at the ~400 mA** the 12 V converter pulls. An ideal-diode P-MOSFET there (AO3401A at 50 mΩ: drain to the cell, source to the load, gate to ground through 100 kΩ) drops 20 mV instead and moves the working floor from ~3.5 V to ~3.2 V.
>
> > [!NOTE]
> >  ### Done — `D1` is now `Q5` + `R13` (9 October 2026)
> >
> >  The SS34 is gone. In its place sits an **AO3401A** in SOT-23 at rot 180, so pad 3 (drain) faces `F1` and the cell where the old cathode band faced, and pad 2 (source) feeds `+VBATT`. `R13`, 100 kΩ, holds the gate at `GND`, which makes VGS = −Vbatt whenever the cell is the right way round and 0 when it is not. VGS never passes −4.2 V against the part's ±12 V rating, so no gate Zener is needed.
> >
> >  **The orientation is the whole protection.** Source-to-cell would leave the body diode conducting on a reversed cell, and the board would still appear to work — which is why the BOM line for `Q5` says so in full and the silkscreen now reads `Q5 = IDEAL DIODE, NOT A SCHOTTKY`.
> >
> >  Routing: `Q5.2` is the north pad and the gate sits directly south of it, so the supply cannot drop straight down past its own gate. It steps east to x 56, runs south clear of `R13`, and joins the existing y 48 rail at `D2.1`. **Verified:** 0 DRC violations, 0 unconnected pads, 0 footprint errors; 56 footprints, 253 segments, 37 vias, 150 pads. Both new parts fold into BOM lines that already existed (`Q1`/`Q3` are the same AO3401A, `R1`/`R3`/`R4`/`R6` the same 100 kΩ), so the JLCPCB bill went from 23 lines to **22**, and after the screw terminals were split it is now **21**.
> >
> >  **The board on the silkscreen is now Rev B.** A Rev A panel carries a Schottky at that position and must not be confused with this one.
> >
> >
>
> Regulator floor, for the record: HE2031A33MPR, **3.5–25 V in**, 0.18 V dropout at 100 mA, 2.0 µA quiescent. With `D1` in series the cell-side cut-off lands near 3.3 V. That is a brown-out, **not a disconnect** — nothing on this board or the WaziSense isolates the cell afterwards, so deep-discharge protection has to come from the cell’s own PCM or from a firmware hibernate threshold.

## Section 03Block diagram

![Block diagram. The 18650 cell enters the carrier through a protection chain of PTC, an ideal-diode MOSFET and SMBJ. The protected battery rail feeds three things: the WaziSense BAT_IN input, high-side switch A which feeds the U3V9F12 twelve volt regulator and the two Modbus probes, and high-side swi](img/sensor-carrier-03.svg)

*The cell enters the carrier, not the WaziSense — so the protection chain is unavoidably in circuit, which it currently is not.*

  ## Section 04Interface to the WaziSense

> [!WARNING]
> **Rev A was wrong about this**
>
> Rev A specified a 16-way ribbon into a pin header. **The WaziSense v2 has no signal pin headers.** Everything a user can reach comes out on **3.5 mm screw terminal blocks** (DB125-3.5), and the only 2.54 mm headers are `H1`, `H2`, the charge-status pins, the SPI header and the FTDI socket. A ribbon has nothing to plug into.

### Everything the board brings out

Read out of the EasyEDA PCB source, so these are net names as the designer assigned them rather than as a manual describes them. `X` is the pad centre in the board’s own coordinate system.

| Block | Pin | Net | X (mm) | Note |
| --- | --- | --- | --- | --- |
| `SW_1`Sensor Power 1 | 1 | `SW_OUT` | −50.01 | the permanently live “+”, set by `H1` |
| 2 | `U3` drain | −46.51 | **the switched return** — gated by `D6` |
| `SW_2`Sensor Power 2 | 1 | `SW_OUT` | −42.39 | **same net as `SW_1` pin 1.** This is the 1.2 Ω you measured |
| 2 | `U4` drain | −38.89 | a second switched return, gated by `D7` — unused here |
| `ANALOG_PINS` | 1 | `A2` | −34.74 | 1-Wire, the DS18B20 |
| 2 | `A1` | −31.24 | `ISO_EN` |
| 3 | `GND` | −27.74 |  |
| `DIGITAL_PINS` | 1 | `D4` | −23.57 | RS485 `TXD` |
| 2 | `D3` | −20.07 | RS485 `RXD` |
| 3 | `GND` | −16.57 |  |
| `I2C_PORT` | 1 | `SCL` | −12.36 | = `A5` |
| 2 | `SDA` | −8.86 | = `A4` |
| 3 | `D5` | −5.36 | **`D5` lives on the I²C block**, not the digital one. `RAIL12_EN` |
| 4 | `GND` | −1.86 |  |
| `BAT_IN` | 1 | `BATT` | −22.25 | on the opposite edge, Y = +33.02 |
| 2 | `GND` | −25.75 |  |
| `H1`2.54 mm | 1 | `BATT` | −45.21 | Y = 13.84. Pin 2 is `SW_OUT`, which is why one jumper sets *both* Sensor Power blocks |
| 2 | `SW_OUT` | −42.67 |
| 3 | `3.3V` | −40.13 |
| `FTDI`2.54 mm, female | 1 | `DTR` | 14.86Y −11.37… +1.33 |  |
| 2 | `TXD` |  |
| 3 | `RXD` | **trap 4 lives here** — 10 kΩ to pin 4 |
| 4 | `3.3V` | the only place the board’s own regulated rail is reachable, besides SPI pin 2 |
| 5 | `GND` |  |
| 6 | `GND` |  |

> [!NOTE]
> **There is not one spare pin**
>
> Counting what the terminals expose: `D3`, `D4`, `D5`, `A1`, `A2`, `SDA`, `SCL`. This build uses all seven. `A0` is the battery divider, `A3`, `A6` and `A7` exist only as pads in the prototyping area, and `D7` drives `SW_2`’s MOSFET and goes nowhere else.
>
> So the pin map in the firmware is not one arrangement among several. It is *the* arrangement, and anything further needs a wire soldered into the prototyping area.

### Mechanical

| Item | Value |
| --- | --- |
| Board outline | **70.37 × 52.85 mm** — X from −52.08 to +18.29, Y from −14.23 to +38.61 |
| Mounting holes | **3 × ∅3.0 mm** at (−46.99, 9.14), (8.38, −0.25), (4.83, 36.20) |
| Signal edge | All four signal blocks on **one edge**, Y = −5.72 (`I2C_PORT` at −5.97) |
| Power edge | `BAT_IN` and `SLR_IN` on the opposite edge, Y ≈ +33 |
| Block pitch | 3.50 mm within a block; the gap between blocks is **4.1–4.2 mm**, so the row is *not* one continuous 3.5 mm grid |

### Three ways to join the two boards

**A — screw to screw, nothing changes on the WaziSense.** The carrier carries its own 3.5 mm blocks and fourteen short wires join them. That is 14 conductors in place of about 40, every one of them short and fixed in length, and it works with the board exactly as it is today. It is still a screwdriver job.

**B — pluggable terminals, and this is what I would build for ten boards.** The DB125-3.5 blocks are one BOM line on a board your own organisation makes. Replace them with the *pluggable* 3.5 mm family (2EDGV / 2EDGK and equivalents): same footprint, same pitch, but the board side becomes a keyed male header and the wires terminate in a plug. The carrier gets the same headers, and a set of short pre-made jumpers joins them. Plug in, plug out, no screwdriver, and no mechanical alignment to get right.

**C — a true mezzanine.** Now designable: the outline, the three M3 holes and every pad centre are known to a hundredth of a millimetre. It is the neatest result and the least forgiving, and it needs the enclosure chosen first, because the two boards become one rigid assembly. I would not do this before a Rev B carrier has been built and proven as option B.

## Sheet APower input, protection and the two switches

The cell enters here, not at the WaziSense, so the protection chain is unavoidably in circuit. Everything downstream of `+VBATT` is switched by one of the two high-side stages.

![Schematic sheet A. The cell connects to terminal X1. Its positive goes through PTC F1 and, on the board as built, the ideal-diode MOSFET Q5 (drawn here as the Schottky D1 it replaced) to the VBATT net, with TVS D2 and capacitors C1 and C2 from VBATT to ground. VBATT feeds connector J6 to the WaziSen](img/sensor-carrier-04.svg)

***This sheet still draws `D1` as a Schottky.** On the board that position is `Q5`, an AO3401A ideal diode, with `R13` holding its gate to `GND`; the symbol has deliberately not been redrawn, so read it with that substitution in mind. Both switch stages are identical and use the same five parts. The soft start on `C3`/`C5` is what the Pololu #2810 could not do, and it is aimed at the rail sag measured during the wiper sweep.*

  ## Sheet BThe 3.3 V domain and the SWGND island

Everything on this sheet returns to `SWGND`, except the three capacitors. That one distinction is the whole reason the board exists.

![Schematic sheet B. Connector J2 brings in the permanently live 3V3P from the Sensor Power plus terminal and SWGND from the minus terminal. 3V3P feeds either a link JP1 or the MCP1700 regulator U2, producing 3V3REG. 3V3REG supplies the MAX3485 module, the 4.7 kilohm one-wire pull-up R9, the 680 ohm b](img/sensor-carrier-05.svg)

*`JP1` and `U2` are alternatives, not both. Fit the link for `H1` on 3.3 V, which is the build as it stands; fit the regulator if `H1` ever moves to BATT as the WaziSense manual advises for solar deployments.*

  ## Sheet CI²C, the isolators and the probes

Each isolator carries **its own 4.7 kΩ pull-ups on both sides** — confirmed in the Atlas datasheet — so the board fits none of its own by default.

![Schematic sheet C. SDA and SCL arrive from connector J5 and run to the data A and B pins of both isolators U4 and U5. Each isolator takes its supply from ISO_VIN and its ground from SWGND, and leaves its enable pin unconnected. Beyond each isolator's barrier, an isolated 3.9 volt supply and a separa](img/sensor-carrier-06.svg)

*Two isolated islands, each with its own floating ground, each separated by a routed slot. Neither isolated ground touches `GND`, `SWGND`, or the other one.*

  ## Section 05Complete netlist

Reference designators are provisional and become fixed when the schematic is drawn. Nets in `SWGND` and the two isolated groups are marked, because those are the ones that must not be merged.

| Ref | Part | From | To | Net class |
| --- | --- | --- | --- | --- |
| `X1` | 2-way screw block, 5 mm | 18650 +/− | `F1` / `GND` | battery |
| `F1` | PTC 2.6 A hold | `X1-1` | `Q5` drain | battery |
| `Q5` | AO3401A ideal diode *(was `D1`, SS34)* | `F1` | `+VBATT` | battery |
| `R13` | 100 kΩ gate pull-down for `Q5` | `Q5` gate | `GND` | battery |
| `D2` | SMBJ5.0A TVS | `+VBATT` | `GND` | **Was the prime suspect for the node’s unexplained sleep current. Cleared — keep it as specified.** Its datasheet leakage is alarming here (IR 800 µA max at 5.0 V, permanently across the cell) but the bench supply acquits it: **4.2 V → 1.46 mA, 3.0 V → 1.17 mA**, where a purely resistive load would give 1.04 mA. Avalanche leakage collapses far faster than linearly, so the TVS is at most ~0.4 mA of the total and probably much less. |
| `C1` | 100 µF + 100 nF | `+VBATT` | `GND` | battery |
| `Q1`/`Q2` | AO3401A + 2N7002 | `+VBATT` | `V12IN` | battery → 12 V |
| `R1–R3` | 100 k / 100 k / 10 k | gate network A | — | — |
| `C2` | 100 nF soft start | `Q1` gate | `+VBATT` | — |
| `U1` | Pololu U3V9F12, socket | `V12IN` | `+12V` | 12 V |
| `C3` | 470–1000 µF | `+12V` | `GND` | 12 V |
| `Q3`/`Q4` | AO3401A + 2N7002 | `+VBATT` | `ISO_VIN` | battery → isolators |
| `R4–R6`, `C4` | gate network B | — | — | — |
| `U7` | MCP1700-3302E LDO | `3V3P` (= `D6(+)`, cell voltage) | `3V3REG`, ground on `GND` | 1.6 µA quiescent |
| `C7`/`C8` | 1 µF ceramic each | LDO in / out | `GND` | required by the part |
| `U2` | MAX3485 module, socket | `3V3REG` / `SWGND` | `RS485_A/B` | **SWGND** return |
| `R7` | 680 Ω bias pull-up | `RS485_A` | `3V3REG` | — |
| `R8` | 680 Ω bias pull-down | `RS485_B` | `SWGND` | **SWGND** — trap 2 |
| `RT1` | 120 Ω terminator | `RS485_A` | `RS485_B` | — |
| `R9` | 10 kΩ | `RXI_PU` | `3V3REG` | trap 4 |
| `R10` | 4.7 kΩ | `OW_DATA` | `3V3REG` | mandatory |
| `R11`/`R12` | 2 × 4.7 kΩ | `SDA`/`SCL` | `3V3REG` | **DO NOT FIT — this row used to say the opposite.** It was written when `H1` was going to sit on BATT; the BOM, schematic sheet C and the placement drawing have said DNP throughout, and they are right. The isolators carry their own 4.7 kΩ pull-ups on *both* sides, powered from `ISO_VIN`, which `A1` switches. Fitting `R11`/`R12` to the permanently live `3V3REG` would rebuild the bench build’s worst sleep load on the PCB: there, unpowered modules clamp the bus and the lines measure **2.25 V instead of 3.3 V — 223 µA per line, 447 µA continuously**. Leave them empty and leave `I2C_J` open. |
| `C5` | 470 µF | `3V3REG` | `GND` | the cold-start brownout reserve, now on the carrier’s own rail instead of the WaziSense’s |
| `C6` | 10 µF X7R + 100 nF | `3V3REG` | `GND` | short leads — the electrolytic’s ESR cannot supply the first microseconds |
| `X2` | 3-way block | DS18B20 | `3V3REG`, `OW_DATA`, `SWGND` | **SWGND** |
| `X3`/`X4` | 2 × 4-way block | Modbus probes | `+12V`, `GND`, `A`, `B` | 12 V |
| `U4`/`U5` | 2 × BE-IVI isolator, socket | `ISO_VIN`, **`SWGND`**, `SDA`, `SCL` | isolated 3.9 V, data | **MCU-side ground on the island, not on `GND`** — even though the supply comes from the cell. Measured on the hand-wired node: grounding it to `GND` instead **added 0.4 mA**, because the pull-ups on the permanently live rail then drive current through the unpowered isolator’s input clamps. On the island its ground floats up with them. |
| `U4`/`U6` | EZO-pH / EZO-EC, socket | isolated side of `U3`/`U5` | probes | **ISO1 / ISO2** |
| `R13–R16` | 4 × 4.7 kΩ | isolated `SDA`/`SCL` | isolated 3.9 V | **ISO1 / ISO2** — measure first |
| `J4` | SMA jack | pH probe | `U4` `PRB` / `PGND` | **guarded** |
| `X5` | 2-way block | EC probe | `U6` probe pins | not polarised |

## Section 06Layout rules

1. **Three isolated copper groups, and the DRC will complain about all three.** `SWGND`, `ISO1_GND` and `ISO2_GND` each form an island with no path to `GND`. Declare them as separate net classes so the check reports them as expected rather than as errors, and note it on the fabrication drawing.
2. **A routed slot under each isolator**, 2 mm wide, running the full width of the module's isolation barrier. The ADM3260 inside provides the isolation; the slot makes it visible and stops anyone routing a trace across it later.
3. **Guard the `PRB` net.** The pH electrode is a gigaohm source. Keep the trace from J4 to the EZO pads under 15 mm, ring it with a guard track tied to `PGND` (the reference electrode, *not* ground), and place a soldermask and paste keepout around it so no flux residue bridges it. Specify a no-clean process or a wash, and say so on the assembly drawing — a fingerprint across this net is a few hundred megohms and shifts the reading visibly.
4. **`C3` within 10 mm of the U3V9F12's `VOUT`**, and `C5`/`C6` within 10 mm of J2. Both were fitted on the hand-wired node to cure real faults; long traces give the inductance back.
5. **The RS485 pair is a pair.** Route `RS485_A` and `RS485_B` together at constant spacing from U2 to X3 and X4, with `RT1` at the transceiver end. No termination at the probe blocks — the single terminator is the whole point.
6. **Test points on every rail:** `+VBATT`, `V12IN`, `+12V`, `3V3P`, `SWGND`, `ISO_VIN`, both isolated 3.9 V rails, `RS485_A`, `RS485_B`. This node gets probed constantly; make it cheap.
7. **Two layers are enough.** Bottom is a `GND` pour with the three islands cut out of it; top carries signals and the switched rails.

## Section 07What the board is expected to save

Two of the four traps are also current draws, and both are continuous rather than per-cycle. The figures below are the measured drains that the design removes, not a modelled budget.

RS485 fail-safe bias, pull-down returned to `SWGND`−2.23 mA

Two #2810 indicator LEDs, modules removed−1.6 mA

Continuous saving−3.8 mA

That is about 91 mAh per day against a 3400 mAh cell, or roughly the difference between a fortnight of autonomy and a month. The number to check it against is a sleep-current measurement, which has still not been taken on the final topology — with the FTDI removed, since it feeds the battery domain and hides exactly this class of fault.

## Section 08Footprints

> [!WARNING]
> **A correction to Rev B**
>
> Rev B gave **20.16 × 13.97 mm** as the isolator's outline. **It is not.** That is the *EZO circuit* drawing, which appears in the isolator datasheet as well because the two boards mate — and it was read as the isolator's own. The isolator's outline is still unmeasured.

| Part | Outline | Pins | Source |
| --- | --- | --- | --- |
| `U6 U7`EZO-pH, EZO-EC | **20.16 × 13.97 mm**0.79″ × 0.55″ | **Two 2.54 mm rows, 17.78 mm (0.7″) apart**, holes **∅1.05 mm**. One row `GND TX RX`, the other `VCC PRB PGND` | `settled` both Atlas datasheets |
| `U1`Pololu U3V9F12 | 13.6 × 8.6 × 2.8 mm | 3 pins at 0.1″: **1 = VOUT, 2 = GND, 3 = VIN** | `settled` Pololu + the board |
| `U3`MAX3485 module | **to measure** | Front row 1 × 4 at 2.54 mm, left to right **`VCC TXD RXD GND`**. **The A/B side is still unknown** — screw block or header, and how many ways | partly from the board in hand |
| `U4 U5`BE-IVI isolator | **to measure** | MCU side `3.0–5.0V` · `data A` · `data B` · `on/off` · `GND`; isolated side `3.9V` · `data A` · `data B` · `GND` | pin names settled, geometry not |
| `J7`pH connector | fit an **SMA jack directly** | centre → `PRB`, shell → `PGND` | see the note below |

> [!NOTE]
> **Do not use the SMA breakout board here**
>
> Atlas sells an **SMA Breakout Board**, 18 × 27.5 × 1.7 mm with a 6.2 mm mounting hole and `PRB`/`PGND` pads. It exists for people who have no PCB of their own. Using it on the carrier would put the gigaohm node on a flying lead *between two boards*, which is the one place this design cannot afford one. **Fit the SMA jack on the carrier itself** and keep the trace under 15 mm with a guard ring.

## Section 09Placement plan

**Target 100 × 100 mm.** That is JLCPCB's cheapest tier, and everything fits inside it comfortably — so there is no reason to design a board that costs more per piece than it needs to.

![Placement plan, top view, drawn to scale on a 100 by 100 millimetre board. Along the bottom edge sit the five pluggable headers to the WaziSense, in the same left to right order as the WaziSense's own terminal blocks. The left third holds the power section: cell terminal, PTC, diodes, the two high-s](img/sensor-carrier-07.svg)

*Drawn to scale at 5 px per millimetre. Dry connectors on the bottom edge, wet ones on the top, so no cable crosses the board and the two cable bundles leave the enclosure at opposite ends.*

  ### The five decisions this plan encodes

1. **The WaziSense headers keep the WaziSense's own order.** `SW_1`, `ANALOG_PINS`, `DIGITAL_PINS`, `I2C_PORT`, `BAT_IN` left to right — so the loom is five straight jumpers with no crossovers, and a mis-plug is visible.
2. **Wet and dry on opposite edges.** Probe terminals, the DS18B20 and the SMA on the top; everything that goes to the WaziSense on the bottom. The glands then sit at one end of the enclosure.
3. **The `SWGND` island is one contiguous region in the middle**, not scattered. Everything that returns to it lives inside that rectangle, which makes the pour a single shape and the rule visually checkable.
4. **Each isolated chain is a column with the slot across it.** The EZO sits on the far side of the slot from its isolator, so the isolated pour is the region beyond the slot and nothing has to be routed back across.
5. **The SMA sits beside the EZO-pH, on the board edge.** That is what keeps the `PRB` trace under 15 mm — it is the one dimension in this layout that is not negotiable.

## Section 10Footprint coordinates

All four library parts, complete. Origin at the body centre, long axis along X, millimetres. Pad ∅1.8 mm on a 1.05 mm drill throughout.

### EZO circuit — `U6`, `U7`

```
Body 20.16 × 13.97        rows 17.78 apart, 2.54 pitch

1  GND       -8.89  +2.54        4  VCC       +8.89  +2.54
2  TX/SDA    -8.89   0.00        5  PRB       +8.89   0.00
3  RX/SCL    -8.89  -2.54        6  PGND      +8.89  -2.54
```
### Isolator — `U4`, `U5`

```
Body 20.00 × 14.50 (measured)   rows 17.78 apart, 2.54 pitch

1  VCC       -8.89  +5.08        6  3.9V      +8.89  +5.08
2  OFF       -8.89  +2.54        7  N/C       +8.89  +2.54
3  GND       -8.89   0.00        8  GND       +8.89   0.00
4  A         -8.89  -2.54        9  A         +8.89  -2.54
5  B         -8.89  -5.08       10  B         +8.89  -5.08
```
### Pololu U3V9F12 — `U1`

```
Body 13.60 (Y) × 8.60 (X)   pins on the short edge, 1.3 mm in from it

1  VOUT      -2.54  -5.50
2  GND        0.00  -5.50
3  VIN       +2.54  -5.50
```
### MAX3485 module — `U3`

```
Four pads, 2.54 pitch, in a line. The module stands on them.

1  VCC       -3.81   0.00
2  TXD       -1.27   0.00
3  RXD       +1.27   0.00
4  GND       +3.81   0.00
```

> [!WARNING]
> **Two things about this part**
>
> **Mark pin 1 on the silkscreen.** The order along the row is fixed, but which end you turn it towards is decided at assembly — and `VCC` swapped with `GND` destroys the transceiver on first power-up.
>
> **It is 53 mm tall.** That is an enclosure dimension, not a board one. Check it against your housing before you commit to this module.

## Section 11Preliminary placement

Origin at the bottom-left corner of a 100 × 100 mm board, coordinates are component centres in millimetres, all parts on the **top** layer.

**What has been checked, and what has not**

Every pair of bounding boxes was tested for overlap, every part for board fit, and every part against the four mounting holes and the two slots. **Seven collisions were found and fixed** before this table was written; it now reports clean.

What that does *not* prove: that the routing works, that the pours have room, or that the footprint sizes used for the check match the parts you will actually draw. Treat it as a starting arrangement to nudge, not as a finished layout.

| Ref | X | Y | Note |
| --- | --- | --- | --- |
| `X2` | 20 | 94 | DS18B20, 3-way 3.5 mm |
| `X3` | 38 | 94 | probe 1, 4-way: +12V GND A B |
| `X4` | 56 | 94 | probe 2, 4-way |
| `F1` | 24 | 84 | PTC 2.6 A, 1812 |
| `D1` | 31 | 84 | SS34, SMA |
| `U4` | 76 | 84 | isolator, pH chain |
| `X1` | 12 | 80 | cell in, 5 mm 2-way screw |
| `D2` | 31 | 78 | SMBJ5.0A, SMB |
| `C1` | 23 | 74 | 100 µF |
| `U3` | 52 | 72 | MAX3485 — 4 pads. Body 22 mm wide, **53 mm tall** |
| `Q1` | 12 | 68 | AO3401A — switch A |
| `Q2` | 18 | 68 | 2N7002 |
| `C3` | 24 | 68 | 100 nF soft start |
| `R1` | 12 | 64 | 100 kΩ |
| `R2` | 18 | 64 | 10 kΩ |
| `R3` | 24 | 64 | 100 kΩ |
| `J8` | 62 | 64 | **A / B down from the module’s screw terminal** |
| `RT1` | 48 | 62 | 120 Ω terminator |
| `R7` | 53 | 62 | 680 Ω bias pull-up → 3V3REG |
| `R8` | 58 | 62 | 680 Ω bias pull-down → **SWGND** |
| `U6` | 76 | 62 | EZO-pH |
| `J7` | 93 | 62 | SMA — PRB run is 8.1 mm |
| `U1` | 14 | 56 | U3V9F12 |
| `C4` | 28 | 56 | 470 µF — 9 mm from U1 |
| `C9` | 52 | 52 | 10 µF 0805 |
| `C10` | 56 | 52 | 100 nF |
| `Q3` | 12 | 44 | AO3401A — switch B |
| `Q4` | 18 | 44 | 2N7002 |
| `C5` | 24 | 44 | 100 nF soft start |
| `C8` | 38 | 44 | 470 µF — at the LDO output |
| `R11` | 48 | 44 | 47 kΩ **DNP** |
| `R12` | 53 | 44 | 47 kΩ **DNP** |
| `U5` | 76 | 44 | isolator, EC chain |
| `R4` | 12 | 40 | 100 kΩ |
| `R5` | 18 | 40 | 10 kΩ |
| `R6` | 24 | 40 | 100 kΩ |
| `U2` | 12 | 32 | MCP1700 — fit **either** this |
| `C6` | 18 | 32 | 1 µF |
| `C7` | 24 | 32 | 1 µF |
| `JP1` | 12 | 28 | 0 Ω — **or** this |
| `U7` | 76 | 22 | EZO-EC |
| `X5` | 93 | 22 | EC probe, 2-way |
| `J2` | 20 | 7 | SW\_1 — SW\_OUT / D6(−) |
| `J3` | 34 | 7 | ANALOG — A2 / A1 / GND |
| `J4` | 50 | 7 | DIGITAL — D4 / D3 / GND |
| `J5` | 68 | 7 | I2C\_PORT — SCL / SDA / D5 / GND |
| `J6` | 86 | 7 | BAT\_IN — BATT / GND |

### Slots, holes and the fixed points

| Feature | Geometry | Note |
| --- | --- | --- |
| Slot, pH chain | x 65–89, y 71.8–74.2 | 2.4 mm wide — between `U4` and `U6` |
| Slot, EC chain | x 65–89, y 31.8–34.2 | between `U5` and `U7` |
| Mounting holes | (5,5) (95,5) (5,95) (95,95) | M3, ∅3.2 mm, 3.2 mm keepout |
| `PRB` run | 8.1 mm | `U6` pin 5 at x 84.89 → `J7` at x 93. Limit 15 mm, and it must not cross the slot |

> [!NOTE]
> **Why the two electrode cables leave on the right**
>
> `J7` and `X5` sit on the right edge rather than the top with the others. That is forced: the `PRB` net must stay short *and* stay on the EZO’s side of the slot. Moving the SMA to the top edge would send it across the barrier. So the enclosure gets two gland groups — three cables on top, the two electrode cables on the right, on the shortest path.

## Section 12Confirm before routing

These are the dimensions I could not verify. Each has to come from the manufacturer's drawing before a footprint is drawn, and none of them changes the schematic.

| Item | What is needed | Where from |
| --- | --- | --- |
| WaziSense v2 | `settled` Outline, mounting holes, every pad centre and every net — see Section 04 | From `PickAndPlace_PCB.csv`, `Gerber_PCB.zip` and the EasyEDA source in `Source_PCB.zip` |
| Atlas EZO circuit | **Pin functions settled** from the datasheet artwork: two opposite edges, `GND` · `TX/SDA` · `RX/SCL` on one and `VCC` · `PRB` · `PGND` on the other. Outline and pitch still open. | The datasheet is an 80-page image PDF with no dimensioned drawing in it. **Measure the circuit you have** — that is authoritative for your revision anyway. |
| Basic EZO Inline Voltage Isolator | Footprint, pin order both sides, barrier position for the slot | Its datasheet URL returns a server error. Measure the two you have fitted. |
| Pololu U3V9F12 | `settled` **13.6 × 8.6 × 2.8 mm**, three pins at 0.1″: **1 = VOUT, 2 = GND, 3 = VIN** | Dimensions from Pololu, pin order read off the board |
| MAX3485 module | Footprint — these vary between sellers | Measure the one you have |
| Enclosure | Board outline, mounting holes, gland positions for the six probe cables | Your choice of housing |

> [!NOTE]
> **On LCSC part numbers**
>
> The BOM gives manufacturer part numbers rather than LCSC codes on purpose. JLCPCB's stock and basic-part list change often enough that a code written today may be an extended part or out of stock by the time you order, and an extended part carries a setup fee per line. Pull the codes from their parts library at order time and prefer whatever is marked *basic*.

### What else the manual settled

None of these change the schematic, and two of them change how the node should be run.

| From the manual | What follows |
| --- | --- |
| Sleep current **29 µA** with the solar circuit active, **6.5 µA** without | The 3.2 mA measured at the time was about **110×** the board’s own figure, and it is 1.08 mA today. Whatever is drawing it is external, which is what the bias-network finding in Section 02 says it is. |
| The HE2031A33 regulator needs **3.5 V minimum input** | A cell at 3.2 V is already below it. That, not the radio, sets the floor for a low-battery cut-off. |
| “allow a delay of at least **1500 ms**” after switching a sensor rail on | `RAIL33_SETTLE_MS` is **1200**. It works, but it is under the manual’s figure — worth raising if anything on that rail ever misbehaves at start-up. |
| `A3`, `A6` and `A7` are brought out as pads in the prototyping area | Two more analog inputs than this build assumed. `A6`/`A7` are **input-only** on this part, so they cannot gate a switch — but they can read one more sensor each. |
| Square holes in the prototyping area are **ground**; the rest are unconnected | Useful if anything has to be bodged onto the WaziSense itself. |
| `I2C_J` is closed from the factory with 2 × 4.7 kΩ | Confirms the open jumper on this unit is a **board fault**, not a documentation error. Worth checking on every board of a batch. |
| Cutting `MI_J`, `MO_J`, `SCK_J` frees the SPI header | Needed to flash a bootloader. Do not cut them otherwise — the radio lives on that bus. |

## Section 13Bring-up order

The first board gets tested before any module is fitted. Every step below can only fail in one way, which is the point of the order.

1. **Bare board, meter only.** `SWGND` to `GND` must read **open**. `ISO1_GND` and `ISO2_GND` to `GND` must read **open**. If any of them reads a few ohms, the pour was stitched and the board is wrong — stop here.
2. **Cell only, nothing else fitted.** `+VBATT` should read cell voltage minus roughly **20 mV** across `Q5` — at a light load it is hard to measure at all. Reversed, it should read zero and nothing should get warm. **Correction:** this step used to say “minus about 0.4 V across the SS34”. That was right for Rev A; on a board with `Q5` a 0.4 V drop would mean the FET is in backwards.
3. **WaziSense connected, no other modules.** It should boot and blink. Drive `D5` and `A1` high by hand and watch `V12IN` and `ISO_VIN` follow, then fall when driven low.
4. **Fit U1.** `+12V` must be 12 V ± 0.5 V under no load, and stay above 11 V with a 250 mA dummy load. This is the step that catches a converter like the XL6009 that put out 30 V from a 3.6 V input.
5. **Fit U2 and the DS18B20.** Check that with `D6` low, the current into `SW_1` pin 1 is microamps, not milliamps. This is where a stitched `SWGND` would show up if step 1 was somehow passed.
6. **Fit the isolators, then the EZO circuits.** Power the isolators with the EZO circuits unplugged and measure the isolated data lines against isolated ground: about 3.9 V means `R13–R16` are right, near zero means they are missing and `Wire` will hang with no message.
7. **Probes last**, one at a time, checking each reads before adding the next.

> [!NOTE]
> **What does not change**
>
> No firmware change is needed for this board. The pin map, the rail sequencing with the isolators up first and down last, the soft start on `D6` and the turbidity retry all apply unchanged. The board is an interconnect, not a redesign.
>
> ## Section 14Bill of materials
>
> > [!WARNING]
> >  **Superseded — this is the planning BOM**
> >
> >  The tables below are the BOM as planned before layout and are kept for the reasoning. They are **not** what was ordered: on the Rev C order **everything is assembled by JLCPCB**, through-hole included, there is no `X5`, `X1` is a 3.5 mm block, `X3`/`X4` are each two ganged 2-pole blocks, and every part has an LCSC code. The ordered BOM is `pcb/BOM.csv` and `pcb/out/FACTS.md`.
> >
> >
>
> Manufacturer part numbers rather than LCSC codes, for the reason in Section 08: JLCPCB's stock and its *basic* list move, and an extended part carries a setup fee per line. Pull the codes at order time and prefer whatever is marked basic. Quantities are per board.
>
> ### Assembled by JLCPCB — all SMD, all common
>
> | Ref | Qty | Part | Package | Note |
> | --- | --- | --- | --- | --- |
> | `Q1 Q3` | 2 | AO3401A | SOT-23 | P-channel, −30 V, 4 A. The high-side pass element |
> | `Q2 Q4` | 2 | 2N7002 | SOT-23 | level shifter for the gate |
> | `U2` | 1 | MCP1700T-3302E/TT | SOT-23 | **Fit *or* `JP1`, never both.** 1.6 µA quiescent |
> | `JP1` | 1 | 0 Ω | 0603 | the alternative to `U2`, for `H1` on 3.3 V |
> | `Q5` | 1 | AO3401A | SOT-23 | ideal diode, **not** a Schottky. Drain to the cell, source to `+VBATT` — source-to-cell leaves the body diode conducting on a reversed cell. *Was `D1`, an SS34 in SMA.* |
> | `R13` | 1 | 100 kΩ | 0603 | holds `Q5`’s gate at `GND`. Without it the node runs through the body diode at 0.8 V |
> | `D2` | 1 | SMBJ5.0A | SMB | TVS, cathode to `+VBATT` |
> | `F1` | 1 | PTC, 2.6 A hold | 1812 | resettable — no field visit to change a fuse |
> | `R1 R3 R4 R6` | 4 | 100 kΩ 1 % | 0603 | gate pull-up and pull-down, both stages |
> | `R2 R5` | 2 | 10 kΩ 1 % | 0603 | GPIO series to the level shifter |
> | `R7 R8` | 2 | 680 Ω 1 % | 0603 | RS485 fail-safe bias. **`R8` returns to `SWGND`** |
> | `RT1` | 1 | 120 Ω 1 % | 0603 | the only terminator on the bus |
> | `R9` | 1 | 4.7 kΩ 1 % | 0603 | 1-Wire pull-up — **mandatory** |
> | `R11 R12` | 2 | 47 kΩ 1 % | 0603 | **DNP.** The isolators carry their own 4.7 kΩ on both sides; fit these only as insurance against a missing isolator |
> | `C2 C3 C5 C10` | 4 | 100 nF X7R 50 V | 0603 | `C3`/`C5` are the soft starts |
> | `C6 C7` | 2 | 1 µF X7R 16 V | 0603 | required by the MCP1700 |
> | `C9` | 1 | 10 µF X7R 16 V | 0805 | the fast half of the bulk |
>
> ### Fitted by hand — modules, electrolytics and connectors
>
> | Ref | Qty | Part | Note |
> | --- | --- | --- | --- |
> | `U1` | 1 | Pololu U3V9F12 | 13.6 × 8.6 mm, 3 pins at 0.1″, **1 = VOUT, 2 = GND, 3 = VIN** |
> | `U3` | 1 | MAX3485 auto-direction module | Kept as a module: a bare MAX3485 needs `DE` driven, and there is no spare GPIO. Footprint must be measured |
> | `U4 U5` | 2 | Atlas Basic EZO Inline Voltage Isolator | 20.16 × 13.97 mm, 1 mm holes. `EN` left open |
> | `U6 U7` | 2 | Atlas EZO-pH, EZO-EC | Footprint still to be measured. Pins: `GND TX RX` on one edge, `VCC PRB PGND` on the other |
> | `C1` | 1 | 100 µF 10 V electrolytic | on `+VBATT` |
> | `C4` | 1 | 470 µF 25 V electrolytic | on the 12 V rail, within 10 mm of `U1` |
> | `C8` | 1 | 470 µF 10 V electrolytic | the cold-start brownout reserve |
> | `X1` | 1 | 2-way screw terminal, 5 mm | cell |
> | `X2` | 1 | 3-way terminal, 3.5 mm | DS18B20 |
> | `X3 X4` | 2 | 4-way terminal, 3.5 mm | one block per Modbus probe, deliberately not shared |
> | `X5` | 1 | 2-way terminal, 3.5 mm | EC probe, not polarised |
> | `J7` | 1 | SMA jack, edge or vertical | pH probe. **Guard ring to `PGND`**, trace under 15 mm |
> | `J2–J6` | 5 | 3.5 mm pluggable headers, 2/3/3/4/2-way | to the WaziSense — see Section 04, option B |
>
> > [!NOTE]
> >  **What a batch of ten actually costs you in effort**
> >
> >  Every part in the first table is a jellybean that JLCPCB will place for you. Everything in the second is through-hole and goes in by hand — which for ten boards is an evening, and which you would be doing anyway, because the Atlas modules and the Pololu regulator are not in anyone's pick-and-place library.
> >
> >  **Order a few spares of `Q1`–`Q4` and the 0603 values.** They cost cents and the alternative is a second shipment for one resistor.
> >
> >
