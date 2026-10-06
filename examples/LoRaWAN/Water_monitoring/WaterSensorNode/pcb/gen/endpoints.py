# -*- coding: utf-8 -*-
"""Every track endpoint must sit on a pad of its own net, or on another track of
   the same net. Catches exactly the failure the pad mirroring introduced:
   a trace that still lands where the old pad used to be."""
from __future__ import print_function
import io, re, sys, codecs
sys.stdout = codecs.getwriter('ascii')(sys.stdout.buffer, 'replace')
from pads import POS

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
s = io.open(PCB, encoding='utf-8').read()
netname = dict((int(m.group(1)), m.group(2)) for m in re.finditer(r'\(net (\d+) "([^"]*)"\)', s))

segs = []
for m in re.finditer(r'\(segment\s*\(start ([-0-9.]+) ([-0-9.]+)\)\s*\(end ([-0-9.]+) ([-0-9.]+)\)'
                     r'\s*\(width [-0-9.]+\)\s*\(layer "([^"]+)"\)\s*\(net (\d+)\)', s):
    segs.append((round(float(m.group(1)), 2), round(float(m.group(2)), 2),
                 round(float(m.group(3)), 2), round(float(m.group(4)), 2),
                 netname.get(int(m.group(6)), '?')))

# where does each net have copper?
ends = {}
for x1, y1, x2, y2, net in segs:
    ends.setdefault(net, set()).update([(x1, y1), (x2, y2)])

pad_at = {}
for (ref, pn), (x, y, net) in POS.items():
    pad_at.setdefault((x, y), []).append((ref, pn, net))

print('%d segments\n' % len(segs))
bad = []
for x1, y1, x2, y2, net in segs:
    for (x, y) in ((x1, y1), (x2, y2)):
        here = pad_at.get((x, y))
        if here:
            if not any(p[2] == net for p in here):
                bad.append('%-9s endpoint (%.2f, %.2f) lands on %s which is net %s'
                           % (net, x, y, '/'.join(p[0] + '.' + p[1] for p in here),
                              here[0][2] or 'none'))
        else:
            # not a pad: fine only if another segment of the same net shares it
            shared = sum(1 for a, b, c, d, n in segs
                         if n == net and ((a, b) == (x, y) or (c, d) == (x, y)))
            if shared < 2:
                bad.append('%-9s endpoint (%.2f, %.2f) is a dead end -- no pad, no junction'
                           % (net, x, y))

if bad:
    print('ENDPOINT PROBLEMS:')
    for b in sorted(set(bad)):
        print('  ' + b)
else:
    print('Every track endpoint sits on a pad of its own net or on a junction.')

# which pads of the routed nets are still untouched?
print()
routed = set(n for _, _, _, _, n in segs)
for net in sorted(routed):
    miss = [r + '.' + p for (r, p), (x, y, nn) in sorted(POS.items())
            if nn == net and (x, y) not in ends[net]]
    if miss:
        print('  %-9s pads not reached: %s' % (net, ', '.join(miss)))
