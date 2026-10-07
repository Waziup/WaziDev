# KijaniSpace Carrier — PCB

Generated for **KiCad 10.0.6**. Open `KijaniCarrier.kicad_pcb`.

## What is in here

| File | What it is |
|---|---|
| `KijaniCarrier.kicad_pcb` | The board: 100 × 100 mm outline, the isolation slot, 4 mounting holes, **51 footprints placed** |
| `KijaniCarrier.kicad_pro` | Project file |
| `KijaniSpace.pretty/` | The four footprints nobody else has: EZO circuit, BE-IVI isolator, Pololu U3V9F12, MAX3485 module |
| `out/drc.txt` | The DRC report as generated |
| `out/view.svg` | Front copper + silkscreen + outline, for a quick look |

## Before you open it: register the library

KiCad does not yet know where `KijaniSpace.pretty` lives, which is why the DRC
report carries 11 library warnings. They are configuration, not design faults.

**Preferences → Manage Footprint Libraries → Project Specific Libraries → add**
`KijaniSpace.pretty` from this folder, nickname `KijaniSpace`.

## Pad order: mirrored on 2026-10-06

The first version had every module's pad row reversed. It was caught by laying a
module on the table exactly as it will sit on the board — component side up,
pins down — and reading the silkscreen left to right. The board now matches:

| Module | row | left → right |
|---|---|---|
| BE-IVI isolator | MCU side (top) | `VCC OFF GND A B` |
| | isolated side (bottom) | `3.9V N/C GND A B` |
| EZO circuit | bus side (top) | `GND TX RX` |
| | probe side (bottom) | `VCC PRB PGND` |
| Pololu U3V9F12 | single row | `VIN GND VOUT` |
| MAX3485 module | single row | `VCC TXD RXD GND` |

**The Pololu row is the one entry not confirmed against a photo.** Its pin
labels are printed on the *back* of the module, so reading them gives the
mirrored order; that is why it was flipped along with the rest. Check it the
same way before soldering. `VCC` and `GND` swapped destroys the transceiver and
both EZO circuits on first power-up.

Each custom footprint carries a 1 mm silkscreen label naming pin 1 — `1:VCC`,
`1:GND`, `1:VOUT` — beside the square pad. That is the marking to check the
real module against.

## State: fully routed, and clean once the zones are filled

**With the pours filled, KiCad reports 0 unconnected, 0 electrical errors and
0 silkscreen errors.** The only thing left is 11 `lib_footprint_mismatch`
warnings: KiCad compares the board's embedded footprints against the library
and the generator writes rotation differently than pcbnew would. The Gerbers
come from the board, not the library, so this does not reach the fabricator.
**Do not run "Update Footprints from Library"** — that is the one action that
could act on the difference and move pads.

Seven reference designators sat on a neighbour's silkscreen, on a pad, or in
the isolation slot, and the two isolator outlines ran straight across the slot;
`REFPOS` in `gen/mkpcb.py` places those seven, and the isolator footprint now
breaks its outline where the slot passes. One of them was less obvious than the
rest: the DRC also checks a footprint's reference against *its own* graphics,
and `C1`'s had been sitting inside its own body circle.

251 track segments and 36 vias on 21 nets. `kicad-cli` on the unfilled file
still shows GND as unconnected and five vias as dangling, because it does not
fill zones. Fill and it goes to zero: `gen/fill2.py` does exactly that from
KiCad's own Python, writes `filled.kicad_pcb`, and running the DRC on that is
how the state above was measured.

### What the ground plane hid

Five GND pads could not actually reach the pour, and the plane itself was in
pieces. None of it is visible before the fill.

- `Q2.2`, `R3.2` and `C6.2` sat in pockets walled in by the traces around them,
  with half a millimetre between two keepouts as the only way out.
- `U1.2`, the Pololu's ground pin, is boxed in between its own two neighbours
  on **both** layers. It now leaves on the back through the gap between the
  V12IN and +12V runs.
- The front-layer pour below y=67 breaks into **six islands**. One of them
  carries `Q4.2` and `R6.2` and hung on nothing at all; another was the only
  copper under half the bottom edge. Each island that carries a pad now has its
  own stitching via.
- `SWGND` reached the isolators from the transceiver only through the SWGND
  pour, which the RS485 descents cut apart. It is a trace now.

Two pads also take the pour solid instead of through a thermal relief
(`gen/zonecon.py`): `C4.2`, where three of four spokes are blocked, and `U1.2`,
whose front-layer spokes all ran into a dead island.

