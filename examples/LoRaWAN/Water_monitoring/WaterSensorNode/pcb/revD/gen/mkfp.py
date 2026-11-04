# -*- coding: utf-8 -*-
"""The four custom footprints, plus a minimal board to prove the toolchain."""

from __future__ import print_function

import sys as _sys
_GENERATOR_GUARD = True
if __name__ != '__main__':
    raise ImportError('mkfp.py rewrites the board file; run it, do not import it')
import io, os, hashlib

from paths import PCBDIR as OUT
LIB = os.path.join(OUT, 'KijaniSpace.pretty')
for d in (OUT, LIB):
    if not os.path.isdir(d):
        os.makedirs(d)

FPVER = '20260206'
PCBVER = '20241229'

def uid(seed):
    h = hashlib.md5(seed.encode('utf-8')).hexdigest()
    return '%s-%s-%s-%s-%s' % (h[0:8], h[8:12], h[12:16], h[16:20], h[20:32])

def pad(n, x, y, seed, shape='circle', dia=1.8, drill=1.05):
    return ('\t(pad "%d" thru_hole %s\n'
            '\t\t(at %s %s)\n'
            '\t\t(size %s %s)\n'
            '\t\t(drill %s)\n'
            '\t\t(layers "*.Cu" "*.Mask")\n'
            '\t\t(uuid "%s")\n'
            '\t)\n') % (n, shape, x, y, dia, dia, drill, uid(seed))

def rect(x1, y1, x2, y2, layer, width, seed, gap=0):
    """gap>0 breaks the two long edges around local x=0, for a footprint the
       isolation slot runs through. Silkscreen over a slot is not printed, and
       the rule for this board is that nothing crosses a barrier."""
    out = ''
    if gap:
        pts = [(x1, y1, -gap, y1), (gap, y1, x2, y1),
               (x2, y1, x2, y2),
               (x2, y2, gap, y2), (-gap, y2, x1, y2),
               (x1, y2, x1, y1)]
    else:
        pts = [(x1, y1, x2, y1), (x2, y1, x2, y2), (x2, y2, x1, y2), (x1, y2, x1, y1)]
    for i, (ax, ay, bx, by) in enumerate(pts):
        out += ('\t(fp_line\n\t\t(start %s %s)\n\t\t(end %s %s)\n'
                '\t\t(stroke (width %s) (type solid))\n\t\t(layer "%s")\n\t\t(uuid "%s")\n\t)\n'
                ) % (ax, ay, bx, by, width, layer, uid(seed + str(i)))
    return out

def header(name, descr):
    return ('(footprint "%s"\n'
            '\t(version %s)\n'
            '\t(generator "kijani-gen")\n'
            '\t(layer "F.Cu")\n'
            '\t(descr "%s")\n'
            '\t(attr through_hole)\n'
            '\t(property "Reference" "REF**"\n\t\t(at 0 -%s 0)\n\t\t(layer "F.SilkS")\n'
            '\t\t(uuid "%s")\n'
            '\t\t(effects (font (size 1 1) (thickness 0.15)))\n\t)\n'
            '\t(property "Value" "%s"\n\t\t(at 0 %s 0)\n\t\t(layer "F.Fab")\n'
            '\t\t(uuid "%s")\n'
            '\t\t(effects (font (size 1 1) (thickness 0.15)))\n\t)\n'
            ) % (name, FPVER, descr, 9.5, uid(name + 'ref'), name, 9.5, uid(name + 'val'))

def label(x, y, txt, rot, seed, size=0.8):
    return ('\t(fp_text user "%s"\n\t\t(at %s %s %s)\n\t\t(layer "F.SilkS")\n\t\t(uuid "%s")\n'
            '\t\t(effects (font (size %s %s) (thickness 0.12)))\n\t)\n'
            ) % (txt, x, y, rot, uid(seed), size, size)

def build(name, descr, pads, bw, bh, cw, ch, gap=0, cx=0.0, pin1=True, pin1_x=None):
    """pads: list of (number, x, y, label). cx offsets the body and the
       courtyard, for a part whose pin 1 is not in the middle."""
    s = header(name, descr)
    s += rect(cx - bw / 2.0, -bh / 2.0, cx + bw / 2.0, bh / 2.0, 'F.SilkS', 0.12, name + 'silk', gap)
    s += rect(cx - cw / 2.0, -ch / 2.0, cx + cw / 2.0, ch / 2.0, 'F.CrtYd', 0.05, name + 'crt')
    for n, x, y, lab in pads:
        if name.startswith('TerminalBlock'):
            s += pad(n, x, y, '%s-p%d' % (name, n),
                     'roundrect' if n == 1 else 'circle', dia=2.6, drill=1.3)
        else:
            s += pad(n, x, y, '%s-p%d' % (name, n), 'rect' if n == 1 else 'circle')

    # Pin 1 is the square pad; name it on the silkscreen so the module's own
    # marking can be checked against the board before anything is soldered.
    if pin1:
        n1, x1, y1, lab1 = pads[0]
        tx = (bw / 2.0 + 3.4) * (1 if x1 > 0 else -1)
        if pin1_x is not None:
            tx = pin1_x
        s += label(round(tx, 2), y1, '1:' + lab1, 0, name + '-pin1', 1.0)
    s += ')\n'
    return s

