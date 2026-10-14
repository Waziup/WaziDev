# Superseded — built 7 October 2026, kept only as a record

**Nothing in this folder describes the board as it is now. Do not upload it,
do not print it, do not measure against it.**

These files were produced on 7 October and then sat in `out/` while the board
moved on. They are kept rather than deleted because they are the record of what
Rev A actually was — but they look exactly like the current files, which is what
makes them dangerous.

| File | Why it is wrong now |
|---|---|
| `KijaniCarrier-gerber.zip` | **The hazard.** Eleven files, like the current archive, so a glance cannot tell them apart. It carries `D1` (an SS34 Schottky) where the board now has `Q5` + `R13`, and three 4-pole screw terminals where the board now has six ganged 2-pole blocks. Every copper layer differs. It also holds a single combined `KijaniCarrier.drl` plus a `.gbrjob`, where the current archive has separate PTH and NPTH drill files. Uploading it orders the superseded design. |
| `front.svg`, `back.svg`, `silk.svg`, `board.svg` | Renders of the same old board. The silkscreen in them reads **Rev B** or earlier; the board is **Rev C**. |

## What to use instead

Everything here is now generated, so it cannot drift again:

| Want | Run | Get |
|---|---|---|
| Fabrication archive | `bash gen/fab.sh` | `out/fab/KijaniCarrier-JLCPCB.zip` |
| A quick look, the 1:1 print, footprint views | `bash gen/views.sh` | `out/view.svg`, `out/KijaniCarrier-top-1zu1.pdf`, `out/footprints/` |
| The numbers any document must agree with | `python gen/facts.py` | `out/FACTS.md` |

`gen/fab.sh` empties `out/fab/` before it writes, and `gen/zipcheck.py` reads
the finished archive back — counting its holes against the board's pads and vias
and measuring the outline out of the Edge.Cuts gerber — so a stale or truncated
export cannot pass silently again.

This folder can be deleted whenever the record is no longer wanted.
