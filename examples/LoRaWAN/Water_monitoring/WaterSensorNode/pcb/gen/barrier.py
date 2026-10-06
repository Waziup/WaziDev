# -*- coding: utf-8 -*-
"""Does any copper cross an isolation barrier? The one check this board exists for."""
from __future__ import print_function
import io, re, sys, codecs
sys.stdout = codecs.getwriter('ascii')(sys.stdout.buffer, 'replace')

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
SLOTS = [('pH', 74.0, 94.0, 68.8, 71.2), ('EC', 96.0, 116.0, 68.8, 71.2)]

s = io.open(PCB, encoding='utf-8').read()
netname = dict((int(m.group(1)), m.group(2)) for m in re.finditer(r'\(net (\d+) "([^"]*)"\)', s))

segs = []
for m in re.finditer(r'\(segment\s*\(start ([-0-9.]+) ([-0-9.]+)\)\s*\(end ([-0-9.]+) ([-0-9.]+)\)'
                     r'\s*\(width [-0-9.]+\)\s*\(layer "([^"]+)"\)\s*\(net (\d+)\)', s):
    x1, y1, x2, y2 = (float(m.group(i)) for i in (1, 2, 3, 4))
    segs.append((x1, y1, x2, y2, m.group(5), netname.get(int(m.group(6)), '?')))

print('%d track segments' % len(segs))
bad = []
for name, sx1, sx2, sy1, sy2 in SLOTS:
    for x1, y1, x2, y2, lay, net in segs:
        # does the segment span the barrier band, within the slot's x range?
        if min(y1, y2) < sy1 and max(y1, y2) > sy2:
            if max(min(x1, x2), sx1) <= min(max(x1, x2), sx2):
                bad.append('%s barrier crossed by %s on %s: (%.2f,%.2f)-(%.2f,%.2f)'
                           % (name, net, lay, x1, y1, x2, y2))

# zones must not span it either
zones = re.findall(r'\(zone\s*\(net \d+\)\s*\(net_name "([^"]*)"\)\s*\(layer "([^"]+)"[\s\S]*?'
                   r'\(polygon\s*\(pts\s*((?:\(xy [-0-9.]+ [-0-9.]+\)\s*)+)\)', s)
for net, lay, pts in zones:
    ys = [float(b) for a, b in re.findall(r'\(xy ([-0-9.]+) ([-0-9.]+)\)', pts)]
    xs = [float(a) for a, b in re.findall(r'\(xy ([-0-9.]+) ([-0-9.]+)\)', pts)]
    for name, sx1, sx2, sy1, sy2 in SLOTS:
        if min(ys) < sy1 and max(ys) > sy2 and max(min(xs), sx1) <= min(max(xs), sx2):
            bad.append('%s barrier spanned by ZONE %s on %s  (y %.1f..%.1f, x %.1f..%.1f)'
                       % (name, net, lay, min(ys), max(ys), min(xs), max(xs)))

print()
if bad:
    print('BARRIER VIOLATIONS:')
    for b in sorted(set(bad)):
        print('  ' + b)
else:
    print('No track and no zone crosses either barrier.')

# and report which nets live on each side
print()
for name, sx1, sx2, sy1, sy2 in SLOTS:
    below = set(n for x1, y1, x2, y2, l, n in segs
                if min(y1, y2) > sy2 and max(min(x1, x2), sx1) <= min(max(x1, x2), sx2))
    print('%s: nets routed below the barrier -> %s' % (name, ', '.join(sorted(below)) or 'none'))
