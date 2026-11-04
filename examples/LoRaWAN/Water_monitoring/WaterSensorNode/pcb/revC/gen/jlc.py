# -*- coding: utf-8 -*-
"""Turn the KiCad position file and BOM.csv into the files JLCPCB's assembly
   service asks for: the full set (SMT and THT, '-jlcpcb') that was ordered,
   and an SMT-only set ('-smt') for a reflow-only order.

   Everything marked 'DNP' or 'mech' in BOM.csv is deliberately not fitted
   and is left out of both. Every placement point is a BODY centre (see
   centroid.py), and cplcheck.py proves each one sits on its own pads.
"""
from __future__ import print_function
import csv, io, os
from pads import POS
from centroid import CENTRE, FOOTPRINT, ROWS

from paths import FAB
from paths import BOM
POSFILE = os.path.join(FAB, 'KijaniCarrier-cpl.csv')

# Two orders are possible: SMT only (Economic, reflow) or SMT plus through
# hole (wave or selective soldering). Build the files for both.
SETS = {'smt': ('SMT',), 'jlcpcb': ('SMT', 'THT')}

#  JLCPCB's own library sets a different zero orientation for some packages.
#  SOT-23 is the classic one: its pin 1 sits where KiCad's pin 3 is. Seen in
#  the 15 Oct placement view, on two parts at two rotations - Q1 at 0 deg
#  showed JLCPCB's pin-1 dot bottom right against KiCad's pin-1 triangle on
#  the left, Q5 at 180 deg the dot top left against the triangle on the
#  right. Uncorrected, all five MOSFETs - both load switches, both level
#  shifters and the reverse-polarity FET - go on turned round. D_SMB and
#  CP_Radial matched in the same view and need nothing.
JLC_ROT_FP = {'SOT-23': 180.0}

#  Per part, on top of that. J8 is a symmetrical 2-pole block, so turning it
#  round keeps each screw on its own pad; it turns the wire opening from
#  facing away from the RS485 module to facing it.
JLC_ROT = {'J8': 180.0}

#  Which footprints go through the pick-and-place rather than the solder bath.
SMT_FOOTPRINTS = ('C_0603', 'C_0805', 'R_0603', 'R_1812', 'SOT-23', 'D_SMB')

# which designators does the assembler place?
fit, skip = {}, {}
with io.open(BOM, encoding='utf-8') as f:
    for row in csv.DictReader(f):
        for ref in [r.strip() for r in row['Designator'].split(',') if r.strip()]:
            fit.setdefault(row['Assembly'], {})[ref] = (
                row['Value'], row['Footprint'], row.get('LCSC', ''),
                row.get('Lib', ''), row.get('UnitEUR', ''))

rows = list(csv.DictReader(io.open(POSFILE, encoding='utf-8')))

# U4..U7 each take two socket strips; ROWS (centroid.py) names them.
for tag, (ref, a, b) in sorted(ROWS.items()):
    pa, pb = POS[(ref, a)], POS[(ref, b)]
    rows.append({'Ref': tag, 'Val': 'female header',
                 'PosX': '%.4f' % ((pa[0] + pb[0]) / 2.0),
                 'PosY': '%.4f' % (-(pa[1] + pb[1]) / 2.0),
                 'Rot': '0.0000', 'Side': 'top'})
    print('  %s at (%.2f, %.2f)' % (tag, (pa[0]+pb[0])/2.0, (pa[1]+pb[1])/2.0))

