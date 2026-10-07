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

FAB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\out\fab'
BOM = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\BOM.csv'
POSFILE = os.path.join(FAB, 'KijaniCarrier-cpl.csv')

# Two orders are possible: SMT only (Economic, reflow) or SMT plus through
# hole (wave or selective soldering). Build the files for both.
SETS = {'smt': ('SMT',), 'jlcpcb': ('SMT', 'THT')}

# which designators does the assembler place?
fit, skip = {}, {}
with io.open(BOM, encoding='utf-8') as f:
    for row in csv.DictReader(f):
        for ref in [r.strip() for r in row['Designator'].split(',') if r.strip()]:
            fit.setdefault(row['Assembly'], {})[ref] = (
                row['Value'], row['Footprint'], row['Note'])

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
            w.writerow([r['Ref'], r['PosX'], r['PosY'],
                        'Top' if r['Side'] == 'top' else 'Bottom', r['Rot']])

    groups = {}
    for ref in sorted(place):
        val, fp, note = place[ref]
        groups.setdefault((val, fp), []).append(ref)
    out = os.path.join(FAB, 'KijaniCarrier-bom-%s.csv' % tag)
    with io.open(out, 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f)
        w.writerow(['Comment', 'Designator', 'Footprint', 'LCSC Part #'])
        for (val, fp), refs in sorted(groups.items(), key=lambda kv: kv[1][0]):
            w.writerow([val, ','.join(sorted(refs)), fp, ''])
    print('%-8s %2d Bauteile, %2d BOM-Zeilen' % (tag, len(placed), len(groups)))

left = sorted(set(r['Ref'] for r in rows) - set(
    k for kinds in SETS.values() for kk in kinds for k in fit.get(kk, {})))
print()
print('nie vom Bestuecker (%d): %s' % (len(left), ', '.join(left)))
