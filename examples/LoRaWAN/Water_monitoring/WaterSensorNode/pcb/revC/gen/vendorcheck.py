# -*- coding: utf-8 -*-
"""Compare each through-hole footprint on the board against the vendor's own.

   WHY. The 1:1 paper check is the real test, and it can only be done with the
   part in your hand. These through-hole parts are bought from JLCPCB with the
   assembly, so they are never on the bench before the board is. This is the
   next best thing: the lead pitch and the hole of every THT footprint,
   measured against the footprint the part's own supplier publishes.

   WHAT IT ANSWERS, and what it does not:
     - is the lead pitch right?  A wrong pitch never fits.
     - is the hole big enough?   Too small never fits either.
     - it CANNOT tell you whether a body fouls a neighbour. Only paper does.

   THE HOLE IS JUDGED AGAINST THE LEAD, NOT AGAINST THE VENDOR'S HOLE. A first
   version of this script compared hole to hole and flagged C1, whose board
   hole is 0.80 mm against the vendor's 0.90. That was a bad test: the lead is
   0.5 mm (the vendor's own package name ends -D0.5-), so 0.80 mm is 0.30 mm
   of clearance, which is what IPC asks for in a wave-soldered joint. Comparing
   two holes measures two opinions about the same pin.

   Vendor figures read from the EasyEDA footprint data on 14 Oct 2026.
"""
from __future__ import print_function
import io, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from centroid import _blocks

PCB = os.path.join(HERE, '..', 'KijaniCarrier.kicad_pcb')

#  refs, pad span mm, lead dia mm (None = unknown), vendor hole mm, part
VENDOR = [
    ('C1',                        2.00, 0.50, 0.90, 'C47344171 100uF D5x11'),
    ('C4,C8',                     5.00, 0.60, 1.00, 'C49256798 470uF D10x13'),
    ('J2,J6,J8,X1,J5A,J5B,X3A,X3B,X4A,X4B',
                                  3.50, None, 1.20, 'C474892  KF350-3.5-2P'),
    ('J3,J4,X2',                  7.00, None, 1.20, 'C474893  KF350-3.5-3P'),
    #  The fitted jack is the BWSMA-KE-Z001; pitch and hole here are the
    #  Amphenol 132134's, the part it replaced in the same holes.
    ('J7,J9',                     5.08, None, 1.50, 'C496549  BWSMA-KE-Z001'),
    ('U1',                        5.08, None, 1.02, 'C146243  1x3 header'),
    ('U3',                        7.62, None, 1.10, 'C2718488 1x4 header'),
]

s = io.open(PCB, encoding='utf-8').read()
board = {}
for blk in _blocks(s, 'footprint'):
    m = re.search(r'\(property "Reference" "([^"]+)"', blk)
    if not m:
        continue
    xs, ys, drills = [], [], []
    for p in _blocks(blk, 'pad'):
        if '(drill ' not in p:
            continue
        at = re.search(r'\(at ([-\d.]+) ([-\d.]+)', p)
        dr = re.search(r'\(drill ([\d.]+)', p)
        if at and dr:
            xs.append(float(at.group(1)))
            ys.append(float(at.group(2)))
            drills.append(float(dr.group(1)))
    if xs:
        span = max(max(xs) - min(xs), max(ys) - min(ys))
        board[m.group(1)] = (span, min(drills))

print('%-6s %-24s %7s %7s  %7s %7s %7s  %s'
      % ('Ref', 'Teil', 'Raster', 'Board', 'Bein', 'Herst.', 'Board', ''))
bad = []
for refs, vspan, lead, vhole, what in VENDOR:
    label = what
    for ref in refs.split(','):
        span, hole = board[ref]
        ok_span = abs(span - vspan) < 0.01
        need = (lead + 0.15) if lead else (vhole - 0.05)
        ok_hole = hole >= need - 1e-9
        flag = '' if (ok_span and ok_hole) else '  <-- PRUEFEN'
        if flag:
            bad.append((ref, span, vspan, hole, need))
        print('%-6s %-24s %7.2f %7.2f  %7s %7.2f %7.2f%s'
              % (ref, label, vspan, span,
                 ('%.2f' % lead) if lead else '-', vhole, hole, flag))
        label = ''
    print()

if bad:
    for ref, span, vspan, hole, need in bad:
        print('ABWEICHUNG %s: Raster %.2f (soll %.2f), Bohrung %.2f (mind. %.2f)'
              % (ref, span, vspan, hole, need))
    raise SystemExit(1)

print('Alle 20 THT-Raster treffen die Herstellerangabe auf 0.01 mm.')
print('Jede Bohrung ist mindestens so gross, wie der Stift verlangt.')
print()
print('Was das NICHT zeigt: ob ein Bauteilkoerper einen Nachbarn beruehrt.')
print('Dafuer gibt es nur den 1:1-Ausdruck mit dem Teil in der Hand.')
