# -*- coding: utf-8 -*-
"""Append traces and vias to the board.

   The routing itself lives in routes_*.py, one module per block of the
   circuit, each exporting ROUTES = [(net, layer, width, [points])] and
   VIAS = [(net, x, y)]. All coordinates are KiCad board coordinates (mm);
   plan -> KiCad is x+20, 120-y.
"""

from __future__ import print_function

import sys as _sys
_GENERATOR_GUARD = True
if __name__ != '__main__':
    raise ImportError('route.py rewrites the board file; run it, do not import it')
import io, re, hashlib, importlib
from pads import POS

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
BLOCKS = ['routes_iso', 'routes_a', 'routes_b', 'routes_c', 'routes_d', 'routes_e', 'routes_g']
VIA_SIZE, VIA_DRILL = 0.8, 0.4

ROUTES, VIAS = [], []
for name in BLOCKS:
    try:
        m = importlib.import_module(name)
    except ImportError:
        continue
    ROUTES += list(getattr(m, 'ROUTES', []))
    VIAS += list(getattr(m, 'VIAS', []))

def coord(v, axis):
    """A point element is a number, or 'REF.PAD' meaning that pad's x or y.
       Writing ('R1.1', 'R1.1') for a pad, or (28.5, 'R1.1') for a lane at the
       pad's height, means a coordinate can never be mistyped."""
    if not isinstance(v, str):
        return round(float(v), 3)
    ref, pn = v.split('.')
    if (ref, pn) not in POS:
        raise KeyError('no pad ' + v)
    return round(POS[(ref, pn)][axis], 3)

def pt(p):
    return (coord(p[0], 0), coord(p[1], 1))

def uid(seed):
    h = hashlib.md5(seed.encode('utf-8')).hexdigest()
    return '%s-%s-%s-%s-%s' % (h[0:8], h[8:12], h[12:16], h[16:20], h[20:32])

s = io.open(PCB, encoding='utf-8').read()
num = dict((m.group(2), int(m.group(1))) for m in re.finditer(r'\(net (\d+) "([^"]*)"\)', s))

out, n = '', 0
for net, layer, w, pts in ROUTES:
    if net not in num:
        print('  !! unknown net %s' % net)
        continue
    pts = [pt(p) for p in pts]
    for i in range(len(pts) - 1):
        (x1, y1), (x2, y2) = pts[i], pts[i + 1]
        out += ('\t(segment\n\t\t(start %s %s)\n\t\t(end %s %s)\n\t\t(width %s)\n'
                '\t\t(layer "%s")\n\t\t(net %d)\n\t\t(uuid "%s")\n\t)\n'
                ) % (x1, y1, x2, y2, w, layer, num[net],
                     uid('%s-%s-%s-%s-%s' % (net, layer, i, pts[i], pts[i + 1])))
        n += 1

nv = 0
for net, vx, vy in VIAS:
    x, y = coord(vx, 0), coord(vy, 1)
    if net not in num:
        print('  !! unknown net %s on via' % net)
        continue
    out += ('\t(via\n\t\t(at %s %s)\n\t\t(size %s)\n\t\t(drill %s)\n'
            '\t\t(layers "F.Cu" "B.Cu")\n\t\t(net %d)\n\t\t(uuid "%s")\n\t)\n'
            ) % (x, y, VIA_SIZE, VIA_DRILL, num[net], uid('via-%s-%s-%s' % (net, x, y)))
    nv += 1

s = s.rstrip()[:-1].rstrip() + '\n' + out + ')\n'
io.open(PCB, 'w', encoding='utf-8', newline='\n').write(s)
print('%d segments, %d vias, from %d route entries' % (n, nv, len(ROUTES)))
