# -*- coding: utf-8 -*-
"""Silkscreen collisions, checked at every severity.

   WHY. Every DRC run in this project used --severity-error, and KiCad files
   silkscreen overlaps under WARNINGS. So the Rev C board passed "0 violations"
   while the legend printed through U5's reference, three designators turned
   U3's 1:VCC label into 'VBC', and pin names ran into terminal outlines. The
   JLCPCB placement render showed it at a glance; the checks did not.

   Two kinds of finding are expected and allowed:
     - outline against outline inside a ganged terminal pair (J5A/J5B,
       X3A/X3B, X4A/X4B): those blocks are meant to touch;
     - lib_footprint_mismatch: library configuration, see README.
   Anything else on the silkscreen stops the build.
"""
from __future__ import print_function
import io, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(HERE, '..', 'KijaniCarrier.kicad_pcb')
# run with python -I, so paths.py cannot be imported; same defaults
KICAD = os.environ.get('KICAD_CLI', os.path.join(
    os.environ.get('KICAD_DIR', r'C:\Program Files\KiCad\10.0'), 'bin', 'kicad-cli.exe'))
GANGED = ('J5', 'X3', 'X4')

rpt = os.path.join(tempfile.gettempdir(), 'kijani_drc_all.rpt')
subprocess.call([KICAD, 'pcb', 'drc', '--severity-all', '-o', rpt, PCB],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
t = io.open(rpt, encoding='utf-8', errors='replace').read()

allowed, bad = 0, []
for b in re.split(r'\n(?=\[)', t):
    m = re.match(r'\[(\w+)\]', b)
    if not m or m.group(1) not in ('silk_overlap', 'silk_over_copper'):
        continue
    names = [n for x, y, n in re.findall(r"@\(([\d.]+) mm, ([\d.]+) mm\): (.+)", b)]
    segs = [re.search(r'Segment of (\w+)', n) for n in names]
    if (len(segs) == 2 and all(segs)
            and segs[0].group(1)[:-1] == segs[1].group(1)[:-1]
            and segs[0].group(1)[:-1] in GANGED):
        allowed += 1
        continue
    bad.append('%s: %s' % (m.group(1), ' | '.join(
        n.replace(' on F.Silkscreen', '').replace(' on F.Cu', '') for n in names)))

print('Silkscreen: %d gewollte Beruehrungen in gangbaren Klemmenpaaren' % allowed)
if bad:
    for x in bad:
        print('  KOLLISION  ' + x)
    sys.exit(1)
print('Silkscreen: keine Kollision, keine Beschriftung auf Kupfer.')