for tag, kinds in sorted(SETS.items()):
    place = {}
    for k in kinds:
        place.update(fit.get(k, {}))
    placed = [r for r in rows if r['Ref'] in place]

    out = os.path.join(FAB, 'KijaniCarrier-cpl-%s.csv' % tag)
    with io.open(out, 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f)
        w.writerow(['Designator', 'Mid X', 'Mid Y', 'Layer', 'Rotation'])
        for r in placed:
            # Mid X/Mid Y is the centre of the BODY. kicad-cli writes the
            # footprint origin, which for a screw terminal or a radial
            # electrolytic is pad 1 - up to 3.5 mm out. See centroid.py.
            c = CENTRE.get(r['Ref'])
            px = '%.4f' % c[0] if c else r['PosX']
            py = '%.4f' % -c[1] if c else r['PosY']
            rot = (float(r['Rot']) + JLC_ROT.get(r['Ref'], 0.0)
                   + JLC_ROT_FP.get(FOOTPRINT.get(r['Ref'], ''), 0.0)) % 360
            w.writerow([r['Ref'], px, py,
                        'Top' if r['Side'] == 'top' else 'Bottom', '%.4f' % rot])

    groups = {}
    for ref in sorted(place):
        val, fp, lcsc, lib, eur = place[ref]
        groups.setdefault((val, fp, lcsc, lib, eur), []).append(ref)

    #  THROUGH-HOLE LINES ARE SPLIT. The 14 October quote priced every THT
    #  line of up to 2 designators (10 pieces at 5 boards) and returned
    #  quantity 0 for every line of 3 or more - the 13 screw terminals and
    #  9 of the 10 socket rows, which is exactly the plug-and-play half of
    #  the board. The parts are stocked and marked for Economic assembly, so
    #  the limit is on the LINE, not the part. Splitting keeps every line
    #  inside what demonstrably quotes. It costs nothing: the Extended
    #  feeder fee is charged per part number, not per line.
    #  WHAT THE 14 OCTOBER TEST ACTUALLY SHOWED, which is not what the
    #  comment above predicted:
    #
    #    - splitting did get C474893 and C146243 quoted, which the single
    #      long line never managed;
    #    - but JLCPCB does NOT add up a part number that appears on several
    #      lines. C146243 sits on three lines and only the first carries a
    #      quantity - 5 pieces, where 5 designators on 2 boards need 10. The
    #      split trades a zero for an under-supply, which is worse: a zero
    #      is obvious, a short count is not.
    #    - C474892 and C50950 return 0 at ANY quantity, down to 4 pieces,
    #      and are the only two rows carrying a warning icon. Those two are
    #      a part problem, not a quantity problem.
    #
    #  THE REAL CAUSE, found 15 October: none of the above. JLCPCB marks a
    #  row 'there may be multiple types of parts, but one type has been
    #  matched' and quietly UNTICKS it in the Select column. An unticked row
    #  shows quantity 0 and is not assembled. The four rows were exactly the
    #  four with that warning. Ticking them gave C474892 21 pcs, C474893 6,
    #  C146243 13, C50950 8 - every one covering the need.
    #
    #  So: one line per part number, the format JLCPCB expects, and after
    #  every upload check the Select column, not just the match. Set this to
    #  2 only to reproduce the experiment.
    MAX_THT_PER_LINE = 99
    lines = []
    for key, refs in groups.items():
        refs = sorted(refs)
        tht = key[1] not in SMT_FOOTPRINTS
        step = MAX_THT_PER_LINE if tht else len(refs)
        for i in range(0, len(refs), step):
            lines.append((key, refs[i:i + step]))
    lines.sort(key=lambda kv: (kv[1][0]))

    out = os.path.join(FAB, 'KijaniCarrier-bom-%s.csv' % tag)
    with io.open(out, 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f)
        w.writerow(['Comment', 'Designator', 'Footprint', 'LCSC Part #', 'Lib',
                    'Unit EUR'])
        for (val, fp, lcsc, lib, eur), refs in lines:
            w.writerow([val, ','.join(refs), fp, lcsc, lib, eur])
    split = len(lines) - len(groups)
    print('%-8s %2d Bauteile, %2d BOM-Zeilen (%d Teilenummern, %d THT-Zeilen geteilt)'
          % (tag, len(placed), len(lines), len(groups), split))

left = sorted(set(r['Ref'] for r in rows) - set(
    k for kinds in SETS.values() for kk in kinds for k in fit.get(kk, {})))
print()
print('nie vom Bestuecker (%d): %s' % (len(left), ', '.join(left)))
