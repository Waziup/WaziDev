# -*- coding: utf-8 -*-
"""Every placement point sits on its own pads.

   WHY. On 16 Oct 2026 JLCPCB's engineer sent back a placement render with the
   3-pin socket for the boost module, U1, drawn 5.5 mm above its holes.
   centroid.py had taken the centre of the Pololu footprint's courtyard - the
   outline of the whole plug-in module - but the assembler fits only the pin
   socket, and that sits on the pad row at the module's edge. Every other
   check passed, because none of them compared the CPL with the copper. The
   engineer's own correction did not catch it either.

   The rule, per line of the placement file:
     - through hole: the point is the centroid of the part's own holes, to
       0.05 mm. True for a terminal block, a radial electrolytic, a socket
       row and the SMA jack alike, because all of them are centred on their
       leads;
     - SMT: the point lies inside the bounding box of the part's pads.
"""
from __future__ import print_function
import csv, io, os, sys
from pads import POS
from centroid import ROWS

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..')
BOM = os.path.join(ROOT, 'BOM.csv')
FAB = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'out', 'fab')
TOL = 0.05

kind = {}
for r in csv.DictReader(io.open(BOM, encoding='utf-8')):
    for d in [x.strip() for x in r['Designator'].split(',') if x.strip()]:
        kind[d] = r['Assembly']


def pads_of(ref):
    if ref in ROWS:
        base, a, b = ROWS[ref]
        nums = [str(n) for n in range(int(a), int(b) + 1)]
        return [POS[(base, n)][:2] for n in nums]
    return sorted(set(v[:2] for k, v in POS.items() if k[0] == ref))


bad, n = [], 0
for name in ('KijaniCarrier-cpl-jlcpcb.csv', 'KijaniCarrier-cpl-smt.csv'):
    for r in csv.DictReader(io.open(os.path.join(FAB, name), encoding='utf-8')):
        ref = r['Designator']
        x, y = float(r['Mid X']), -float(r['Mid Y'])
        pp = pads_of(ref)
        if not pp:
            bad.append('%s %s: keine Pads auf dem Board' % (name, ref))
            continue
        n += 1
        if kind.get(ref, 'THT') == 'THT':
            cx = sum(p[0] for p in pp) / len(pp)
            cy = sum(p[1] for p in pp) / len(pp)
            d = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
            if d > TOL:
                bad.append('%s %s: %.2f mm neben der Mitte seiner %d Bohrungen '
                           '(CPL %.2f/%.2f, Bohrungen %.2f/%.2f)'
                           % (name, ref, d, len(pp), x, -y, cx, -cy))
        else:
            xs, ys = [p[0] for p in pp], [p[1] for p in pp]
            if not (min(xs) - TOL <= x <= max(xs) + TOL
                    and min(ys) - TOL <= y <= max(ys) + TOL):
                bad.append('%s %s: ausserhalb seiner Pads' % (name, ref))

print('Platzierung: %d Zeilen gegen das Kupfer geprueft' % n)
if bad:
    for b in bad:
        print('  DANEBEN  ' + b)
    sys.exit(1)
print('Platzierung: jedes THT-Teil sitzt mittig auf seinen Bohrungen,')
print('jedes SMT-Teil innerhalb seiner Pads.')