FPS = {}

# ---- EZO circuit: 2 x 3, rows 17.78 apart, 2.54 pitch -----------------------
FPS['EZO_Circuit'] = build(
    'EZO_Circuit', 'Atlas Scientific EZO circuit. 1 GND 2 TX/SDA 3 RX/SCL 4 VCC 5 PRB 6 PGND',
    [(1, -8.89, 2.54, 'GND'), (2, -8.89, 0, 'TX'), (3, -8.89, -2.54, 'RX'),
     (4, 8.89, 2.54, 'VCC'), (5, 8.89, 0, 'PRB'), (6, 8.89, -2.54, 'PGND')],
    20.16, 13.97, 21.2, 15.0)

# ---- BE-IVI isolator: 2 x 5 ------------------------------------------------
FPS['BE_IVI_Isolator'] = build(
    'BE_IVI_Isolator', 'Atlas Basic EZO inline voltage isolator. In: VCC OFF GND A B. Out: 3V9 NC GND A B',
    [(1, -8.89, 5.08, 'VCC'), (2, -8.89, 2.54, 'OFF'), (3, -8.89, 0, 'GND'),
     (4, -8.89, -2.54, 'A'), (5, -8.89, -5.08, 'B'),
     (6, 8.89, 5.08, '3V9'), (7, 8.89, 2.54, 'NC'), (8, 8.89, 0, 'GNDi'),
     (9, 8.89, -2.54, 'Ai'), (10, 8.89, -5.08, 'Bi')],
    20.0, 14.5, 21.0, 15.5, gap=1.8)

# ---- Pololu U3V9F12: 3 pins on the short edge, 1.3 mm in -------------------
FPS['Pololu_U3V9F12'] = build(
    'Pololu_U3V9F12', 'Pololu U3V9F12 step-up 12V. 1 VOUT 2 GND 3 VIN. Pins on the 8.6 mm edge',
    [(1, 2.54, 5.5, 'VOUT'), (2, 0, 5.5, 'GND'), (3, -2.54, 5.5, 'VIN')],
    8.6, 13.6, 9.6, 14.6)

# ---- MAX3485 module: 4 pads, module stands 53 mm tall ----------------------
FPS['MAX3485_Module'] = build(
    'MAX3485_Module', 'MAX3485 auto-direction module, vertical. 1 VCC 2 TXD 3 RXD 4 GND. 53 mm tall',
    [(1, -3.81, 0, 'VCC'), (2, -1.27, 0, 'TXD'), (3, 1.27, 0, 'RXD'), (4, 3.81, 0, 'GND')],
    22.0, 4.0, 23.0, 5.0, pin1_x=-8.0)
#  1:VCC sits inside the body, left of pin 1. Its old place, 3.4 mm outside
#  the body, is where Q5 and R13 went in Rev B - the label printed over
#  R13's pad and came out as 'VBC'. Inside it is covered once the module is
#  plugged, which is fine: it is read before plugging, not after.

# ---- KF350-3.5 2-pole screw terminal, gangable ----------------------------
#  Pads match the 4Ucon 3.50 library part (2.6 mm pad, 1.3 mm drill) so the
#  two can sit in the same row. Body 7.00 = 2 x 3.50, courtyard 6.90 so a
#  ganged neighbour 7.00 mm away clears it by 0.10 mm.
FPS['TerminalBlock_KF350_1x02'] = build(
    'TerminalBlock_KF350_1x02',
    'KF350-3.5-2P screw terminal, 3.50 mm. Gangable: place two 7.00 mm apart and the four pads land on the 3.50 mm grid of a 4-pole part.',
    [(1, 0, 0, 'P1'), (2, 3.5, 0, 'P2')],
    7.0, 6.9, 6.9, 9.5, cx=1.75, pin1=False)
#  Silk 6.9 mm deep: the body depth in the vendor's own footprint data
#  (EasyEDA, C474892). It was 9.4 and ran into the GND/SDA/SCL pin names
#  under J5A/J5B. The courtyard stays 9.5 deep - that is the reserve.

for n, body in FPS.items():
    path = os.path.join(LIB, n + '.kicad_mod')
    io.open(path, 'w', encoding='utf-8', newline='\n').write(body)
    print('wrote', os.path.basename(path), len(body), 'bytes')
