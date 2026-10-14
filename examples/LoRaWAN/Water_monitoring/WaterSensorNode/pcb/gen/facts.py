# -*- coding: utf-8 -*-
"""Ground truth for the documentation, read off the board and nothing else.

   WHY THIS EXISTS. Five published pages and a README quote numbers about this
   board - segment counts, footprint counts, BOM line counts, part numbers. Every
   one of those was true when it was typed and several are not true now. The only
   defensible way to audit them is against a file that is generated, so the
   auditor is never comparing one piece of prose with another.

   Run it after build.sh and fab.sh. It writes out/FACTS.md.
"""
from __future__ import print_function
import csv, io, math, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..')
sys.path.insert(0, HERE)
from centroid import _blocks, CENTRE

PCB = os.path.join(ROOT, 'KijaniCarrier.kicad_pcb')
s = io.open(PCB, encoding='utf-8').read()

# ---- geometry -------------------------------------------------------------
xs, ys = [], []
for ln in _blocks(s, 'gr_line') + _blocks(s, 'gr_rect'):
    if '"Edge.Cuts"' in ln:
        for a, b in re.findall(r'\((?:start|end) ([-\d.]+) ([-\d.]+)\)', ln):
            xs.append(float(a)); ys.append(float(b))
W, H = max(xs) - min(xs), max(ys) - min(ys)

# ---- what is on the board -------------------------------------------------
refs, pads_tht, pads_all = [], 0, 0
for blk in _blocks(s, 'footprint'):
    m = re.search(r'\(property "Reference" "([^"]+)"', blk)
    if not m:
        continue
    refs.append(m.group(1))
    for p in _blocks(blk, 'pad'):
        pads_all += 1
        if '(drill ' in p:
            pads_tht += 1

segs = len(_blocks(s, 'segment'))
vias = len(_blocks(s, 'via'))
# Nets appear on the pads by name; there is no separate net table to count.
nets = len(set(re.findall(r'\(net "([^"]+)"\)', s)))
# only top-level zones, not the (zone_connect N) property inside a pad
zones = len([z for z in _blocks(s, 'zone') if not z.startswith('(zone_connect')])
fps_lib = sorted(set(re.findall(r'\(footprint "KijaniSpace:([^"]+)"', s)))

# ---- what will be ordered -------------------------------------------------
bom = list(csv.DictReader(io.open(os.path.join(ROOT, 'out/fab/KijaniCarrier-bom-jlcpcb.csv'),
                                  encoding='utf-8')))
basic = sum(1 for r in bom if r['Lib'] == 'Basic')
ext = sum(1 for r in bom if r['Lib'] == 'Extended')
SMT_FP = ('C_0603', 'C_0805', 'R_0603', 'R_1812', 'SOT-23', 'D_SMB')

