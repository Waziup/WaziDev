# -*- coding: utf-8 -*-
"""Will the ground pour actually connect every GND pad once it is filled?

   kicad-cli does not fill zones, so its unconnected count says nothing about
   the pours. This rasterises what is left of each GND zone after every foreign
   track, pad and via takes its clearance, then floods from one GND pad and
   reports any pad the copper cannot reach. Through-hole GND pads tie the two
   layers together.
"""
from __future__ import print_function
import io, re, sys, math, codecs
sys.stdout = codecs.getwriter('ascii')(sys.stdout.buffer, 'replace')
from pads import POS, META

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
NET = 'GND'
STEP = 0.25
CLEAR = 0.3                      # the zone's own clearance
ZONES = [(21.0, 21.0, 119.0, 67.0), (21.0, 67.0, 72.0, 119.0)]
# higher-priority pours keep GND out of their area entirely
KEEPOUT = [(56.0, 38.0, 76.0, 64.0), (78.0, 72.0, 94.0, 119.0), (96.0, 72.0, 116.0, 119.0)]
SLOTS = [(74.0, 68.8, 94.0, 71.2), (96.0, 68.8, 116.0, 71.2)]

s = io.open(PCB, encoding='utf-8').read()
netname = dict((int(m.group(1)), m.group(2)) for m in re.finditer(r'\(net (\d+) "([^"]*)"\)', s))

segs = []
for m in re.finditer(r'\(segment\s*\(start ([-0-9.]+) ([-0-9.]+)\)\s*\(end ([-0-9.]+) ([-0-9.]+)\)'
                     r'\s*\(width ([-0-9.]+)\)\s*\(layer "([^"]+)"\)\s*\(net (\d+)\)', s):
    g = [float(m.group(i)) for i in (1, 2, 3, 4, 5)]
    segs.append((g[0], g[1], g[2], g[3], g[4], m.group(6), netname.get(int(m.group(7)), '?')))
vias = [(float(m.group(1)), float(m.group(2)), float(m.group(3)), netname.get(int(m.group(4)), '?'))
        for m in re.finditer(r'\(via\s*\(at ([-0-9.]+) ([-0-9.]+)\)\s*\(size ([-0-9.]+)\)'
                             r'[\s\S]{0,60}?\(net (\d+)\)', s)]

X0, Y0, NX, NY = 20.0, 20.0, int(100 / STEP) + 1, int(100 / STEP) + 1
def gx(x): return int(round((x - X0) / STEP))
def gy(y): return int(round((y - Y0) / STEP))

free = {}
for lay in ('F.Cu', 'B.Cu'):
    free[lay] = bytearray(NX * NY)
    for iy in range(NY):
        y = Y0 + iy * STEP
        for ix in range(NX):
            x = X0 + ix * STEP
            ok = any(a <= x <= c and b <= y <= d for a, b, c, d in ZONES)
            if ok and any(a <= x <= c and b <= y <= d for a, b, c, d in KEEPOUT):
                ok = False
            if ok and any(a - CLEAR <= x <= c + CLEAR and b - CLEAR <= y <= d + CLEAR
                          for a, b, c, d in SLOTS):
                ok = False
            free[lay][iy * NX + ix] = 1 if ok else 0


def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0.0 if L == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / L))
    return math.hypot(px - ax - t * dx, py - ay - t * dy)


def block(lay, cx, cy, r):
    g = free[lay]
    for iy in range(max(0, gy(cy - r)), min(NY, gy(cy + r) + 1)):
        for ix in range(max(0, gx(cx - r)), min(NX, gx(cx + r) + 1)):
            if math.hypot(X0 + ix * STEP - cx, Y0 + iy * STEP - cy) <= r:
                g[iy * NX + ix] = 0


for ax, ay, bx, by, w, lay, net in segs:
    if net == NET:
        continue
    r = w / 2.0 + CLEAR
    n = int(max(abs(bx - ax), abs(by - ay)) / STEP) + 1
    for i in range(n + 1):
        t = float(i) / n
        block(lay, ax + (bx - ax) * t, ay + (by - ay) * t, r)

for k, (x, y, pnet) in POS.items():
    if pnet == NET:
        continue
    rad, both = META[k]
    for lay in (('F.Cu', 'B.Cu') if both else ('F.Cu',)):
        block(lay, x, y, rad + CLEAR)
for vx, vy, vs, vnet in vias:
    if vnet == NET:
        continue
    for lay in ('F.Cu', 'B.Cu'):
        block(lay, vx, vy, vs / 2.0 + CLEAR)

# GND's own tracks are copper too, and its vias tie the layers
for ax, ay, bx, by, w, lay, net in segs:
    if net != NET:
        continue
    n = int(max(abs(bx - ax), abs(by - ay)) / STEP) + 1
    for i in range(n + 1):
        t = float(i) / n
        px, py = ax + (bx - ax) * t, ay + (by - ay) * t
        for jy in range(max(0, gy(py - w / 2)), min(NY, gy(py + w / 2) + 1)):
            for jx in range(max(0, gx(px - w / 2)), min(NX, gx(px + w / 2) + 1)):
                free[lay][jy * NX + jx] = 1

# --- flood from the first GND pad ------------------------------------------
gnd = sorted([(k, v) for k, v in POS.items() if v[2] == NET])
tie = {}
for k, (x, y, _) in gnd:
    tie[k] = [l for l in (('F.Cu', 'B.Cu') if META[k][1] else ('F.Cu',))]

seen = dict((l, bytearray(NX * NY)) for l in ('F.Cu', 'B.Cu'))
start = None
for k, (x, y, _) in gnd:
    if free['F.Cu'][gy(y) * NX + gx(x)]:
        start = ('F.Cu', gx(x), gy(y))
        break
if start is None:
    print('no GND pad sits on free copper at all')
    sys.exit(1)

stack = [start]
seen[start[0]][start[2] * NX + start[1]] = 1
link = {}
for k, (x, y, _) in gnd:
    if META[k][1]:
        link.setdefault((gx(x), gy(y)), True)
for vx, vy, vs, vnet in vias:
    if vnet == NET:
        link.setdefault((gx(vx), gy(vy)), True)
        for lay in ('F.Cu', 'B.Cu'):
            free[lay][gy(vy) * NX + gx(vx)] = 1
while stack:
    lay, ix, iy = stack.pop()
    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        jx, jy = ix + dx, iy + dy
        if 0 <= jx < NX and 0 <= jy < NY and free[lay][jy * NX + jx] and not seen[lay][jy * NX + jx]:
            seen[lay][jy * NX + jx] = 1
            stack.append((lay, jx, jy))
    if (ix, iy) in link:
        other = 'B.Cu' if lay == 'F.Cu' else 'F.Cu'
        if free[other][iy * NX + ix] and not seen[other][iy * NX + ix]:
            seen[other][iy * NX + ix] = 1
            stack.append((other, ix, iy))

bad = []
for k, (x, y, _) in gnd:
    hit = False
    for lay in tie[k]:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                jx, jy = gx(x) + dx, gy(y) + dy
                if 0 <= jx < NX and 0 <= jy < NY and seen[lay][jy * NX + jx]:
                    hit = True
    if not hit:
        bad.append('%s.%s at (%.2f, %.2f)' % (k[0], k[1], x, y))

print('%d GND pads, grid %.2f mm' % (len(gnd), STEP))
if bad:
    print('\nNOT reached by the pour:')
    for b in bad:
        print('  ' + b)
else:
    print('\nEvery GND pad is reachable through the pour.')
