# KijaniSpace Carrier — Rev D (laid out, not yet ordered)

> **Start at [`../../README.md`](../../README.md)**. The board as built is
> [`../revC/`](../revC/README.md), and its README documents the generator, the
> checks and the fabrication files; all of that applies here unchanged. This
> folder is a self-contained copy of that generator: `bash gen/build.sh` and
> `bash gen/fab.sh` here only ever write this folder.

## What Rev D changes

The two plug-in modules that are not Atlas parts are now on the board, so the
assembler fits them and only the Atlas circuits and isolators are left to plug in:

| Rev C | Rev D | Why |
| --- | --- | --- |
| RS485 module (MAX13487) on a socket at `U3`, linked to the bus by the terminal `J8` | **THVD1406** transceiver at `U3`, on the board; `J8` is gone | the MAX13487 is specified for 4.75–5.25 V and ran here at 3.3 V; module `TXD`/`RXD` labels differ between batches (one carrier needed its `J4` wires crossed); the module is 53 mm tall |
| Pololu U3V9F12 boost in socket `U1` | **TPS61085** boost (`U8`) on the board; **`U1` stays as a fitted fallback socket**, selected by the solder jumper `JP3` | the module was most of the module cost per board; the socket keeps a known-good way out |
| R7/R8 680 Ω | **470 Ω** | the module's own fail-safe bias is gone, see below |
| — | **SM712** TVS (`D3`) on A/B at the probe terminals | the long cable to the lake |
| transceiver and bias between `3V3REG` and `SWGND` | **between `3V3RS` and `GND`**: `Q6` (AO3401A) switches `3V3RS` from `3V3REG` while `SWGND` is low, `R17` (100 k) holds it off otherwise | in sleep the bus floated up to 3.3 V into the unpowered probes: **0.5 mA per probe**, measured on Rev C (see *Sleep current*) |
| WaziSense beside the carrier | **WaziSense stacked on top**, on three M2.5 standoffs at `H5`–`H7` | both boards take one footprint in the enclosure (see *WaziSense on top*) |

**One firmware change, safe on both carriers:** `parkPinsForSleep()` leaves
D3/D4 as plain inputs instead of with pull-ups (Rev D's unpowered transceiver
would otherwise be fed through `R`). D3/D4, D5, D6 and A1 keep their roles and
timing.

### Pin-out decisions, from the datasheets

- **THVD1406** (SLLSF87A): 1 R, 2 RE, 3 SHDN, 4 D, 5 GND, 6 A, 7 B, 8 VCC.
  `RE` and `SHDN` are tied to VCC: that is the auto-direction mode in TI's own
  application section. In that mode the receiver output is high-impedance
  while the chip drives, so the node hears no echo of its own frame -
  `RS485_ECHOES_OWN_TX` stays 0.