`gen/isl.py` lists the filled islands; `gen/pour.py` floods the pour from one
GND pad. pour.py found the first three pockets. It missed `U1.2` — its raster
counted a 0.44 mm squeeze beside the pad as copper, where KiCad's filler, with
`min_thickness` and island removal, does not. **The authority is the filled
board, not the raster.**

### The pull-up resistors moved

`R11` and `R12` were at (60, 78) and (65, 78), where their links to the I2C bus
ran up the middle of the board at x 56.5 and 57.5 and blocked the only corridor
the RS485 bus could use. They now sit at **(84, 50) and (84, 46), rotated
180°**, directly on the SDA and SCL runs. The stubs are a few millimetres
instead of forty, which suits an I2C bus better than the old position did.

The alternative was to move `J8` about 8 mm west. That was rejected: `J8` is a
screw terminal with a cable on it, so moving it changes mechanics that were
already reviewed, while two 0603 resistors change nothing but copper.

### The routing discipline

Nets run in named lanes so two of them never need to cross on the same layer.
Down the left edge, on the back:

| x | net |
|---|---|
| 21.5 | SWGND |
| 24.5 | 3V3P |
| 27.5 | 3V3REG |
| 29.5 | A1 |
| 38.5 | OW_DATA |
| 51.5 | ISO_VIN |

The four MCU-side nets reach the isolators' top row in a fixed order, each
dropping onto its own pad before continuing east to the second isolator in
back-layer lanes at y 63 (SWGND), 65 (SDA) and 67 (SCL) — all north of the
isolation slot at y 68.8. The RS485 pair comes down the front layer in two
columns at x 67 and x 72.5, both west of the pH slot at x 74, so neither
crosses a barrier.

### Checks, run on every regeneration (`gen/build.sh`)

- `barrier.py` — no track and no zone crosses either isolation slot. This is
  the one thing the board exists for.
- `check.py` — endpoints on their own net's pads, pad clearance, track-to-track
  clearance per layer, via clearance, and the barrier again. This caught every
  crossing during routing; the DRC run only ever confirmed it.
- `pour.py` — can the filled ground actually reach every GND pad.
- `endpoints.py` — the narrower endpoint check, kept because it is what found
  the stale coordinates after the pad order was mirrored.
- `kicad-cli pcb drc`

Route points are written as `'REF.PAD'` wherever they touch a pad, so a
coordinate cannot be mistyped — an earlier pass transcribed them from a
one-decimal printout and every endpoint missed its pad.

## Full review before the first board

Checked, with the method, not by eye:

| | Result |
|---|---|
| Netlist against the firmware's pin assignments | **matches, no firmware change needed** |
| Every pad's net, every net's pad count | 35 nets, no net with a single pad |
| Pads with no net | 8: the four mounting holes and the isolators' `OFF` and `N/C`, all correct |
| Trace width against current (IPC-2152, 1 oz, 10 K rise) | widest requirement 0.12 mm, narrowest power trace 0.50 mm |
| Via annular ring | 0.20 mm (JLCPCB minimum 0.13) |
| Drill sizes | 0.40 … 3.20 mm (minimum 0.30) |
| Board outline | clean 100 × 100 mm, two closed 20 × 2.4 mm slots |
| Placement | all 52 footprints on the front — single-sided assembly |
| Gerber set | 2 layers, 1.6 mm, 9 files correctly typed, slots in the profile |
| BOM against the board | all 52 designators covered, none spare, none twice |
| Isolation | no track, via or pour crosses either slot |
| Filled ground | 0 unconnected, every GND pad reachable |

The netlist check is the one that matters most, because no DRC can catch a
wrong net. `RAIL12_EN = 5` meets `J5.3` (D5), `ISO_EN = A1` meets `J3.2`,
`ONE_WIRE_PIN = A2` meets `J3.1`, and RS485 TX/RX meet D4/D3 the right way
round — `U3.2` is the module's TXD and goes to D4, which the firmware drives
as its transmit pin.

### Three things to check on the test board, not on the screen

**1. The isolator supply is the raw cell.** `ISO_VIN` comes off `Q3`'s drain,
whose source is `+VBATT` — so the isolators see 3.0 to 4.2 V, where on the
bench they sit behind a 3.3 V regulator. The firmware comment says "off the
cell", so this is intended, and the ADM3260 inside the isolator takes 3.0–5.5 V.
But the EZO circuits hang on the isolator's *output*. **Measure `ISO1_3V9`
with a full cell**; it reads 3.9 V at 3.3 V in, and it must stay under the
EZO's 5 V maximum.

