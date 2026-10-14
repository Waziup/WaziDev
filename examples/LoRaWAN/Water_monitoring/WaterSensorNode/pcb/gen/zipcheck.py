# -*- coding: utf-8 -*-
"""Check the archive that actually gets uploaded, not the KiCad source.

   Every check so far has read KijaniCarrier.kicad_pcb. That proves the design
   is right; it does not prove the export is. This reads the zip itself and
   compares the hole count and the outline against the board, so an export that
   silently dropped something cannot pass.
"""
from __future__ import print_function
import io, re, sys, zipfile

NL = chr(10)

sys.path.insert(0, 'gen')
from centroid import _blocks

ZIP = 'out/fab/KijaniCarrier-JLCPCB.zip'
z = zipfile.ZipFile(ZIP)
names = z.namelist()

# ---- holes: drill files against the board's THT pads and vias -------------
holes = 0
for n in [x for x in names if x.endswith('.drl')]:
    txt = z.read(n).decode('latin-1')
    body = txt.split('%')[-1] if '%' in txt else txt
    holes += len(re.findall(r'^X[-\d.]+Y[-\d.]+', body, re.M))

s = io.open('KijaniCarrier.kicad_pcb', encoding='utf-8').read()
tht = sum(1 for blk in _blocks(s, 'footprint')
          for p in _blocks(blk, 'pad') if '(drill ' in p)
vias = len(_blocks(s, 'via'))
print('Bohrungen im Zip      : %d' % holes)
print('  THT-Pads am Board   : %d' % tht)
print('  Vias am Board       : %d' % vias)
print('  erwartet            : %d' % (tht + vias))
assert holes == tht + vias, 'Bohrungen fehlen oder sind doppelt'

# ---- outline: read the Edge.Cuts gerber -----------------------------------
ed = z.read('KijaniCarrier-Edge_Cuts.gbr').decode('latin-1')
fs = re.search(r'%FSLAX(\d)(\d)Y\d\d\*%', ed)
dec = int(fs.group(2))
unit = 25.4 if '%MOIN*%' in ed else 1.0
# Skip the %...% header blocks - %FSLAX46Y46*% looks exactly like a
# coordinate pair to a naive regex and reads as X=46, Y=46.
body = NL.join(l for l in ed.split(NL) if not l.startswith('%'))
pts = re.findall(r'X(-?\d+)Y(-?\d+)', body)
assert pts, 'keine Koordinaten im Umriss'
xs = [int(a) / 10.0 ** dec * unit for a, b in pts]
ys = [int(b) / 10.0 ** dec * unit for a, b in pts]
w, h = max(xs) - min(xs), max(ys) - min(ys)
print()
print('Umriss im Gerber      : %.2f x %.2f mm' % (w, h))
assert abs(w - 100.0) < 0.05 and abs(h - 100.0) < 0.05, 'Umriss weicht ab'

# ---- the layers a fab needs, and no second drill set ----------------------
need = ['F_Cu', 'B_Cu', 'F_Mask', 'B_Mask', 'F_Silkscreen', 'B_Silkscreen',
        'F_Paste', 'B_Paste', 'Edge_Cuts']
missing = [n for n in need if 'KijaniCarrier-%s.gbr' % n not in names]
assert not missing, 'fehlende Lagen: %s' % missing
assert len([x for x in names if x.endswith('.drl')]) == 2, 'nicht genau ein Bohrsatz'
print('Lagen                 : %d Gerber + 2 Bohrdateien, nichts fehlt' % len(need))

# ---- nothing empty --------------------------------------------------------
tiny = [n for n in names if z.getinfo(n).file_size < 200]
print('Leere/winzige Dateien : %s' % (tiny or 'keine'))
assert not tiny, 'eine Lage ist praktisch leer'
print()
print('Zip ist in sich stimmig.')
