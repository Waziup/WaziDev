# -*- coding: utf-8 -*-
"""Read pad positions straight out of the board file. KiCad rotates clockwise:
   gx = px + lx*cos + ly*sin
   gy = py - lx*sin + ly*cos
"""
from __future__ import print_function
import io, re, sys, math, codecs
if __name__ == '__main__':
    sys.stdout = codecs.getwriter('ascii')(sys.stdout.buffer, 'replace')

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
WANT = sys.argv[1:] or ['U4', 'U6', 'U5', 'U7', 'J7', 'J9']


def spans(t, tok):
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


def place(px, py, rot, lx, ly):
    th = math.radians(rot)
    c, s = math.cos(th), math.sin(th)
    return px + lx * c + ly * s, py - lx * s + ly * c


POS = {}
META = {}
s = io.open(PCB, encoding='utf-8').read()
for a, b in spans(s, '(footprint "'):
    blk = s[a:b]
    rm = re.search(r'\(property "Reference" "([^"]*)"', blk)
    if not rm:
        continue
    ref = rm.group(1)
    # the generator writes one indentation, pcbnew another; take the first
    # top-level (at ...) either way
    fa = re.search(r'\n\t+\(at ([-0-9.]+) ([-0-9.]+)(?: ([-0-9.]+))?\)', blk)
    px, py, rot = float(fa.group(1)), float(fa.group(2)), float(fa.group(3) or 0)
    for pa, pb in spans(blk, '(pad "'):
        pbk = blk[pa:pb]
        pn = re.match(r'\(pad "([^"]*)"', pbk).group(1)
        # the generator writes (net 9 "GND"), pcbnew 10 writes (net "GND")
        nt = re.search(r'\(net (?:\d+ )?"([^"]*)"\)', pbk)
        la = re.search(r'\(at ([-0-9.]+) ([-0-9.]+)', pbk)
        gx, gy = place(px, py, rot, float(la.group(1)), float(la.group(2)))
        sz = re.search(r'\(size ([-0-9.]+) ([-0-9.]+)\)', pbk)
        sw, sh = (float(sz.group(1)), float(sz.group(2))) if sz else (1.0, 1.0)
        ty = re.match(r'\(pad "[^"]*" (\w+)', pbk).group(1)
        lay = re.search(r'\(layers ([^)]*)\)', pbk)
        both = 'thru_hole' in ty or (lay and '*.Cu' in lay.group(1))
        # worst-case half extent, rotation-agnostic
        rad = max(sw, sh) / 2.0
        k, i = (ref, pn), 0
        while k in POS:
            i += 1; k = (ref, '%s#%d' % (pn, i))
        POS[k] = (round(gx, 2), round(gy, 2), nt.group(1) if nt else '')
        META[k] = (rad, both)

if __name__ == '__main__':
    for ref in WANT:
        pins = sorted([k for k in POS if k[0] == ref], key=lambda k: (len(k[1]), k[1]))
        if not pins:
            print('%s: not found' % ref)
            continue
        print('%s  (rot applied)' % ref)
        for k in pins:
            x, y, net = POS[k]
            print('   pad %-3s  (%7.2f, %7.2f)  %s' % (k[1], x, y, net or '-'))
        print()
