# -*- coding: utf-8 -*-
"""Geometric check of the routing before KiCad ever sees it.

   - every track endpoint sits on a pad of its own net or on a junction
   - no track passes too close to a foreign pad
   - no two tracks of different nets on the same layer come too close
   - nothing crosses an isolation barrier
"""
from __future__ import print_function
import io, re, sys, math, codecs
sys.stdout = codecs.getwriter('ascii')(sys.stdout.buffer, 'replace')
from pads import POS, META

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
CLEAR = 0.2
SLOTS = [('pH', 74.0, 94.0, 68.8, 71.2), ('EC', 96.0, 116.0, 68.8, 71.2)]


def seg_dist(p, a, b):
    (px, py), (ax, ay), (bx, by) = p, a, b
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0.0 if L == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / L))
    return math.hypot(px - ax - t * dx, py - ay - t * dy)


def seg_seg(a, b, c, d):
    def cr(o, p, q):
        return (p[0] - o[0]) * (q[1] - o[1]) - (p[1] - o[1]) * (q[0] - o[0])
    d1, d2, d3, d4 = cr(c, d, a), cr(c, d, b), cr(a, b, c), cr(a, b, d)
    if ((d1 > 0) != (d2 > 0)) and ((d3 > 0) != (d4 > 0)):
        return 0.0
    return min(seg_dist(a, c, d), seg_dist(b, c, d), seg_dist(c, a, b), seg_dist(d, a, b))


s = io.open(PCB, encoding='utf-8').read()
netname = dict((int(m.group(1)), m.group(2)) for m in re.finditer(r'\(net (\d+) "([^"]*)"\)', s))

segs = []
for m in re.finditer(r'\(segment\s*\(start ([-0-9.]+) ([-0-9.]+)\)\s*\(end ([-0-9.]+) ([-0-9.]+)\)'
                     r'\s*\(width ([-0-9.]+)\)\s*\(layer "([^"]+)"\)\s*\(net (\d+)\)', s):
    g = [float(m.group(i)) for i in (1, 2, 3, 4, 5)]
    segs.append(((round(g[0], 3), round(g[1], 3)), (round(g[2], 3), round(g[3], 3)),
                 g[4], m.group(6), netname.get(int(m.group(7)), '?')))

vias = []
for m in re.finditer(r'\(via\s*\(at ([-0-9.]+) ([-0-9.]+)\)\s*\(size ([-0-9.]+)\)[\s\S]{0,60}?\(net (\d+)\)', s):
    vias.append(((round(float(m.group(1)), 3), round(float(m.group(2)), 3)),
                 float(m.group(3)), netname.get(int(m.group(4)), '?')))

bad = []

# --- 1. endpoints ----------------------------------------------------------
pad_at = {}
for k, (x, y, net) in POS.items():
    pad_at.setdefault((round(x, 2), round(y, 2)), []).append((k[0], k[1], net))
via_at = dict(((v[0][0], v[0][1]), v[2]) for v in vias)

for a, b, w, lay, net in segs:
    for p in (a, b):
        key = (round(p[0], 2), round(p[1], 2))
        here = pad_at.get(key)
        if here:
            if not any(h[2] == net for h in here):
                bad.append('%-9s %s endpoint (%.2f,%.2f) lands on %s [net %s]'
                           % (net, lay, p[0], p[1],
                              '/'.join(h[0] + '.' + h[1] for h in here), here[0][2] or 'none'))
            continue
        if via_at.get(key) == net:
            continue
        shared = sum(1 for x, y, _, _, n in segs if n == net and (x == p or y == p))
        if shared < 2:
            bad.append('%-9s %s endpoint (%.2f,%.2f) dead end' % (net, lay, p[0], p[1]))

# --- 2. track vs foreign pad ----------------------------------------------
for a, b, w, lay, net in segs:
    for k, (x, y, pnet) in POS.items():
        if pnet == net:
            continue
        rad, both = META[k]
        if not both and lay != 'F.Cu':
            continue
        d = seg_dist((x, y), a, b)
        need = rad + w / 2.0 + CLEAR
        if d < need:
            bad.append('%-9s %s track (%.2f,%.2f)-(%.2f,%.2f) is %.2f from %s.%s [%s], needs %.2f'
                       % (net, lay, a[0], a[1], b[0], b[1], d, k[0], k[1], pnet or 'none', need))

# --- 3. track vs track ------------------------------------------------------
for i in range(len(segs)):
    a, b, w, lay, net = segs[i]
    for j in range(i + 1, len(segs)):
        c, d2, w2, lay2, net2 = segs[j]
        if net == net2 or lay != lay2:
            continue
        d = seg_seg(a, b, c, d2)
        need = w / 2.0 + w2 / 2.0 + CLEAR
        if d < need:
            bad.append('%s x %s on %s: %.2f apart at (%.1f,%.1f)-(%.1f,%.1f) / (%.1f,%.1f)-(%.1f,%.1f), needs %.2f'
                       % (net, net2, lay, d, a[0], a[1], b[0], b[1], c[0], c[1], d2[0], d2[1], need))

# --- 4. via vs foreign pad / track -----------------------------------------
for (vx, vy), vs, vnet in vias:
    for k, (x, y, pnet) in POS.items():
        if pnet == vnet:
            continue
        rad, both = META[k]
        d = math.hypot(vx - x, vy - y)
        if d < rad + vs / 2.0 + CLEAR:
            bad.append('via %s at (%.2f,%.2f) is %.2f from %s.%s [%s]'
                       % (vnet, vx, vy, d, k[0], k[1], pnet or 'none'))
    for a, b, w, lay, net in segs:
        if net == vnet:
            continue
        d = seg_dist((vx, vy), a, b)
        if d < vs / 2.0 + w / 2.0 + CLEAR:
            bad.append('via %s at (%.2f,%.2f) is %.2f from %s track on %s'
                       % (vnet, vx, vy, d, net, lay))

# --- 5. the barrier ---------------------------------------------------------
for name, sx1, sx2, sy1, sy2 in SLOTS:
    for a, b, w, lay, net in segs:
        if min(a[1], b[1]) < sy1 and max(a[1], b[1]) > sy2:
            if max(min(a[0], b[0]), sx1) <= min(max(a[0], b[0]), sx2):
                bad.append('%s BARRIER crossed by %s on %s' % (name, net, lay))
    for (vx, vy), vs, vnet in vias:
        if sy1 - 1 < vy < sy2 + 1 and sx1 < vx < sx2:
            bad.append('%s BARRIER: via %s sits in the slot' % (name, vnet))

print('%d segments, %d vias' % (len(segs), len(vias)))
if bad:
    print('\n%d PROBLEMS:' % len(set(bad)))
    for b in sorted(set(bad)):
        print('  ' + b)
else:
    print('\nClean: endpoints, pad clearance, track clearance, vias, barriers.')

# --- what is still unrouted -------------------------------------------------
print()
reach = {}
for a, b, w, lay, net in segs:
    reach.setdefault(net, set()).update([a, b])
for (vx, vy), vs, vnet in vias:
    reach.setdefault(vnet, set()).add((vx, vy))
tot = 0
for net in sorted(set(v[2] for v in POS.values() if v[2])):
    pads = [(k, (round(v[0], 3), round(v[1], 3))) for k, v in POS.items() if v[2] == net]
    miss = [k[0] + '.' + k[1] for k, p in pads if p not in reach.get(net, set())]
    if miss:
        tot += len(miss)
        print('  %-9s %2d/%2d unrouted: %s' % (net, len(miss), len(pads), ', '.join(sorted(miss))))
print('\n%d pads still unrouted (zones not counted)' % tot)
