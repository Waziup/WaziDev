# -*- coding: utf-8 -*-
"""Everything printed on the board, written for someone who has not seen the
   schematic, and in English throughout.

   The footprints' Value fields sit on F.Fab, which is documentation and is not
   printed, so without this the finished board would carry nothing but
   reference designators. Every terminal pin gets its name, the cell gets its
   polarity, the isolation slots get a warning, and the free area right of the
   WaziSense connectors carries the Waziup logo and a legend.

   Pin labels run vertically so three characters fit between pads on a 3.5 mm
   pitch at a height the board house can still print; 0.8 mm height and 0.15 mm
   stroke are JLCPCB's minimums.
"""
from __future__ import print_function

import sys as _sys
_GENERATOR_GUARD = True
if __name__ != '__main__':
    raise ImportError('silk.py rewrites the board file; run it, do not import it')

import io, re, hashlib
import svg2silk as SVG

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
LOGO = 'waziup.svg'

H_PIN, H_NOTE, H_NAME = 0.8, 0.9, 1.1
W = 0.15
TEXTS = []


def t(txt, x, y, rot=0, h=H_PIN):
    TEXTS.append((txt, x, y, rot, h))


def pins(labels, y, rot=90, h=H_PIN):
    for x, s in labels:
        t(s, x, y, rot, h)


# ---- to the WaziSense: pin names in the band below the connectors ---------
# the WaziSense terminal each connector mates with, over the connector itself
t('SW_1', 38.25, 20.9, 0, H_NOTE)
t('ANALOG', 50.5, 20.9, 0, H_NOTE)
t('DIGITAL', 66.5, 20.9, 0, H_NOTE)
t('I2C_PORT', 82.75, 20.9, 0, H_NOTE)
t('POWER_OUT', 104.25, 20.9, 0, H_NOTE)
t('TO WAZISENSE', 95.5, 26.0, 90, H_NOTE)
pins([(36.5, 'D6-'), (40.0, 'D6+')], 32.6)
pins([(47.0, 'GND'), (50.5, 'A1'), (54.0, 'A2')], 32.6)
pins([(63.0, 'GND'), (66.5, 'D3'), (70.0, 'D4')], 32.6)
pins([(77.5, 'GND'), (81.0, 'D5'), (84.5, 'SDA'), (88.0, 'SCL')], 32.6)
pins([(102.5, 'GND'), (106.0, '+BAT')], 32.9)

# ---- the cell, labelled below its body ------------------------------------
t('+', 32.0, 46.3, 0, H_NAME)
t('-', 35.5, 46.3, 0, H_NAME)
t('18650 CELL IN', 33.75, 48.2, 0, H_NOTE)

# ---- sensors along the bottom edge ----------------------------------------
t('DS18B20', 32.5, 105.0, 0, H_NOTE)
pins([(29.0, '3V3'), (32.5, 'DQ'), (36.0, 'GND')], 102.0)
t('PROBE 1', 51.25, 105.0, 0, H_NOTE)
pins([(46.0, '+12V'), (49.5, 'GND'), (53.0, 'A'), (56.5, 'B')], 102.0)
t('PROBE 2', 70.25, 105.0, 0, H_NOTE)
pins([(65.0, '+12V'), (68.5, 'GND'), (72.0, 'A'), (75.5, 'B')], 102.0)
t('pH', 84.0, 107.6, 0, H_NAME)
t('EC', 106.0, 107.6, 0, H_NAME)

# ---- the RS485 link up from the transceiver's own screw terminal ----------
t('RS485 MODULE A/B', 69.75, 44.6, 0, H_NOTE)
t('A', 68.0, 46.6, 0, H_PIN)
t('B', 71.5, 46.6, 0, H_PIN)

# ---- polarity on the electrolytics ----------------------------------------
t('+', 42.0, 42.3, 0, H_NAME)      # C1
t('+', 48.0, 62.3, 0, H_NAME)      # C4
t('+', 64.0, 62.3, 0, H_NAME)      # C8

# ---- the socket rows, named beside each one -------------------------------
# The board has one footprint per module, but each takes two socket strips, so
# the assembly files name the rows U4A..U7B. Printing those names here means
# the assembler can find them without reading the CPL alone.
t('U4A', 93.0, 61.11, 0, H_PIN)
t('U4B', 93.0, 78.89, 0, H_PIN)
t('U5A', 115.5, 61.11, 0, H_PIN)
t('U5B', 115.5, 78.89, 0, H_PIN)
t('U6A', 93.0, 87.11, 0, H_PIN)
t('U6B', 93.0, 104.89, 0, H_PIN)
t('U7A', 115.5, 87.11, 0, H_PIN)
t('U7B', 115.5, 104.89, 0, H_PIN)

