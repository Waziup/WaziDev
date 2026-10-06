# -*- coding: utf-8 -*-
"""Everything printed on the board, written for someone who has not seen the
   schematic.

   The footprints' Value fields sit on F.Fab, which is documentation and is not
   printed, so without this the finished board would carry nothing but
   reference designators. Every terminal pin gets its name, the cell gets its
   polarity, the isolation slots get a warning, and the free area right of the
   WaziSense connectors carries a legend.

   Pin labels run vertically so three characters fit between pads on a 3.5 mm
   pitch at a height the board house can still print; 0.8 mm height and 0.15 mm
   stroke are JLCPCB's minimums.
"""
from __future__ import print_function

import sys as _sys
_GENERATOR_GUARD = True
if __name__ != '__main__':
    raise ImportError('silk.py rewrites the board file; run it, do not import it')

import io, hashlib

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'

H_PIN, H_NOTE, H_NAME = 0.8, 0.9, 1.1
W = 0.15
TEXTS = []


def t(txt, x, y, rot=0, h=H_PIN):
    TEXTS.append((txt, x, y, rot, h))


def pins(labels, y, rot=90, h=H_PIN):
    for x, s in labels:
        t(s, x, y, rot, h)


# ---- to the WaziSense: pin names in the band below the connectors ---------
t('ZUM WAZISENSE', 70.0, 20.9, 0, H_NOTE)
pins([(36.5, 'D6-'), (40.0, 'D6+')], 32.6)
pins([(47.0, 'GND'), (50.5, 'A1'), (54.0, 'A2')], 32.6)
pins([(63.0, 'GND'), (66.5, 'D3'), (70.0, 'D4')], 32.6)
pins([(77.5, 'GND'), (81.0, 'D5'), (84.5, 'SDA'), (88.0, 'SCL')], 32.6)
pins([(102.5, 'GND'), (106.0, '+BAT')], 32.9)

# ---- the cell, labelled below its body ------------------------------------
t('+', 32.0, 46.3, 0, H_NAME)
t('-', 35.5, 46.3, 0, H_NAME)
t('ZELLE 18650', 33.75, 48.2, 0, H_NOTE)

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
t('MAX3485 A/B', 69.75, 44.6, 0, H_NOTE)
t('A', 68.0, 46.6, 0, H_PIN)
t('B', 71.5, 46.6, 0, H_PIN)

# ---- polarity on the electrolytics ----------------------------------------
t('+', 42.0, 42.3, 0, H_NAME)      # C1
t('+', 48.0, 62.3, 0, H_NAME)      # C4
t('+', 64.0, 62.3, 0, H_NAME)      # C8

# ---- the isolation barrier, on the land bridge between the two slots ------
t('ISOLIERT', 95.0, 70.0, 90, H_NOTE)
# the reference electrode warning belongs where the electrodes are
t('PGND NIE AN GND', 95.0, 112.0, 90, H_NOTE)

# ---- legend, in the free area right of the WaziSense connectors -----------
LEGEND = [
    'KIJANISPACE CARRIER Rev A',
    '',
    'J2 SW_1     J3 ANALOG',
    'J4 DIGITAL  J5 I2C',
    'J6 BAT_IN',
    '',
    'A1 = ISOLATOREN EIN',
    'D5 = 12V EIN',
    'D6 = 3V3 EIN',
    '',
    'JP1 BESTUECKEN, U2 NICHT',
    'R11 R12 NICHT BESTUECKEN',
    '',
    'MODULE: BAUTEILSEITE OBEN',
    'PIN 1 = ECKIGES PAD',
]
y = 35.6
for line in LEGEND:
    if line:
        t(line, 93.5, y, 0, H_NOTE)
        y += 1.55
    else:
        y += 0.8          # a blank line is only half a step, to stay clear of U5


def uid(seed):
    h = hashlib.md5(seed.encode('utf-8')).hexdigest()
    return '%s-%s-%s-%s-%s' % (h[0:8], h[8:12], h[12:16], h[16:20], h[20:32])


s = io.open(PCB, encoding='utf-8').read()
out = ''
for i, (txt, x, y, rot, h) in enumerate(TEXTS):
    just = '\t\t\t(justify left)\n' if x == 93.5 else ''
    out += ('\t(gr_text "%s"\n\t\t(at %s %s %s)\n\t\t(layer "F.SilkS")\n\t\t(uuid "%s")\n'
            '\t\t(effects\n\t\t\t(font\n\t\t\t\t(size %s %s)\n\t\t\t\t(thickness %s)\n\t\t\t)\n'
            '%s\t\t)\n\t)\n'
            ) % (txt, x, y, rot, uid('silk-%d-%s' % (i, txt)), h, h, W, just)

s = s.rstrip()[:-1].rstrip() + '\n' + out + ')\n'
io.open(PCB, 'w', encoding='utf-8', newline='\n').write(s)
print('%d Siebdruck-Texte geschrieben' % len(TEXTS))
