# -*- coding: utf-8 -*-
"""What exactly differs between the board's footprints and the library?

   KiCad reports eleven lib_footprint_mismatch warnings. The warning does not
   say whether the difference is in the copper or only in the bookkeeping, and
   that is the whole question: copper would reach the fabricator, bookkeeping
   would not. This compares pad number, position, size and drill, and the
   graphic outlines, between each board footprint and its library file.
"""
from __future__ import print_function
import io, os, re, sys, codecs, glob
sys.stdout = codecs.getwriter('ascii')(sys.stdout.buffer, 'replace')

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
LIBS = [r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniSpace.pretty',
        r'C:\Program Files\KiCad\10.0\share\kicad\footprints\Resistor_SMD.pretty',
        r'C:\Program Files\KiCad\10.0\share\kicad\footprints\TerminalBlock_4Ucon.pretty']


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


def fingerprint(txt):
    """Only the things that become copper, mask, paste or a hole."""
    pads = []
    for a, b in blocks(txt, '(pad "'):
        p = txt[a:b]
        num = re.match(r'\(pad "([^"]*)"', p).group(1)
        at = re.search(r'\(at ([-0-9.]+) ([-0-9.]+)', p)
        sz = re.search(r'\(size ([-0-9.]+) ([-0-9.]+)\)', p)
        dr = re.search(r'\(drill ([-0-9.]+)', p)
        shape = re.match(r'\(pad "[^"]*" (\w+) (\w+)', p)
        pads.append((num, round(float(at.group(1)), 4), round(float(at.group(2)), 4),
                     round(float(sz.group(1)), 4), round(float(sz.group(2)), 4),
                     round(float(dr.group(1)), 4) if dr else None,
                     shape.group(1), shape.group(2)))
    lines = []
    for m in re.finditer(r'\(fp_(line|circle|arc|poly|rect)([\s\S]{0,500}?)\(layer "([^"]+)"\)', txt):
        coords = tuple(round(float(v), 4) for v in re.findall(r'[-0-9.]+', m.group(2))
                       if re.match(r'^-?[0-9.]+$', v))
        lines.append((m.group(1), m.group(3), coords))
    return sorted(pads), sorted(lines)


lib = {}
for d in LIBS:
    for f in glob.glob(os.path.join(d, '*.kicad_mod')):
        lib[os.path.splitext(os.path.basename(f))[0]] = io.open(f, encoding='utf-8').read()

s = io.open(PCB, encoding='utf-8').read()
seen = set()
print('%-26s %-6s %s' % ('Footprint', 'Ref', 'Vergleich Pads / Grafik gegen die Bibliothek'))
for a, b in blocks(s, '(footprint "'):
    blk = s[a:b]
    name = re.match(r'\(footprint "([^"]*)"', blk).group(1).split(':')[-1]
    ref = re.search(r'\(property "Reference" "([^"]*)"', blk)
    if not ref or name not in lib:
        continue
    bp, bl = fingerprint(blk)
    lp, ll = fingerprint(lib[name])
    same_pads = bp == lp
    same_gfx = bl == ll
    msg = 'Pads %s, Grafik %s' % ('gleich' if same_pads else 'UNTERSCHIEDLICH',
                                  'gleich' if same_gfx else 'UNTERSCHIEDLICH')
    print('%-26s %-6s %s' % (name[:26], ref.group(1), msg))
    if not same_pads:
        for x, y in zip(bp, lp):
            if x != y:
                print('      Platine %s' % (x,))
                print('      Library %s' % (y,))