# ---- the isolation barrier ------------------------------------------------
t('ISOLATED', 95.0, 70.0, 90, H_NOTE)
t('PGND NEVER TO GND', 95.0, 112.0, 90, H_NOTE)

# ---- legend, in the free area right of the WaziSense connectors -----------
LEGEND = [
    'KIJANISPACE CARRIER Rev A',
    '',
    'A1=ISOLATORS D5=12V D6=3V3',
    'X1 = 18650 CELL IN',
    'J6 = POWER OUT TO WAZISENSE',
    '',
    'D6- IS NOT GND - KEEP APART',
    '',
    'LEAVE OPEN, DO NOT BRIDGE:',
    'R11 R12 U2 (JP1 REPLACES U2)',
    '',
    'MODULES: PIN1 = SQUARE PAD',
]
y = 39.6
for line in LEGEND:
    if line:
        t(line, 93.5, y, 0, H_NOTE)
        y += 1.45
    else:
        y += 0.75


def uid(seed):
    h = hashlib.md5(seed.encode('utf-8')).hexdigest()
    return '%s-%s-%s-%s-%s' % (h[0:8], h[8:12], h[12:16], h[16:20], h[20:32])


def logo(path, x0, y0, width_mm, keep=0.35):
    """Filled polygons from an SVG. A glyph counter - the gap in an A or a P -
       is spliced into its outer contour, because gr_poly has no holes. keep is
       the minimum step in SVG units, so curve flattening does not put
       thousands of points into the Gerber."""
    src = io.open(path, encoding='utf-8').read()
    vb = re.search(r'viewBox="([-0-9.]+) ([-0-9.]+) ([-0-9.]+) ([-0-9.]+)"', src)
    vw, vh = float(vb.group(3)), float(vb.group(4))
    s = width_mm / vw
    out, n, pts_total = '', 0, 0
    for m in re.finditer(r'<path[^>]*\sd="([^"]*)"', src):
        cs = SVG.parse(m.group(1))
        if not cs:
            continue
        outer = [c for c in cs if not any(SVG.inside(c[0], o) for o in cs if o is not c)]
        holes = [c for c in cs if not any(c is o for o in outer)]
        for o in outer:
            poly = o
            for h in list(holes):
                if SVG.inside(h[0], o):
                    poly = SVG.splice(poly, h)
                    holes.remove(h)
            thin = [poly[0]]
            for p in poly[1:]:
                if (p[0] - thin[-1][0]) ** 2 + (p[1] - thin[-1][1]) ** 2 >= keep * keep:
                    thin.append(p)
            if len(thin) < 3:
                continue
            pts_total += len(thin)
            body = ' '.join('(xy %.3f %.3f)' % (x0 + p[0] * s, y0 + p[1] * s) for p in thin)
            out += ('\t(gr_poly\n\t\t(pts %s)\n\t\t(stroke (width 0.1) (type solid))\n'
                    '\t\t(fill solid)\n\t\t(layer "F.SilkS")\n\t\t(uuid "%s")\n\t)\n'
                    ) % (body, uid('logo-%d' % n))
            n += 1
    print('  logo: %d polygons, %d points, %.1f x %.1f mm'
          % (n, pts_total, width_mm, vh * s))
    return out


s = io.open(PCB, encoding='utf-8').read()
out = logo(LOGO, 93.5, 33.4, 20.0)
for i, (txt, x, y, rot, h) in enumerate(TEXTS):
    just = '\t\t\t(justify left)\n' if x == 93.5 else ''
    out += ('\t(gr_text "%s"\n\t\t(at %s %s %s)\n\t\t(layer "F.SilkS")\n\t\t(uuid "%s")\n'
            '\t\t(effects\n\t\t\t(font\n\t\t\t\t(size %s %s)\n\t\t\t\t(thickness %s)\n\t\t\t)\n'
            '%s\t\t)\n\t)\n'
            ) % (txt, x, y, rot, uid('silk-%d-%s' % (i, txt)), h, h, W, just)

s = s.rstrip()[:-1].rstrip() + '\n' + out + ')\n'
io.open(PCB, 'w', encoding='utf-8', newline='\n').write(s)
print('%d silkscreen texts written' % len(TEXTS))