**2. The transceiver has no decoupling capacitor near it.** The nearest cap on
`3V3REG` is `C8`, 25 mm away. Most MAX3485 breakout modules carry their own
100 nF — check yours. At 9600 baud on a biased bus this will work either way,
but it is worth a 100 nF across the module's own VCC/GND pins in batch two.

**3. The terminal blocks' bodies.** The footprint is
`TerminalBlock_4Ucon_P3.50mm_Vertical`. The pads fit any 3.5 mm terminal; the
drawn body is that one part. Lay your actual terminals on the 1:1 print. The
tightest spot on the board is `X1` against `C1`, with 0.4 mm between their
outlines — a deeper terminal than 8.5 mm collides there.

### Mounting holes

Four M3, and **not** a rectangle — placement forced them apart. Measured from
the board's top-left corner:

| | from left | from top |
|---|---|---|
| H3 | 5.0 | 5.0 |
| H4 | 95.0 | 5.0 |
| H1 | 5.0 | 82.0 |
| H2 | 95.0 | 92.0 |

### What the board says

The Value fields sit on `F.Fab`, which is documentation and is never printed,
so before this the finished board would have carried nothing but reference
designators. `gen/silk.py` adds 53 texts: every terminal pin by name, the
cell's polarity, a plus on each electrolytic, `ISOLIERT` on the land bridge
between the two slots, `PGND NIE AN GND` beside the electrodes, and a legend
block in the free area right of the WaziSense connectors giving the connector
functions, which control pin does what, and which parts are deliberately not
fitted.

## Fabrication files

`out/fab/` holds everything a board house needs, and `out/KijaniCarrier-gerber.zip`
is the upload: nine layers, the Excellon drill file and the job file. The drill
*map* is deliberately left out of the zip — it is a human-readable drawing with
a frame around it, not a fabrication layer, and it only confuses layer
detection.

| File | What it is |
|---|---|
| `KijaniCarrier-bom-jlcpcb.csv` + `-cpl-jlcpcb.csv` | **49 parts, 23 lines** — SMT and through-hole |
| `KijaniCarrier-bom-smt.csv` + `-cpl-smt.csv` | 24 parts — reflow only, if you order Economic without THT |
| `KijaniCarrier-cpl.csv` | the full placement, every part, for reference |

The `LCSC Part #` column is empty on purpose. JLCPCB's upload matches parts
interactively, and the `Comment` column carries what it needs to match on
(`AO3401A`, `SMBJ5.0A`, `SS34`, `KF128-3.5-3P` and so on). A part number
guessed here would put the wrong component on every board.

### The socket rows

The board has one footprint per module, `U4` to `U7`, but each takes **two**
socket strips 17.78 mm apart, and an assembly line places one part per
designator. The assembly files therefore name the rows `U4A` … `U7B`, with
positions computed from the real pad coordinates, and those names are printed
on the silkscreen beside each row so nobody has to read the CPL alone to find
them.

Splitting the footprints in the board itself would have been the other way
round. It was rejected on purpose: it would have moved about sixty net
references, and a net swapped in that edit would have passed every check,
because it would still be connected — just connected wrongly.

### Only three positions stay empty

`R11` and `R12` (the isolators carry their own pull-ups) and `U2` (`JP1` does
its job). Everything else is fitted; you plug in the six modules.

## Rebuilding

`gen/build.sh` regenerates the board from scratch and ends with
`fillboard.py`, which fills the pours with KiCad's own filler and saves in
place. **Without that last step the file on disk has empty pours**, every GND
pad reads as unconnected and the stitching vias read as dangling, none of which
is true of the real board.

The generators overwrite `KijaniCarrier.kicad_pcb` outright, so anything edited
in pcbnew is lost on the next build. They now refuse to be imported for the
same reason — importing `nets.py` once silently appended a second set of ten
zones to a board that had just been saved from KiCad.

## The coordinate convention

The board sits at an offset of 20 mm from the sheet origin, and KiCad's Y axis
grows **downward** while the placement plan's grows upward. So a part the plan
puts at (64, 24) appears in KiCad at (84, 96).

| plan | KiCad |
|---|---|
| x | x + 20 |
| y | 120 − y |

## What to check first

1. **The four custom footprints against the real modules.** Print
   `KijaniSpace.pretty` at 1:1 — `kicad-cli fp export svg` produces one file per
   footprint — and lay the parts on the paper. Pin 1 is the square pad.
2. **The slot.** It runs across both chains at y 82.6–85.0 in KiCad coordinates,
   between each isolator and its EZO circuit.
3. **The pH run.** `U6` pin 5 (`PRB`) to `J7`: 8.1 mm, and it stays below the
   slot. That distance is the one constraint in this layout that is not
   negotiable.