- **TPS61085** (SLVS859B): 1 COMP, 2 FB, 3 EN, 4 PGND, 5 SW, 6 IN, 7 FREQ, 8 SS.
  Rev D copies TI's 3.3 V → 12 V example at 1.2 MHz: `FREQ` and `EN` tied to
  `IN`, 3.3 µH, Schottky to the output, two 10 µF out, 10 µF + 1 µF in,
  47 kΩ / 1.5 nF compensation (TI's table: 47 k / 1.6 nF). One deliberate
  departure: **1 µF soft start instead of 100 nF**, because this output also
  carries `C4` (470 µF, the buffer for the Y511-A's wiper motor), which TI's
  example does not. See *Review* below.
  Feedback 200 k / 22 k: **12.49 V** (1.238 V × (1 + 200/22)), 56 µA through the
  divider. Both probes are inside their range: SEN0680 10–30 V, Y511-A 12–24 V
  - the Pololu's 12 V ±4 % was the one that could dip below 12.

### Why the bias went to 470 Ω

The THVD1406 drives a logic 1 actively for only 4–14 µs after each rising edge
(datasheet, `t_device_auto-dir`); after that the bus is held by the fail-safe
bias. At 4800 baud a bit lasts 208 µs, so the bias carries almost all of every
1. The MAX13487 worked the same way, but its module added its own bias on top
(A–B measured 0.36 V against 0.27 V for the bare network). With 470 Ω the idle
A–B is about **0.37 V**, well above the 0.2 V a receiver needs. The bias sits
on the switched `3V3RS`, so it draws nothing in sleep. TI E2E has reports of
trouble at low baud rates that fit a weak bias; this is the first thing to
check on the first board.

### The fallback, and why both converters can never run at once

```
  +VBATT -- Q1 (switched by D5) -- V12IN -- JP3 centre
                                             +-- pad 1 (joined to the centre by a trace) -- U8 TPS61085 -- D4 --+
                                             +-- pad 3 (open) ---------------------- U1 socket VIN            |
                                                                                     U1 socket VOUT ----------+-- +12V -- C4 -- probes
```

- `V12IN` reaches the on-board converter **or** the Pololu socket's `VIN`,
  never both. Fallback: cut the trace between pads 1 and 2, bridge 2–3, plug
  the Pololu into `U1`.
- A Pololu plugged in with JP3 at 1–2 gets no input and does nothing.
- In fallback, `D4` blocks the Pololu's 12 V from back-feeding the dead
  converter.
- JP3's footprint numbers its pads; the legend on the board explains them.

### Sleep current

**What Rev C does.** Its transceiver and bias sit between `3V3REG` and `SWGND`.
In sleep `D6` lets `SWGND` go, and the bias pulls it - and both bus lines - up
to 3.3 V. The probes are unpowered, but their receiver inputs are referenced
to `GND`, so each line pushes current into each probe. Measured on a Rev C
carrier, 4 Nov 2026, DO probe on the bus, no turbidity probe:

| Sleep, cell side | |
| --- | --- |
| probe cable connected | **0.53 mA** |
| only its A/B wires out of `X3B` | **0.03 mA** (reproducible) |

0.50 mA for one probe, against 2 lines × 3.3 V / 12 kΩ (one unit load) =
0.55 mA predicted. Two probes make about 1.1 mA, the 1.08 mA the hand-wired
node showed: **about 26 mAh a day, more than thirty times the rest of the
node's sleep current.**

**What Rev D does.** The transceiver, `C11` and the bias run from `3V3RS`,
which `Q6` switches from `3V3REG`; its gate is `SWGND`. `D6` on pulls `SWGND`
low and turns `Q6` on (V<sub>GS</sub> −3.3 V); `D6` off lets `R17` pull the gate to the
source and `Q6` turns off. `R8` and the transceiver's ground go to `GND`. In
sleep `3V3RS` drains through `R7`-`RT1`-`R8` and **A and B rest at 0 V**, where
the dead probes are:

| Part | In sleep |
| --- | --- |
| TPS61085, its divider and inductor | behind `Q1`: no supply at all |
| THVD1406, bias, `C11` | `3V3RS` at 0 V; bus at 0 V; THVD1406 logic inputs take up to 5.7 V unpowered (abs. max. independent of VCC) |
| `Q6` | off, V<sub>GS</sub> 0; leakage < 1 µA |
| `R17` | `SWGND` sits at `3V3REG` through it; nothing on `SWGND` (isolators, DS18B20) has a path to `GND` while `D6` is off, so no current |
| SM712 | 0 V |

`R17` costs 33 µA only while `D6` is on. The firmware must not hold D3 high
in sleep (see above): through the dead chip's `R` output that would be up to
0.1 mA.

Keep the isolators fitted. With them bridged, the EZO probes sit on `SWGND`,
and if they share the water with a probe whose housing or reference is on
`GND`, the water itself can pull `SWGND` down and switch `Q6` on in sleep.

## Layout

- **Transceiver** where the module's pin row was, rotated so that `D` and `R`
  face `J4` and `A`/`B` face the bus resistors. `3V3RS` runs under its body
  between the two pad rows, from `Q6` just west of it; `C11` (100 nF) sits
  beside `VCC`. Its ground, `C11`'s and `R8`'s go to vias into the GND pours.
  `R7`'s top takes `3V3RS` down the west side of the body.
- **`Q6`/`R17`** between the `+VBATT` run at x 61 and `RS_TX`. The gate's
  `SWGND` drops from the y 22 trunk on the back at x 60.25 (clear of `J3`'s
  and `J4`'s pads). `3V3REG` reaches `R11`/`R12` on the back at y 42.4,
  between the `D5` lane (y 43) and the `SWGND` via at (73.5, 41.5), because
  the run under the transceiver is `3V3RS` now.
- The `SWGND` trunk crosses the `D5` lane (back layer, y 43) by hopping over
  on the front between two vias, then turns for the isolators. The `SWGND`
  pour of Rev C is gone: nothing that needed it is left on `SWGND`.
- **Boost** in the empty area above the probe terminals, right beside the
  `+12V` run it feeds. The switching loop - `SW`, `L1`, `D4`, `C14`/`C15`,
  `PGND` - is one tight ring on the front layer below the IC; soft start,
  compensation and divider sit above it. Its input comes from `JP3` by the
  Pololu socket: down to the back at x 43 and across at y 84, the only place
  where the back layer is clear of the `+VBATT`, `+12V` and `ISO_VIN` lanes.
- `JP3` sits just east of the Pololu module's outline, not under it.

## WaziSense on top

The WaziSense v2 sits above the carrier on three **20 mm M2.5 standoffs**,
component side up and turned 180°, its origin at KiCad (66.5, 45.25). Its
outline covers x 48.2–118.6, y 31.0–83.9: the isolators, `C4`/`C8`, the
transceiver and the legend lie under it; the J-row, the probe terminals, the
EZO sockets, `X1` and the boost block stay outside. Its edge stops about 4 mm
short of the screws of the carrier's J-row.

| Hole | KiCad (mm) | What had to move |
| --- | --- | --- |
| `H5` | (113.49, 54.39) | nothing; the last legend lines are shorter now |
| `H6` | (58.12, 45.00) | `+VBATT` from `Q5` drops at x 54.3 instead of 56; the feed to `J6` leaves it at y 37 instead of the y 48 rail; `D5` (back) loops south of the hole at y 48.8 |
| `H7` | (61.67, 81.45) | `BOOST_IN` surfaces at (56.5, 84) and goes north round the hole at y 77.4 into `C12`; the boost block's row of labels closed up west of it, same order |

- **M2.5, 2.7 mm holes.** The WaziSense's own holes are 3.0 mm (read off its
  Gerbers), too tight for M3.
- **Bare board round each hole.** No track or via of any net within 3.2 mm of
  a centre, on either layer: the standoff's hex foot (5 mm across flats,
  2.9 mm to a corner) on the front, the screw head on the back. `check.py`
  checks it (section 6) and also fails if there are not exactly three.
- **Height.** The tallest parts under the WaziSense are `C4`/`C8` (470 µF,
  10 × 13 mm) and the isolators in their sockets. 20 mm standoffs leave about
  5 mm over the capacitors once the WaziSense's own pin ends (about 2 mm) are
  counted. Measure an isolator in its socket before buying the standoffs.
- **The J-row keeps its order.** Turning the WaziSense mirrors its terminal
  order against the carrier's, so the wires between the blocks cross. A
  reorder was tried and dropped: `J2` (SW_1) would land on `H4`, and `SWGND`,
  `3V3P`, `A1` and `OW_DATA` would have to cross the whole board to the
  circuits on the west side they serve. **Wire the carrier's terminals first,
  then mount the WaziSense.** Within each block the pins keep their order, so
  every wire of a block has the same length:

| Carrier | WaziSense | Across | Straight line with the 20 mm rise |
| --- | --- | --- | --- |
| `J2` | SW_1 | 77.5 mm | 80 mm |
| `J3` | ANALOG | 48.9 mm | 53 mm |
| `J4` | DIGITAL | 23.7 mm | 31 mm |
| `J5A`/`J5B` | I2C | 15.3 mm | 25 mm |
| `J6` | BAT_IN, on the WaziSense's far edge | 52–54 mm | 56–58 mm |

Cut each wire with enough slack to lift the WaziSense off for service.

## A generator bug Rev D fixed

`mkpcb.py` wrote every pad with its library angle. A board file stores pad
angles **absolute**, so a rotated footprint kept its pads in library
orientation. On round and square pads that is invisible; on Rev C it affected
only `R13` (0603, rotated 270°), whose near-square pads came out turned 90° -
harmless. On Rev D's SOIC it would have made neighbouring pads overlap. The pad
angle now gets the footprint rotation added. The same bug was the cause of the
14 `lib_footprint_mismatch` warnings in Rev C's DRC; Rev D has none.

`check.py` and `pads.py` now use each pad's real rectangle (rotation included)
instead of a circle of its long half-side, and accept T-junctions. The circle
called neighbouring SOIC pads 0.6 mm apart a collision.

## State

| Check | Result |
| --- | --- |
| `check.py` (endpoints, pad/track/via clearance, barriers, bare board round the three WaziSense standoffs) | clean |
| `pour.py` | every GND pad reachable through the pour |
| KiCad DRC, errors | **0 violations, 0 unconnected** |
| KiCad DRC, warnings | the same 21 silkscreen touches between ganged terminal pairs as Rev C (intended); nothing else |
| `silkcheck.py`, `cplcheck.py`, `vendorcheck.py`, `bomsync.py`, `fabcheck.py`, `zipcheck.py` | pass |
| `netcheck.py` (new: firmware pins, both chips pin by pin, SWGND, the switched bus supply, JP3, probe supply) | 94 requirements hold |
| `out/FACTS.md` | 75 footprints (72 before the WaziSense standoff holes `H5`–`H7`), 68 placed parts, 30 BOM lines, 7 Extended SMT types |

Parts read off JLCPCB's listing on 4 Nov 2026, all marked "Economic and
Standard": THVD1406DR `C5215918` (5,827 in stock), TPS61085DGKR `C113659`
(26,333), SWPA4030S3R3MT `C15269` (21,889, Isat 3.6 A), SM712 `C293966`
(121,381), 1.5 nF `C1595`; Basic: SS14 `C2480`, 10 µF 50 V 1206 `C13585`,
47 k `C25819`, 200 k `C25811`, 22 k `C31850`, 470 Ω `C23179`. `Q6` and `R17`
join existing BOM lines (AO3401A `C15127`, 100 k `C25803`): no new line, no
new feeder.

**Cost.** Extended feeders 7 × 3 USD and 78 through-hole joints: about 26 USD
once per order (Rev C: 11 USD). Each board saves the Pololu and the RS485 module,
about 12 € net of the new parts, so Rev D is cheaper from the second assembled
board on.

## Review before ordering (4 Nov 2026)

### Placement rotations, read off the vendor footprints

JLCPCB places by the EasyEDA footprint on each part page. Calibrated on the
AO3401A, whose footprint there is KiCad's SOT-23 turned 180° - the correction
the Rev C placement view had shown:

| Part | JLCPCB footprint against KiCad's | `JLC_ROT_FP` | CPL |
| --- | --- | --- | --- |
| `U3` THVD1406, SOIC-8 | pin 1 bottom left, pins in rows: turned 90° | 270 | 180 |
| `U8` TPS61085, MSOP-8 | the same | 270 | 270 |
| `D3` SM712, SOT-23 | the AO3401A's layout, pin 3 common | 180 (existing) | 90 |
| `D4` SS14, SMA | cathode left, as KiCad | 0 | 0 |
| `L1` SWPA4030 | pads left and right, as KiCad | 0 | 0 |

Still check pin 1 of `U3`/`U8` and `D4`'s band in the placement view.

### Operating states

| State | What Rev D does there | Verdict |
| --- | --- | --- |
| Sleep | `Q6` off, `3V3RS` and the bus at 0 V with the probes; `U8` has no supply (`Q1` off); the firmware parks D3/D4 as plain inputs | Rev C's 0.5 mA per probe gone; expect Rev C's 0.03 mA base |
| 3.3 V rail on (D6) | `Q6` on; THVD1406 1.7 mA + bias 3.1 mA (Rev C: 2.2 mA bias) + `R17` 33 µA | +0.9 mA, sensor phase only |
| 12 V switch-on (D5) | as with the Pololu, which has the same inductor-and-diode path: the cell charges `C4` through `L1`/`D4` the moment `Q1` closes, 8-10 A for 0.1-0.3 ms (model) - Rev C does the same on every cycle without resets. Then the soft start: with 100 nF the current limit reached 2.4 A within 5 ms and held it until `C4` was charged, +VBATT down to 2.5 V with a weak cell at 3.3 V; with **1 µF** about 1.1 A over ~27 ms, +VBATT 2.86 V worst / 3.05 V healthy cell | **`C16` 100 nF → 1 µF** (Basic part already on the BOM) |
| Measuring: 12 V and isolators on | during the wiper sweep the Y511-A draws 1.5–2 W, up to 0.6 A from the cell (measured on the earlier builds). That is ~0.16 A out of `U8` at 12.49 V, against ~0.37 A it can deliver at 3.3 V in (2.0 A minimum switch limit, 3.3 µH, 1.2 MHz). Inductor peak ~1.2 A (`L1` Isat 3.6 A), `D4` 0.16 A average (1 A part), `F1` 2.6 A hold. The wiper's start is buffered by `C4`. 12.49 V against the Y511-A's 12 V minimum (Rev C: 12 V ±4 %, sweeps cleanly) | ok, 2× margin |
| RS485 traffic | driver on within 10 µs of D falling (≤ 10 % of a 9600-baud start bit), drives a 1 for 4-14 µs, then the 0.37 V bias holds it; receiver off while driving, so no echo | ok; first-board gate |
| LoRa TX/RX | the rails are off before the radio sends | ok |
| Cell at 3.0 V | TPS61085 runs to 2.3 V; 0.3-0.4 A output capability at 3.0 V in. Rev C measured, 4 Nov 2026 (bench supply, DO probe only): a full cycle at 3.05 V, bootloop at 3.0 V. The firmware stops measuring below 3.50 V and since that day also checks the cell at start-up, so a restart at an empty cell no longer switches the 12 V rail on | ok, 0.45 V above the floor |
| Full cell / charging | 4.2 V against IN 6 V max | ok |
| USB only, no cell | the battery domain sits at ~2.8 V: the Pololu stopped regulating there, the TPS61085 does not and pulls up to ~1 A from a back-fed domain | bench rule stands: cell connected |
| Fallback, JP3 at 2-3 | `D4` blocks back-feed; `U8` unpowered; FB sees `+12V` through 200 kΩ, tens of µA, IN stays under its 2.3 V start | ok |
| Pololu plugged, JP3 at 1-2 | its VIN is open | does nothing |
| A/B shorted to +12V at a terminal | THVD1406 ±16 V bus fault rating, `D3` | better than Rev C |
| Temperature | THVD1406 −40…125 °C, TPS61085 characterised −40…85 °C, X5R to 85 °C | ok in the enclosure |
| Noise | the boost runs during the pH/EC reads (firmware order), ≥ 20 mm from the pH input, in another ground domain; Rev C's Pololu ran then too | compare on the first board |

### The sleep-current question, answered

The review predicted that the floating bus leaks into the unpowered probes,
about 0.55 mA per probe. Rev C measured 0.50 mA for one probe (*Sleep current*
above), so Rev D's copper changed the same day: `Q6`, `R17`, `3V3RS`, the
transceiver block on `GND`, the `SWGND` pour removed, and the firmware's D3/D4
parking. `netcheck.py` now also holds the rule itself: only `Q6` feeds
`3V3RS`, only the transceiver block is on it, and nothing else touches A or B.

### Before ordering

1. JLCPCB placement view: pin 1 of `U3` and `U8`, cathode band of `D4`, and
   `Q6` with its single pin (drain) towards `U3` (SOT-23 rule, CPL 90).
2. The 1:1 print (`out/KijaniCarrier-top-1zu1.pdf`) with the Pololu and a
   SOIC-8 / SMA part on it.
3. On the same print: lay the WaziSense on it, turned 180°, with its terminal
   row towards the J-row. Its three holes must sit over `H5`–`H7`; they were
   drawn from its Gerbers, not measured on a board.

## On the first board

1. **12 V rail:** 12.4–12.6 V without load and with both probes; ripple on a
   scope if one is at hand; a load step (the Y511-A's wiper) without
   oscillation. `C4`'s 470 µF hangs on the output too, which TI's example does
   not have.
2. **RS485:** `RS485Drive` (idle A–B about 0.37 V, collapsing while driving),
   then both probes at 4800 and 9600 baud with the main firmware.
3. **Sleep current**, everything connected, both probes on the bus: expect
   Rev C's 0.03 mA base, not its 0.53 mA. With the FTDI unplugged. While the
   node boots and sends, short the meter with a clip lead; take the lead off
   only once it sleeps, or the meter's shunt can brown the node out (most
   likely why the Rev C measurement began with five resets in a row). Then `SWGND` with D6 off: about 3.3 V
   (`R17`), and `3V3RS`: 0 V.
4. **Fallback once:** cut JP3 1–2, bridge 2–3, plug the Pololu in - 12 V must
   come back, then restore.
