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

FAB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\out\fab'
BOM = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\BOM.csv'
POS = os.path.join(FAB, 'KijaniCarrier-cpl.csv')

# which designators does the assembler place?
fit, skip = {}, {}
with io.open(BOM, encoding='utf-8') as f:
    for row in csv.DictReader(f):
        for ref in [r.strip() for r in row['Designator'].split(',') if r.strip()]:
            if row['Assembly'] == 'SMT':
                fit[ref] = (row['Value'], row['Footprint'], row['Note'])
            else:
                skip[ref] = row['Assembly']

rows = list(csv.DictReader(io.open(POS, encoding='utf-8')))
placed = [r for r in rows if r['Ref'] in fit]
left = sorted(set(r['Ref'] for r in rows) - set(fit))

out = os.path.join(FAB, 'KijaniCarrier-cpl-smt.csv')
with io.open(out, 'w', encoding='utf-8', newline='') as f:
    w = csv.writer(f)
    w.writerow(['Designator', 'Mid X', 'Mid Y', 'Layer', 'Rotation'])
    for r in placed:
        w.writerow([r['Ref'], r['PosX'], r['PosY'],
                    'Top' if r['Side'] == 'top' else 'Bottom', r['Rot']])
print('%s: %d parts' % (os.path.basename(out), len(placed)))

# one BOM line per distinct part, designators grouped
groups = {}
for ref in sorted(fit):
    val, fp, note = fit[ref]
    groups.setdefault((val, fp), []).append(ref)
out = os.path.join(FAB, 'KijaniCarrier-bom-smt.csv')
with io.open(out, 'w', encoding='utf-8', newline='') as f:
    w = csv.writer(f)
    w.writerow(['Comment', 'Designator', 'Footprint', 'LCSC Part #'])
    for (val, fp), refs in sorted(groups.items(), key=lambda kv: kv[1][0]):
        w.writerow([val, ','.join(sorted(refs)), fp, ''])
print('%s: %d lines' % (os.path.basename(out), len(groups)))
print('\nnot sent to the assembler (%d): %s' % (len(left), ', '.join(left)))
