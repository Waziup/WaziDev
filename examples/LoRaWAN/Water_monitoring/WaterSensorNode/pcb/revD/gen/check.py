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
from pads import POS, META, SHAPE

from paths import PCB
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


def local(p, sh):
    """A board point in a pad's own frame (KiCad rotates clockwise)."""
    cx, cy, hx, hy, ang = sh
    th = math.radians(ang)
    dx, dy = p[0] - cx, p[1] - cy
    return (dx * math.cos(th) - dy * math.sin(th), dx * math.sin(th) + dy * math.cos(th))


def pt_rect(p, sh):
    lx, ly = local(p, sh)
    return math.hypot(max(abs(lx) - sh[2], 0.0), max(abs(ly) - sh[3], 0.0))


def seg_rect(a, b, sh):
    """Distance from a track's centre line to a pad's real copper rectangle.
       Rev D: the pads used to be circles of their long half-side, which is
       fine for a terminal and calls neighbouring SOIC pads 0.6 mm apart a
       collision."""
    la, lb = local(a, sh), local(b, sh)
    hx, hy = sh[2], sh[3]
    if (abs(la[0]) <= hx and abs(la[1]) <= hy) or (abs(lb[0]) <= hx and abs(lb[1]) <= hy):
        return 0.0
    cs = [(-hx, -hy), (hx, -hy), (hx, hy), (-hx, hy)]
    best = min(pt_rect(a, sh), pt_rect(b, sh))
    for i in range(4):
        best = min(best, seg_seg(la, lb, cs[i], cs[(i + 1) % 4]))
    return best


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

def pads_under(p, lay):
    """Every pad whose real copper contains the point, on that layer."""
    out = []
    for k, sh in SHAPE.items():
        if not META[k][1] and lay != 'F.Cu':
            continue
        if pt_rect(p, sh) < 1e-6:
            out.append((k[0], k[1], POS[k][2]))
    return out


for a, b, w, lay, net in segs:
    for p in (a, b):
        here = pads_under(p, lay)
        if here:
            if not any(h[2] == net for h in here):
                bad.append('%-9s %s endpoint (%.2f,%.2f) lands on %s [net %s]'
                           % (net, lay, p[0], p[1],
                              '/'.join(h[0] + '.' + h[1] for h in here), here[0][2] or 'none'))
            continue
        key = (round(p[0], 2), round(p[1], 2))
        if via_at.get(key) == net:
            continue
        shared = sum(1 for x, y, _, _, n in segs if n == net and (x == p or y == p))
        # a T-junction: the end lands part-way along a track of its own net
        tee = any(n == net and l2 == lay and x != p and y != p and seg_dist(p, x, y) < 1e-3
                  for x, y, _, l2, n in segs)
        if shared < 2 and not tee:
            bad.append('%-9s %s endpoint (%.2f,%.2f) dead end' % (net, lay, p[0], p[1]))

# --- 2. track vs foreign pad ----------------------------------------------
for a, b, w, lay, net in segs:
    for k, (x, y, pnet) in POS.items():
        if pnet == net:
            continue
        rad, both = META[k]
        if not both and lay != 'F.Cu':
            continue
        d = seg_rect(a, b, SHAPE[k])
        need = w / 2.0 + CLEAR
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
        d = pt_rect((vx, vy), SHAPE[k])
        if d < vs / 2.0 + CLEAR:
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

# --- 6. bare board round the WaziSense standoffs ---------------------------
#  Rev D: the WaziSense sits on three M2.5 standoffs (H5-H7). The standoff's
#  hex foot (5 mm across flats, 2.9 mm to a corner) and the screw head on the
#  back sit on solder mask; no track or via of any net within 3.2 mm of a
#  centre, on either layer.
KEEP = 3.2
n_std = 0
for m in re.finditer(r'\(footprint "MountingHole:MountingHole_2\.7mm_M2\.5"[\s\S]*?\(at ([-0-9.]+) ([-0-9.]+)', s):
    hc = (float(m.group(1)), float(m.group(2)))
    n_std += 1
    for a, b, w, lay, net in segs:
        d = seg_dist(hc, a, b) - w / 2.0
        if d < KEEP:
            bad.append('STANDOFF (%.2f,%.2f): %s on %s only %.2f from the centre, needs %.1f'
                       % (hc[0], hc[1], net, lay, d, KEEP))
    for (vx, vy), vs, vnet in vias:
        d = math.hypot(vx - hc[0], vy - hc[1]) - vs / 2.0
        if d < KEEP:
            bad.append('STANDOFF (%.2f,%.2f): via %s only %.2f from the centre, needs %.1f'
                       % (hc[0], hc[1], vnet, d, KEEP))
if n_std != 3:
    bad.append('STANDOFF: %d WaziSense standoff holes on the board, want 3' % n_std)

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
    pads = [k for k, v in POS.items() if v[2] == net]
    pts = reach.get(net, set())
    mine = [(a, b) for a, b, w, lay, n in segs if n == net]
    miss = [k[0] + '.' + k[1] for k in pads
            if not any(pt_rect(q, SHAPE[k]) < 1e-6 for q in pts)
            and not any(seg_rect(a, b, SHAPE[k]) < 1e-6 for a, b in mine)]
    if miss:
        tot += len(miss)
        print('  %-9s %2d/%2d unrouted: %s' % (net, len(miss), len(pads), ', '.join(sorted(miss))))
print('\n%d pads still unrouted (zones not counted)' % tot)
