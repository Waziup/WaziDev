# -*- coding: utf-8 -*-
"""Where the centre of each part's body actually is, in board coordinates.

   WHY THIS EXISTS. A placement file's "Mid X / Mid Y" is the centre of the
   component, and kicad-cli writes the footprint ORIGIN instead. For every SMT
   footprint the two are the same, so the error is invisible - and for the
   through-hole parts they are not. A KiCad terminal block and a radial
   electrolytic both put their origin on pad 1:

       X2, J3, J4        3.50 mm out    3-pole screw terminals
       C4, C8            2.50 mm out    470 uF, 5.00 mm lead pitch
       X1, J2, J6, J8    1.75 mm out    2-pole screw terminals
       X3A..X4B, J5A/B   1.75 mm out    the ganged 2-pole pairs
       C1                1.00 mm out    100 uF, 2.00 mm lead pitch

   An assembler fed those numbers puts sixteen parts up to 3.5 mm off their
   holes. jlc.py already did this arithmetic by hand for the U4..U7 socket
   rows, which is how the problem came to light: the same correction was
   needed everywhere and was being applied in one place only.

   The centre is read off the F.Courtyard rectangle, not off the pads. A pad
   centroid is right for a terminal block and wrong for a SOT-23, where two
   pads face one and the average lands 0.37 mm off the body.
"""
from __future__ import print_function
import io, math, os, re

PCB = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   '..', 'KijaniCarrier.kicad_pcb')


def _blocks(txt, tag):
    """Balanced (tag ...) blocks. The tokens are followed by a newline as often
       as by a space, so match the tag plus any whitespace."""
    out = []
    for m in re.finditer(r'\(' + tag + r'\s', txt):
        d, j = 0, m.start()
        while j < len(txt):
            if txt[j] == '(':
                d += 1
            elif txt[j] == ')':
                d -= 1
                if d == 0:
                    break
            j += 1
        out.append(txt[m.start():j + 1])
    return out


FOOTPRINT = {}

#  Footprints drawn as the whole module but fitted with a socket row only.
SOCKET_ONLY = ('Pololu_U3V9F12', 'MAX3485_Module')

#  U4..U7 are one footprint per module, but each takes TWO socket strips,
#  and an assembly line places one part per designator. These name the two
#  rows by their first and last pad. They exist only in the assembly files;
#  the board is untouched, so nothing in the netlist can shift.
ROWS = {
    'U4A': ('U4', '1', '5'), 'U4B': ('U4', '6', '10'),
    'U5A': ('U5', '1', '5'), 'U5B': ('U5', '6', '10'),
    'U6A': ('U6', '1', '3'), 'U6B': ('U6', '4', '6'),
    'U7A': ('U7', '1', '3'), 'U7B': ('U7', '4', '6'),
}


def centres(path=PCB):
    s = io.open(path, encoding='utf-8').read()
    out = {}
    for blk in _blocks(s, 'footprint'):
        ref = re.search(r'\(property "Reference" "([^"]+)"', blk)
        fpname = re.search(r'\(footprint "([^"]+)"', blk).group(1).split(':')[-1]
        if ref:
            FOOTPRINT[ref.group(1)] = fpname
        at = re.search(r'\n\t\t\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)', blk)
        if not ref or not at:
            continue
        ox, oy, rot = float(at.group(1)), float(at.group(2)), float(at.group(3) or 0)
        xs, ys = [], []
        for tag in ('fp_line', 'fp_rect', 'fp_circle'):
            for g in _blocks(blk, tag):
                if '"F.CrtYd"' not in g:
                    continue
                pts = [(float(p), float(q)) for p, q in re.findall(
                    r'\((?:start|end|center) ([-\d.]+) ([-\d.]+)\)', g)]
                if tag == 'fp_circle':
                    (cx, cy), (ex, ey) = pts
                    r = math.hypot(ex - cx, ey - cy)
                    pts = [(cx - r, cy - r), (cx + r, cy + r)]
                xs += [p[0] for p in pts]
                ys += [p[1] for p in pts]
        if not xs:
            continue                      # no courtyard: leave it to the origin
        if 'TerminalBlock' in fpname or fpname in SOCKET_ONLY:
            # The board's 3.5 mm terminals are KF350s drawn on 4Ucon
            # footprints, whose courtyard is 0.9 mm deeper on one side than
            # the other. The KF350 body is centred on its pad row (vendor
            # footprint data), so the pad centroid is the true body centre.
            # The courtyard centre put JLCPCB's model 0.455 mm off the pads.
            # A module footprint whose courtyard is the whole plug-in board
            # gets only a pin socket from the assembler, and that socket sits
            # on the pads too: U1's courtyard centre put it 5.5 mm off its
            # holes, seen in JLCPCB's placement render of 16 Oct.
            pp = [tuple(map(float, re.search(r'\(at ([-\d.]+) ([-\d.]+)', p).groups()))
                  for p in _blocks(blk, 'pad') if '(drill ' in p]
            xs = [p[0] for p in pp]
            ys = [p[1] for p in pp]
        lx = (min(xs) + max(xs)) / 2.0
        ly = (min(ys) + max(ys)) / 2.0
        #  KiCad's (at x y r) turns a footprint counter-clockwise by r with the
        #  y axis pointing down, so local (x, y) lands at
        #  (x cos r + y sin r, -x sin r + y cos r). This line once said -rot,
        #  which is the same thing at 0 and 180 degrees and the mirror image
        #  at +-90. Nothing off-centre sits at +-90 on this board, so the
        #  CPL never moved - but the silkscreen check built on it put U4/U5's
        #  labels 27 mm from where they are printed, and passed a collision
        #  the JLCPCB render showed at a glance.
        a = math.radians(rot)
        out[ref.group(1)] = (ox + lx * math.cos(a) + ly * math.sin(a),
                             oy - lx * math.sin(a) + ly * math.cos(a))
    return out


CENTRE = centres()

if __name__ == '__main__':
    import csv, sys
    pos = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       '..', 'out', 'fab', 'KijaniCarrier-cpl.csv')
    moved = []
    for row in csv.DictReader(io.open(pos, encoding='utf-8')):
        c = CENTRE.get(row['Ref'])
        if not c:
            continue
        d = math.hypot(c[0] - float(row['PosX']), c[1] + float(row['PosY']))
        if d > 0.005:
            moved.append((d, row['Ref']))
    moved.sort(reverse=True)
    for d, ref in moved:
        print('  %-5s %5.2f mm' % (ref, d))
    print('%d parts corrected, %d already on centre'
          % (len(moved), len(CENTRE) - len(moved)))
