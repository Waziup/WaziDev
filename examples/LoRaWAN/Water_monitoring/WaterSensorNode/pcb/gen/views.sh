#!/bin/sh
# The things you LOOK at: the DRC report, a quick view, and the 1:1 print.
#
# WHY THIS IS A SCRIPT. These three were made once, by hand, on 7 October, and
# then sat in out/ while the board moved on - D1 became Q5, three 4-pole
# terminals became six 2-pole ones, the silkscreen went to Rev C. The 1:1 PDF
# is the worst of them: the whole point of printing it is to lay the real parts
# on the paper, so a stale one quietly validates the wrong board. Anything the
# board can produce should be produced by the board.
set -e
cd "$(dirname "$0")/.."
K="/c/Program Files/KiCad/10.0/bin/kicad-cli.exe"
PCB=KijaniCarrier.kicad_pcb
VIEW="F.Cu,F.Silkscreen,Edge.Cuts"

mkdir -p out

"$K" pcb drc --severity-error -o out/drc.txt "$PCB" >/dev/null
"$K" pcb export svg --layers "$VIEW" --page-size-mode 2 --exclude-drawing-sheet \
     -o out/view.svg "$PCB" >/dev/null

# --scale 1 and no border: what comes out of the printer must be the board's
# own size. Check it against the mounting holes (3.2 mm) and the 100 mm edge
# before trusting a part that "almost" fits.
"$K" pcb export pdf --layers "$VIEW" --scale 1 --black-and-white \
     -o out/KijaniCarrier-top-1zu1.pdf "$PCB" >/dev/null

# One SVG per custom footprint. The directory had four while the library had
# five - the gangable KF350 terminal was missing, which is the same drift in
# miniature. Regenerating the whole library means the count cannot be wrong.
rm -rf out/footprints
mkdir -p out/footprints
"$K" fp export svg --layers "F.Cu,F.SilkS,F.CrtYd"      -o out/footprints KijaniSpace.pretty >/dev/null

python -I - <<'PY'
import io, os, re
# A 1:1 print is only worth printing if it really is 1:1.
pdf = 'out/KijaniCarrier-top-1zu1.pdf'
raw = open(pdf, 'rb').read()
m = re.search(rb'/MediaBox\s*\[([^\]]*)\]', raw)
assert m, 'kein MediaBox im PDF'
box = [float(v) for v in m.group(1).split()]
w_mm = (box[2] - box[0]) / 72.0 * 25.4
h_mm = (box[3] - box[1]) / 72.0 * 25.4
print('1:1-PDF  Seite %.1f x %.1f mm, Board 100 x 100 mm' % (w_mm, h_mm))
assert w_mm > 100 and h_mm > 100, 'Seite kleiner als das Board - der Druck waere skaliert'
for f in ('out/drc.txt', 'out/view.svg', pdf):
    assert os.path.getsize(f) > 500, 'leer: ' + f
    print('  %-34s %7d B' % (f, os.path.getsize(f)))

# every custom footprint gets a view, and no view outlives its footprint
import glob
lib = {os.path.basename(f)[:-10] for f in glob.glob('KijaniSpace.pretty/*.kicad_mod')}
svg = {os.path.basename(f)[:-4] for f in glob.glob('out/footprints/*.svg')}
assert lib == svg, 'Footprint-Ansichten weichen ab: %s' % sorted(lib ^ svg)
print('  out/footprints/                    %d Ansichten, eine je Footprint' % len(svg))
PY
