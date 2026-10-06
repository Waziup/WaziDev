# -*- coding: utf-8 -*-
"""Per-pad zone connection overrides.

   Two pads cannot get a proper thermal relief where they sit:

   C4.2 has three of its four spokes blocked by the traces around it, so it
   connects solid instead.

   U1.2 is boxed in on the front layer, where all four of its spokes would run
   into a dead island; its real connection is the back-layer trace in
   routes_g.py, so the spokes are turned off and the pad takes the pour solid
   wherever it does touch it.

   KiCad: 0 = none, 1 = thermal relief, 2 = solid.
"""

from __future__ import print_function

import sys as _sys
_GENERATOR_GUARD = True
if __name__ != '__main__':
    raise ImportError('zonecon.py rewrites the board file; run it, do not import it')
import io

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
OVERRIDE = {('C4', '2'): 2, ('U1', '2'): 2}

def blocks(t, tok):
    out, i = [], 0
    while True:
        i = t.find(tok, i)
        if i < 0:
            return out
        d, j = 0, i
        while j < len(t):
            if t[j] == '(':
                d += 1
            elif t[j] == ')':
                d -= 1
                if d == 0:
                    break
            j += 1
        out.append((i, j + 1))
        i = j + 1

s = io.open(PCB, encoding='utf-8').read()
edits = []
for a, b in blocks(s, '(footprint "'):
    blk = s[a:b]
    i = blk.find('(property "Reference" "')
    if i < 0:
        continue
    ref = blk[i + 23:blk.find('"', i + 23)]
    for pa, pb in blocks(blk, '(pad "'):
        pn = blk[pa + 6:blk.find('"', pa + 6)]
        if (ref, pn) in OVERRIDE:
            edits.append((a + pb - 2, OVERRIDE[(ref, pn)], ref, pn))

for pos, val, ref, pn in sorted(edits, reverse=True):
    s = s[:pos] + '\t\t(zone_connect %d)\n\t' % val + s[pos:]
    print('%s.%s -> zone_connect %d' % (ref, pn, val))

io.open(PCB, 'w', encoding='utf-8', newline='\n').write(s)
print('%d override(s) written' % len(edits))