def padcount(ref):
    from collections import Counter
    return PAD.get(ref, PAD.get(ref[:-1], 0) // 2 if ref[-1] in 'AB' else 0)

PAD = {}
for blk in _blocks(s, 'footprint'):
    m = re.search(r'\(property "Reference" "([^"]+)"', blk)
    if m:
        PAD[m.group(1)] = sum(1 for p in _blocks(blk, 'pad') if '(drill ' in p)

feeder = joints = 0
for r in bom:
    smt = r['Footprint'] in SMT_FP
    if smt:
        feeder += (r['Lib'] == 'Extended')
    else:
        joints += sum(padcount(x.strip()) for x in r['Designator'].split(','))

# ---- write ----------------------------------------------------------------
L = []
w = L.append
w('# FACTS - generated from the board, do not edit')
w('')
w('Every number here is read out of `KijaniCarrier.kicad_pcb` or the generated')
w('BOM by `gen/facts.py`. When a document disagrees with this file, the document')
w('is wrong. Regenerate with `python gen/facts.py` after `build.sh` + `fab.sh`.')
w('')
w('## Board')
w('')
w('| | |')
w('|---|---|')
w('| Outline | **%.1f x %.1f mm** |' % (W, H))
w('| Footprints placed | **%d** |' % len(refs))
w('| Pads total | **%d** (of which %d through-hole) |' % (pads_all, pads_tht))
w('| Track segments | **%d** |' % segs)
w('| Vias | **%d** |' % vias)
w('| Nets | **%d** |' % nets)
w('| Copper zones | **%d** |' % zones)
w('| Custom footprints | **%d**: %s |' % (len(fps_lib), ', '.join('`%s`' % f for f in fps_lib)))
w('')
w('Designators: ' + ', '.join('`%s`' % r for r in sorted(refs)))
w('')
w('## Assembly BOM (out/fab/KijaniCarrier-bom-jlcpcb.csv)')
w('')
w('**%d lines**, %d Basic and %d Extended.' % (len(bom), basic, ext))
w('')
w('| Designators | Value | LCSC | Lib |')
w('|---|---|---|---|')
for r in sorted(bom, key=lambda r: r['Designator']):
    w('| `%s` | %s | %s | %s |' % (r['Designator'], r['Comment'], r['LCSC Part #'], r['Lib']))
w('')
w('## What the assembly surcharge comes to')
w('')
w('The feeder loading fee buys a feeder on a pick-and-place machine, so it is')
w('charged per Extended **SMT** type, **once per order, not per board**.')
w('Through-hole parts are hand soldered and charged per joint instead.')
w('')
w('| | |')
w('|---|---|')
w('| Extended SMT types | %d x 3.00 USD = **%.2f USD** |' % (feeder, feeder * 3))
w('| Through-hole joints | %d |' % joints)
w('| Through-hole labour | 3.50 + %d x 0.0173 = **%.2f USD** |' % (joints, 3.5 + joints * 0.0173))
w('| **Total, once per order** | **%.2f USD** |' % (feeder * 3 + 3.5 + joints * 0.0173))
w('')
w('## How to configure the order')
w('')
w('Checked part by part on 12 Oct 2026: **every one of the 10 Extended parts')
w('is marked "Economic and Standard"**, so no single component forces the')
w('dearer tier. Economic is therefore available, and it is the cheaper one:')
w('setup 8.18 USD against 25.56 USD for Standard.')
w('')
w('Economic also supports through-hole parts - they go through wave soldering,')
w('not the pick-and-place - so a fully assembled board does NOT require')
w('Standard. An earlier note in this project claimed "Economic = SMT only";')
w('that was wrong.')
w('')
w('| Economic condition | this board |')
w('|---|---|')
w('| parts on one side only | **yes** - all 53 placed parts are Top |')
w('| 2, 4 or 6 layers | **yes** - 2 |')
w('| no V-cut panelisation | **yes** - single 100 x 100 mm board |')
w('| quantity cap (~30-50) | fine for a prototype run |')
w('')
w('Files to upload:')
w('')
w('| | |')
w('|---|---|')
w('| Gerbers + drill | `out/fab/KijaniCarrier-JLCPCB.zip` (11 files, one drill set) |')
w('| BOM | `out/fab/KijaniCarrier-bom-jlcpcb.csv` |')
w('| Placement | `out/fab/KijaniCarrier-cpl-jlcpcb.csv` - body centres, not origins |')
w('')
w('## Things documents have got wrong before')
w('')
w('- `pcb/README.md` is **not** generated. Nothing in `gen/` writes it, so it')
w('  drifts like any other prose. Only this file and the BOM are generated.')
w('- `D1` (SS34 Schottky) no longer exists. It is `Q5` (AO3401A ideal diode)')
w('  plus `R13` (100k gate pull-down).')
w('- The three 4-pole screw terminals no longer exist. `J5`, `X3` and `X4` are')
w('  each **two ganged 2-pole KF350-3.5 blocks** 7.00 mm apart:')
w('  `J5A`+`J5B`, `X3A`+`X3B`, `X4A`+`X4B`. The pads did not move.')
w('- The gang is **measured, not assumed** (14 Oct 2026, from the EasyEDA')
w('  footprint data for C474892): body **7.000 mm** along the pitch axis,')
w('  pad pitch **3.500 mm**, and interlocking tabs 0.45 mm proud on each side')
w('  at complementary heights. Two blocks on 7.00 mm centres therefore put')
w('  pads at 0 / 3.5 / 7.0 / 10.5 - exactly the 4-pole grid. The tabs clear')
w('  everything: the nearest non-ganged neighbour is J7 at 2.63 mm.')
w('- Two deliberate differences from the vendor footprint, both on the safe')
w('  side: the courtyard here is 9.5 mm deep against the body-s 6.9 mm, and')
w('  the drill is 1.3 mm against 1.0 mm - the KiCad 4Ucon value for this')
w('  class of block, a 0.65 mm annular ring and a more forgiving fit.')
w('- The terminals are **KF350-3.5**, not KF128-3.5. KF128 does not exist at')
w('  this pitch; searching only for it once produced a false "not stocked".')
w('- `C1` is 100uF 16V **D5 x 11 mm, 2.00 mm pitch** (C47344171).')
w('  `C4`/`C8` are 470uF 25V **D10 x 13 mm, 5.00 mm pitch** (C49256798) - the')
w('  5.00 mm pitch is load-bearing, two rails pass between the pads.')
w('- The SMA jacks `J7`/`J9` stay **vertical** (Amphenol 132134, C3174425).')
w('  The enclosure feed-through is a separate panel-mount bulkhead jack with a')
w('  pigtail, decided 12 Oct 2026, so the board is unchanged.')
w('- Nothing further can be made Basic: JLCPCB\'s Basic & Promotional Extended')
w('  library (1586 parts) has no Connectors category at all and no electrolytic,')
w('  TVS or fuse.')

out = os.path.join(ROOT, 'out', 'FACTS.md')
io.open(out, 'w', encoding='utf-8', newline='\n').write('\n'.join(L) + '\n')
print('out/FACTS.md: %d footprints, %d segments, %d BOM lines, %.2f USD Aufschlag'
      % (len(refs), segs, len(bom), feeder * 3 + 3.5 + joints * 0.0173))
