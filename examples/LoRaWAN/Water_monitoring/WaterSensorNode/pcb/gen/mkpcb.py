# -*- coding: utf-8 -*-
"""Assemble KijaniCarrier.kicad_pcb from the verified placement."""

from __future__ import print_function

import sys as _sys
_GENERATOR_GUARD = True
if __name__ != '__main__':
    raise ImportError('mkpcb.py rewrites the board file; run it, do not import it')
import io, os, re, math, hashlib

KI = r'C:\Program Files\KiCad\10.0\share\kicad\footprints'
OUT = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb'
MYLIB = os.path.join(OUT, 'KijaniSpace.pretty')
PCBVER = '20241229'

# KiCad Y grows downward; our plan has Y growing up. Mirror about the board.
BOARD = 100.0
ORG = 20.0          # keep the board away from the sheet edge

def Y(y):
    return round(ORG + (BOARD - y), 3)

def X(x):
    return round(ORG + x, 3)

def uid(seed):
    h = hashlib.md5(seed.encode('utf-8')).hexdigest()
    return '%s-%s-%s-%s-%s' % (h[0:8], h[8:12], h[12:16], h[16:20], h[20:32])

# ref, x, y, rot, footprint, value
C = [
    ('J2', 20, 93, 180, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x02_P3.50mm_Vertical', 'SW_1'),
    ('J3', 34, 93, 180, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x03_P3.50mm_Vertical', 'ANALOG'),
    ('J4', 50, 93, 180, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x03_P3.50mm_Vertical', 'DIGITAL'),
    ('J5', 68, 93, 180, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x04_P3.50mm_Vertical', 'I2C_PORT'),
    ('J6', 86, 93, 180, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x02_P3.50mm_Vertical', 'BAT_IN'),

    ('X1', 12, 80, 0, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x02_P3.50mm_Vertical', 'CELL'),
    ('F1', 25, 82, 0, 'Resistor_SMD:R_1812_4532Metric', 'PTC 2.6A'),
    ('D1', 32, 82, 0, 'Diode_SMD:D_SMA', 'SS34'),
    ('D2', 33, 70, 0, 'Diode_SMD:D_SMB', 'SMBJ5.0A'),
    ('C1', 22, 75, 0, 'Capacitor_THT:CP_Radial_D8.0mm_P3.50mm', '100uF'),
    ('Q1', 12, 66, 0, 'Package_TO_SOT_SMD:SOT-23', 'AO3401A'),
    ('Q2', 18, 66, 0, 'Package_TO_SOT_SMD:SOT-23', '2N7002'),
    ('C3', 24, 66, 0, 'Capacitor_SMD:C_0603_1608Metric', '100nF'),
    ('R1', 12, 62, 0, 'Resistor_SMD:R_0603_1608Metric', '100k'),
    ('R2', 18, 62, 0, 'Resistor_SMD:R_0603_1608Metric', '10k'),
    ('R3', 24, 62, 0, 'Resistor_SMD:R_0603_1608Metric', '100k'),
    ('U1', 14, 53, 0, 'KijaniSpace:Pololu_U3V9F12', 'U3V9F12'),
    ('C4', 28, 54, 0, 'Capacitor_THT:CP_Radial_D10.0mm_P5.00mm', '470uF'),
    ('Q3', 12, 44, 0, 'Package_TO_SOT_SMD:SOT-23', 'AO3401A'),
    ('Q4', 18, 44, 0, 'Package_TO_SOT_SMD:SOT-23', '2N7002'),
    ('C5', 24, 44, 0, 'Capacitor_SMD:C_0603_1608Metric', '100nF'),
    ('R4', 12, 40, 0, 'Resistor_SMD:R_0603_1608Metric', '100k'),
    ('R5', 18, 40, 0, 'Resistor_SMD:R_0603_1608Metric', '10k'),
    ('R6', 24, 40, 0, 'Resistor_SMD:R_0603_1608Metric', '100k'),
    ('U2', 12, 32, 0, 'Package_TO_SOT_SMD:SOT-23', 'MCP1700-3302'),
    ('C6', 18, 32, 0, 'Capacitor_SMD:C_0603_1608Metric', '1uF'),
    ('C7', 24, 32, 0, 'Capacitor_SMD:C_0603_1608Metric', '1uF'),
    ('JP1', 12, 28, 0, 'Resistor_SMD:R_0603_1608Metric', '0R DNP'),

    ('U3', 48, 79, 0, 'KijaniSpace:MAX3485_Module', 'MAX3485'),
    ('J8', 48, 69, 0, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x02_P3.50mm_Vertical', 'A/B'),
    ('RT1', 40, 62, 0, 'Resistor_SMD:R_0603_1608Metric', '120R'),
    ('R7', 45, 62, 0, 'Resistor_SMD:R_0603_1608Metric', '680R'),
    ('R8', 50, 62, 0, 'Resistor_SMD:R_0603_1608Metric', '680R'),
    ('C8', 44, 54, 0, 'Capacitor_THT:CP_Radial_D10.0mm_P5.00mm', '470uF'),
    ('C9', 40, 46, 0, 'Capacitor_SMD:C_0805_2012Metric', '10uF'),
    ('C10', 45, 46, 0, 'Capacitor_SMD:C_0603_1608Metric', '100nF'),
    ('R11', 64, 70, 180, 'Resistor_SMD:R_0603_1608Metric', '47k DNP'),
    ('R12', 64, 74, 180, 'Resistor_SMD:R_0603_1608Metric', '47k DNP'),

    ('U4', 64, 50, 270, 'KijaniSpace:BE_IVI_Isolator', 'BE-IVI pH'),
    ('U6', 64, 24, 270, 'KijaniSpace:EZO_Circuit', 'EZO-pH'),
    ('U5', 86, 50, 270, 'KijaniSpace:BE_IVI_Isolator', 'BE-IVI EC'),
    ('U7', 86, 24, 270, 'KijaniSpace:EZO_Circuit', 'EZO-EC'),

    ('X2', 9, 7, 0, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x03_P3.50mm_Vertical', 'DS18B20'),
    ('X3', 26, 7, 0, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x04_P3.50mm_Vertical', 'PROBE1'),
    ('X4', 45, 7, 0, 'TerminalBlock_4Ucon:TerminalBlock_4Ucon_1x04_P3.50mm_Vertical', 'PROBE2'),
    ('J7', 64, 7, 0, 'Connector_Coaxial:SMA_Amphenol_132291_Vertical', 'SMA pH'),
    ('J9', 86, 7, 0, 'Connector_Coaxial:SMA_Amphenol_132291_Vertical', 'SMA EC'),
    ('R9', 20, 20, 0, 'Resistor_SMD:R_0603_1608Metric', '4k7'),

    ('H1', 5, 18, 0, 'MountingHole:MountingHole_3.2mm_M3', 'M3'),
    ('H2', 95, 8, 0, 'MountingHole:MountingHole_3.2mm_M3', 'M3'),
    ('H3', 5, 95, 0, 'MountingHole:MountingHole_3.2mm_M3', 'M3'),
    ('H4', 95, 95, 0, 'MountingHole:MountingHole_3.2mm_M3', 'M3'),
]

def load_fp(libref):
    lib, name = libref.split(':')
    if lib == 'KijaniSpace':
        path = os.path.join(MYLIB, name + '.kicad_mod')
    else:
        path = os.path.join(KI, lib + '.pretty', name + '.kicad_mod')
    return io.open(path, encoding='utf-8').read()

# Reference designators that land on a neighbour's silkscreen, on a pad, or
# in the isolation slot. Absolute KiCad mm; the DRC report is the arbiter.
REFPOS = {
    # the WaziSense connectors: the pin names go under them, so the
    # designator moves to the left-hand end, read sideways
    'J2': (33.0, 27.0, 90), 'J3': (43.5, 27.0, 90), 'J4': (59.5, 27.0, 90),
    'J5': (73.8, 27.0, 90), 'J6': (99.5, 27.0, 90),
    'J7': (79.8, 113.0, 90), 'J9': (112.5, 113.0, 90),
    'Q3': (27.0, 74.0),
    'Q4': (41.2, 68.5),
    'U1': (40.0, 62.0),
    'C1': (43.75, 51.0),
    'U3': (62.0, 36.0),
    'U4': (84.0, 56.0),
    'U5': (106.0, 56.0),
}


def instance(ref, x, y, rot, libref, value):
    src = load_fp(libref)
    # strip the stand-alone library header lines
    src = re.sub(r'\n\t\(version \d+\)', '', src, count=1)
    src = re.sub(r'\n\t\(generator "[^"]*"\)', '', src, count=1)
    src = re.sub(r'\n\t\(generator_version "[^"]*"\)', '', src, count=1)
    # name it, position it
    src = re.sub(r'^\(footprint "[^"]*"', '(footprint "%s"' % libref, src, count=1)
    src = src.replace('\t(layer "F.Cu")',
                      '\t(layer "F.Cu")\n\t(uuid "%s")\n\t(at %s %s %s)' % (uid(ref + 'fp'), X(x), Y(y), rot), 1)
    src = re.sub(r'\(property "Reference" "[^"]*"', '(property "Reference" "%s"' % ref, src, count=1)
    if ref in REFPOS:
        v = REFPOS[ref]
        rx, ry, trot = (v + (0,))[:3]
        i = src.index('(property "Reference"')
        j = src.index('(at ', i)
        k = src.index(')', j)
        # KiCad stores the text offset in the footprint's own frame and then
        # applies the footprint rotation to it, so the offset has to be
        # rotated back out first.
        th = math.radians(float(rot))
        c, si = math.cos(th), math.sin(th)
        dx, dy = rx - X(x), ry - Y(y)
        src = src[:j] + '(at %s %s %s) (unlocked yes)' % (
            round(dx * c - dy * si, 3), round(dx * si + dy * c, 3), trot) + src[k + 1:]
    src = re.sub(r'\(property "Value" "[^"]*"', '(property "Value" "%s"' % value, src, count=1)
    # every uuid in the instance must be unique
    n = [0]

    def fresh(m):
        n[0] += 1
        return '(uuid "%s")' % uid('%s-%d' % (ref, n[0]))

    head, sep, tail = src.partition('(at %s %s %s)' % (X(x), Y(y), rot))
    tail = re.sub(r'\(uuid "[^"]*"\)', fresh, tail)
    return head + sep + tail

LAYERS = '''\t(layers
\t\t(0 "F.Cu" signal)
\t\t(2 "B.Cu" signal)
\t\t(9 "F.Adhes" user "F.Adhesive")
\t\t(11 "B.Adhes" user "B.Adhesive")
\t\t(13 "F.Paste" user)
\t\t(15 "B.Paste" user)
\t\t(5 "F.SilkS" user "F.Silkscreen")
\t\t(7 "B.SilkS" user "B.Silkscreen")
\t\t(1 "F.Mask" user)
\t\t(3 "B.Mask" user)
\t\t(17 "Dwgs.User" user "User.Drawings")
\t\t(19 "Cmts.User" user "User.Comments")
\t\t(21 "Eco1.User" user "User.Eco1")
\t\t(23 "Eco2.User" user "User.Eco2")
\t\t(25 "Edge.Cuts" user)
\t\t(27 "Margin" user)
\t\t(31 "F.CrtYd" user "F.Courtyard")
\t\t(29 "B.CrtYd" user "B.Courtyard")
\t\t(35 "F.Fab" user)
\t\t(33 "B.Fab" user)
\t)
'''

def edge_rect(x1, y1, x2, y2, seed):
    pts = [(x1, y1, x2, y1), (x2, y1, x2, y2), (x2, y2, x1, y2), (x1, y2, x1, y1)]
    s = ''
    for i, (ax, ay, bx, by) in enumerate(pts):
        s += ('\t(gr_line\n\t\t(start %s %s)\n\t\t(end %s %s)\n'
              '\t\t(stroke (width 0.1) (type default))\n\t\t(layer "Edge.Cuts")\n\t\t(uuid "%s")\n\t)\n'
              ) % (ax, ay, bx, by, uid(seed + str(i)))
    return s

body = '(kicad_pcb\n\t(version %s)\n\t(generator "kijani-gen")\n\t(generator_version "9.0")\n' % PCBVER
body += '\t(general\n\t\t(thickness 1.6)\n\t\t(legacy_teardrops no)\n\t)\n\t(paper "A4")\n'
body += LAYERS
body += '\t(setup\n\t\t(pad_to_mask_clearance 0)\n\t\t(allow_soldermask_bridges_in_footprints no)\n\t)\n'
body += '\t(net 0 "")\n'

# board outline and the isolation slot
body += edge_rect(X(0), Y(100), X(100), Y(0), 'outline')
body += edge_rect(74.0, 68.8, 94.0, 71.2, 'slot_ph')
body += edge_rect(96.0, 68.8, 116.0, 71.2, 'slot_ec')

for ref, x, y, rot, fp, val in C:
    body += instance(ref, x, y, rot, fp, val)

body += ')\n'

if not os.path.isdir(OUT):
    os.makedirs(OUT)
io.open(os.path.join(OUT, 'KijaniCarrier.kicad_pcb'), 'w', encoding='utf-8', newline='\n').write(body)
print('wrote KijaniCarrier.kicad_pcb  %d bytes, %d components' % (len(body), len(C)))

PRO = '''{
  "board": {"design_settings": {}},
  "libraries": {"pinned_footprint_libs": [], "pinned_symbol_libs": []},
  "meta": {"filename": "KijaniCarrier.kicad_pro", "version": 3},
  "sheets": [],
  "text_variables": {}
}
'''
io.open(os.path.join(OUT, 'KijaniCarrier.kicad_pro'), 'w', encoding='utf-8', newline='\n').write(PRO)
print('wrote KijaniCarrier.kicad_pro')
