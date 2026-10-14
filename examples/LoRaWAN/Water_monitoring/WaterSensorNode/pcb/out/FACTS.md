# FACTS - generated from the board, do not edit

Every number here is read out of `KijaniCarrier.kicad_pcb` or the generated
BOM by `gen/facts.py`. When a document disagrees with this file, the document
is wrong. Regenerate with `python gen/facts.py` after `build.sh` + `fab.sh`.

## Board

| | |
|---|---|
| Outline | **100.0 x 100.0 mm** |
| Footprints placed | **56** |
| Pads total | **150** (of which 88 through-hole) |
| Track segments | **253** |
| Vias | **37** |
| Nets | **36** |
| Copper zones | **10** |
| Custom footprints | **5**: `BE_IVI_Isolator`, `EZO_Circuit`, `MAX3485_Module`, `Pololu_U3V9F12`, `TerminalBlock_KF350_1x02` |

Designators: `C1`, `C10`, `C3`, `C4`, `C5`, `C6`, `C7`, `C8`, `C9`, `D2`, `F1`, `H1`, `H2`, `H3`, `H4`, `J2`, `J3`, `J4`, `J5A`, `J5B`, `J6`, `J7`, `J8`, `J9`, `JP1`, `Q1`, `Q2`, `Q3`, `Q4`, `Q5`, `R1`, `R11`, `R12`, `R13`, `R2`, `R3`, `R4`, `R5`, `R6`, `R7`, `R8`, `R9`, `RT1`, `U1`, `U2`, `U3`, `U4`, `U5`, `U6`, `U7`, `X1`, `X2`, `X3A`, `X3B`, `X4A`, `X4B`

## Assembly BOM (out/fab/KijaniCarrier-bom-jlcpcb.csv)

**29 lines**, 11 Basic and 18 Extended.

| Designators | Value | LCSC | Lib |
|---|---|---|---|
| `C1` | 100uF 16V electrolytic | C47344171 | Extended |
| `C10,C3,C5` | 100nF X7R 50V | C14663 | Basic |
| `C4,C8` | 470uF 25V electrolytic | C49256798 | Extended |
| `C6,C7` | 1uF X5R 50V | C15849 | Basic |
| `C9` | 10uF X5R 25V | C15850 | Basic |
| `D2` | SMBJ5.0A | C19077558 | Extended |
| `F1` | PTC 2.6A hold | C70129 | Extended |
| `J2,J5A` | screw terminal 2P, 3.50 mm (KF350-3.5-2P) | C474892 | Extended |
| `J3,J4` | screw terminal 3P, 3.50 mm (KF350-3.5-3P) | C474893 | Extended |
| `J5B,J6` | screw terminal 2P, 3.50 mm (KF350-3.5-2P) | C474892 | Extended |
| `J7,J9` | SMA jack, vertical | C496549 | Extended |
| `J8,X1` | screw terminal 2P, 3.50 mm (KF350-3.5-2P) | C474892 | Extended |
| `JP1` | 0R | C21189 | Basic |
| `Q1,Q3,Q5` | AO3401A | C15127 | Basic |
| `Q2,Q4` | 2N7002 | C8545 | Basic |
| `R1,R13,R3,R4,R6` | 100k 1% | C25803 | Basic |
| `R2,R5` | 10k 1% | C25804 | Basic |
| `R7,R8` | 680R 1% | C23228 | Basic |
| `R9` | 4k7 1% | C23162 | Basic |
| `RT1` | 120R 1% | C22787 | Basic |
| `U1,U6A` | female header 1x3, 2.54 mm | C146243 | Extended |
| `U3` | female header 1x4, 2.54 mm | C2718488 | Extended |
| `U4A,U4B` | female header 1x5, 2.54 mm | C50950 | Extended |
| `U5A,U5B` | female header 1x5, 2.54 mm | C50950 | Extended |
| `U6B,U7A` | female header 1x3, 2.54 mm | C146243 | Extended |
| `U7B` | female header 1x3, 2.54 mm | C146243 | Extended |
| `X2` | screw terminal 3P, 3.50 mm (KF350-3.5-3P) | C474893 | Extended |
| `X3A,X3B` | screw terminal 2P, 3.50 mm (KF350-3.5-2P) | C474892 | Extended |
| `X4A,X4B` | screw terminal 2P, 3.50 mm (KF350-3.5-2P) | C474892 | Extended |

