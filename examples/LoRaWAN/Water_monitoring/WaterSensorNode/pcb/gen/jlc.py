# -*- coding: utf-8 -*-
"""Turn the KiCad position file and BOM.csv into the two files JLCPCB's
   assembly service asks for, covering the SMT parts only.

   Everything marked 'hand' in BOM.csv is a through-hole module, screw terminal
   or electrolytic that is fitted here, not by the assembler, and everything
   marked 'DNP' is deliberately not fitted. Both are left out of the placement
   file so the assembler does not quote for them.
"""
from __future__ import print_function
import csv, io, os
from pads import POS
from centroid import CENTRE

FAB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\out\fab'
BOM = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\BOM.csv'
POSFILE = os.path.join(FAB, 'KijaniCarrier-cpl.csv')

# Two orders are possible: SMT only (Economic, reflow) or SMT plus through
# hole (wave or selective soldering). Build the files for both.
SETS = {'smt': ('SMT',), 'jlcpcb': ('SMT', 'THT')}

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

# The board carries one footprint per module, U4..U7, but each takes TWO socket
# strips, and an assembly line places one part per designator. These name the
# two rows. They exist only in the assembly files; the board is untouched, so
# nothing in the netlist can shift.
ROWS = {
    'U4A': ('U4', '1', '5'), 'U4B': ('U4', '6', '10'),
    'U5A': ('U5', '1', '5'), 'U5B': ('U5', '6', '10'),
    'U6A': ('U6', '1', '3'), 'U6B': ('U6', '4', '6'),
    'U7A': ('U7', '1', '3'), 'U7B': ('U7', '4', '6'),
}
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
            w.writerow([r['Ref'], px, py,
                        'Top' if r['Side'] == 'top' else 'Bottom', r['Rot']])

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
    #  So: one line per part number, the format JLCPCB expects. Set this to
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
