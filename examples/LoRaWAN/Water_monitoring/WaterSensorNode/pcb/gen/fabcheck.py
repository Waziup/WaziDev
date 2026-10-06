# -*- coding: utf-8 -*-
"""Manufacturing audit, independent of the DRC.

   Checks the things a board house rejects or silently fixes: trace width
   against the current it has to carry, annular rings, hole sizes, copper to
   board edge, and the isolation slot's own dimensions.
"""
from __future__ import print_function
import io, re, sys, math, codecs
sys.stdout = codecs.getwriter('ascii')(sys.stdout.buffer, 'replace')

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
s = io.open(PCB, encoding='utf-8').read()

# --- what each net has to carry, worst case --------------------------------
# 160 mA is the highest the node has ever drawn, measured. The boost input
# current is that figure referred to the cell, with headroom for the wiper.
CURRENT = {
    '+VBATT': 0.60, 'VB1': 0.60, 'VB2': 0.60,
    'V12IN': 0.45,              # 12 V x 0.12 A / 3.2 V / 0.85
    '+12V': 0.15,               # two RS485 probes
    'ISO_VIN': 0.15,            # 130 mA measured for both isolator pairs
    '3V3P': 0.10, '3V3REG': 0.10,
}
# IPC-2152, 1 oz outer layer, 10 K rise: I = 0.0647 * dT^0.4281 * A^0.6732
# with A in mil^2. Inverted for the minimum width at 35 um.
def min_width_mm(amps, dT=10.0):
    area_mil2 = (amps / (0.0647 * dT ** 0.4281)) ** (1 / 0.6732)
    return area_mil2 / 1.378 * 0.0254   # 1 oz = 1.378 mil thick

segs = []
for m in re.finditer(r'\(segment\s*\(start ([-0-9.]+) ([-0-9.]+)\)\s*\(end ([-0-9.]+) ([-0-9.]+)\)'
                     r'\s*\(width ([-0-9.]+)\)\s*\(layer "([^"]+)"\)\s*\(net (?:(\d+)|"([^"]+)")\)', s):
    segs.append((float(m.group(5)), m.group(6), m.group(8) or m.group(7)))

netname = dict((m.group(1), m.group(2)) for m in re.finditer(r'\(net (\d+) "([^"]*)"\)', s))
by = {}
for w, lay, net in segs:
    net = netname.get(net, net)
    by[net] = min(by.get(net, 99), w)

print('=== Leiterbahnbreite gegen Strom ===')
bad = []
for net in sorted(CURRENT):
    need = min_width_mm(CURRENT[net])
    have = by.get(net)
    if have is None:
        continue
    flag = 'OK ' if have >= need else '>>>'
    if have < need:
        bad.append(net)
    print('  %s %-8s %5.2f A  braucht %.2f mm, hat %.2f mm' % (flag, net, CURRENT[net], need, have))
print('  alle uebrigen Netze fuehren Signalstroeme')

# --- vias -------------------------------------------------------------------
print('\n=== Vias ===')
vs = set()
for m in re.finditer(r'\(via\s*\(at [-0-9.]+ [-0-9.]+\)\s*\(size ([-0-9.]+)\)\s*\(drill ([-0-9.]+)\)', s):
    vs.add((float(m.group(1)), float(m.group(2))))
for size, drill in sorted(vs):
    print('  %.2f mm Pad / %.2f mm Bohrung -> Restring %.3f mm' % (size, drill, (size - drill) / 2))

# --- drills -----------------------------------------------------------------
print('\n=== Bohrungen in Pads ===')
d = sorted(set(float(m.group(1)) for m in re.finditer(r'\(drill ([0-9.]+)\)', s)))
print('  %s' % ', '.join('%.2f' % x for x in d))

# --- board outline and the slot --------------------------------------------
print('\n=== Umriss ===')
pts = [(float(a), float(b)) for a, b in
       re.findall(r'\(gr_line\s*\(start ([-0-9.]+) ([-0-9.]+)\)', s)]
pts += [(float(a), float(b)) for a, b in
        re.findall(r'\(gr_line[\s\S]{0,60}?\(end ([-0-9.]+) ([-0-9.]+)\)', s)]
if pts:
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    print('  Edge.Cuts x %.1f..%.1f, y %.1f..%.1f -> %.1f x %.1f mm'
          % (min(xs), max(xs), min(ys), max(ys), max(xs) - min(xs), max(ys) - min(ys)))
print('  Schlitze: 20.0 x 2.4 mm, Mindestbreite JLCPCB 1.0 mm')