## What the assembly surcharge comes to

The feeder loading fee buys a feeder on a pick-and-place machine, so it is
charged per Extended **SMT** type, **once per order, not per board**.
Through-hole parts are hand soldered and charged per joint instead.

| | |
|---|---|
| Extended SMT types | 2 x 3.00 USD = **6.00 USD** |
| Through-hole joints | 84 |
| Through-hole labour | 3.50 + 84 x 0.0173 = **4.95 USD** |
| **Total, once per order** | **10.95 USD** |

## How to configure the order

Checked part by part on 12 Oct 2026: **every one of the 10 Extended parts
is marked "Economic and Standard"**, so no single component forces the
dearer tier. Economic is therefore available, and it is the cheaper one:
setup 8.18 USD against 25.56 USD for Standard.

Economic also supports through-hole parts - they go through wave soldering,
not the pick-and-place - so a fully assembled board does NOT require
Standard. An earlier note in this project claimed "Economic = SMT only";
that was wrong.

| Economic condition | this board |
|---|---|
| parts on one side only | **yes** - all 53 placed parts are Top |
| 2, 4 or 6 layers | **yes** - 2 |
| no V-cut panelisation | **yes** - single 100 x 100 mm board |
| quantity cap (~30-50) | fine for a prototype run |

Files to upload:

| | |
|---|---|
| Gerbers + drill | `out/fab/KijaniCarrier-JLCPCB.zip` (11 files, one drill set) |
| BOM | `out/fab/KijaniCarrier-bom-jlcpcb.csv` |
| Placement | `out/fab/KijaniCarrier-cpl-jlcpcb.csv` - body centres, not origins |

## Things documents have got wrong before

- `pcb/README.md` is **not** generated. Nothing in `gen/` writes it, so it
  drifts like any other prose. Only this file and the BOM are generated.
- `D1` (SS34 Schottky) no longer exists. It is `Q5` (AO3401A ideal diode)
  plus `R13` (100k gate pull-down).
- The three 4-pole screw terminals no longer exist. `J5`, `X3` and `X4` are
  each **two ganged 2-pole KF350-3.5 blocks** 7.00 mm apart:
  `J5A`+`J5B`, `X3A`+`X3B`, `X4A`+`X4B`. The pads did not move.
- The gang is **measured, not assumed** (14 Oct 2026, from the EasyEDA
  footprint data for C474892): body **7.000 mm** along the pitch axis,
  pad pitch **3.500 mm**, and interlocking tabs 0.45 mm proud on each side
  at complementary heights. Two blocks on 7.00 mm centres therefore put
  pads at 0 / 3.5 / 7.0 / 10.5 - exactly the 4-pole grid. The tabs clear
  everything: the nearest non-ganged neighbour is J7 at 2.63 mm.
- Two deliberate differences from the vendor footprint, both on the safe
  side: the courtyard here is 9.5 mm deep against the body-s 6.9 mm, and
  the drill is 1.3 mm against 1.0 mm - the KiCad 4Ucon value for this
  class of block, a 0.65 mm annular ring and a more forgiving fit.
- The terminals are **KF350-3.5**, not KF128-3.5. KF128 does not exist at
  this pitch; searching only for it once produced a false "not stocked".
- `C1` is 100uF 16V **D5 x 11 mm, 2.00 mm pitch** (C47344171).
  `C4`/`C8` are 470uF 25V **D10 x 13 mm, 5.00 mm pitch** (C49256798) - the
  5.00 mm pitch is load-bearing, two rails pass between the pads.
- The SMA jacks `J7`/`J9` stay **vertical** (Amphenol 132134, C3174425).
  The enclosure feed-through is a separate panel-mount bulkhead jack with a
  pigtail, decided 12 Oct 2026, so the board is unchanged.
- Nothing further can be made Basic: JLCPCB's Basic & Promotional Extended
  library (1586 parts) has no Connectors category at all and no electrolytic,
  TVS or fuse.
