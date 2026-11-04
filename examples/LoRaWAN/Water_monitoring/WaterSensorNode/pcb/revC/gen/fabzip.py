# -*- coding: utf-8 -*-
"""Pack the files a board house needs, and nothing else.

   Only the layers that become copper, mask, silkscreen, paste, outline and
   holes go in. F.Courtyard and F.Fab are drawings for the designer; they
   carried 190 kB of noise into the old archive and mean nothing to a fab.

   The one thing worth asserting is the drill set: exactly two files, PTH and
   NPTH, and no third. A leftover KijaniCarrier.drl from an earlier run once
   sat beside them, and a board house reading two complete drill sets reads
   two different boards.
"""
from __future__ import print_function
import os, sys, zipfile

out = sys.argv[1]
LAYERS = ['F_Cu', 'B_Cu', 'F_Mask', 'B_Mask',
          'F_Silkscreen', 'B_Silkscreen', 'F_Paste', 'B_Paste', 'Edge_Cuts']
files = ['KijaniCarrier-%s.gbr' % n for n in LAYERS]
drl = ['KijaniCarrier-PTH.drl', 'KijaniCarrier-NPTH.drl']

stray = [n for n in os.listdir(out) if n.endswith('.drl') and n not in drl]
assert not stray, 'a stray drill file is in the output: %s' % stray
files += drl

zp = os.path.join(out, 'KijaniCarrier-JLCPCB.zip')
with zipfile.ZipFile(zp, 'w', zipfile.ZIP_DEFLATED) as z:
    for n in files:
        src = os.path.join(out, n)
        assert os.path.exists(src), 'missing ' + n
        z.write(src, n)

with zipfile.ZipFile(zp) as z:
    got = z.namelist()
assert sorted(got) == sorted(files), got
print('KijaniCarrier-JLCPCB.zip: %d files, one drill set, %d kB'
      % (len(got), os.path.getsize(zp) // 1024))
