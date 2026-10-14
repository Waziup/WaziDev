#!/bin/sh
# Fabrication outputs, start to finish.
#
# This was done by hand once and the hand-made zip ended up carrying a second,
# stale drill file, which a board house reads as two different boards. Doing it
# from a script, into a directory that is emptied first, means that cannot come
# back. The zip is built by python, not the zip(1) binary - Git Bash on Windows
# does not ship one.
set -e
cd "$(dirname "$0")/.."
K="/c/Program Files/KiCad/10.0/bin/kicad-cli.exe"
PCB=KijaniCarrier.kicad_pcb
OUT=out/fab

rm -rf "$OUT"
mkdir -p "$OUT"

# JLCPCB reads the layer from the file name suffix, so keep KiCad's own names.
"$K" pcb export gerbers --no-protel-ext -o "$OUT" "$PCB" >/dev/null
"$K" pcb export drill --format excellon --drill-origin absolute \
     --excellon-units mm --excellon-separate-th -o "$OUT" "$PCB" >/dev/null
"$K" pcb export pos --format csv --units mm --side both \
     -o "$OUT/KijaniCarrier-cpl.csv" "$PCB" >/dev/null

python gen/jlc.py

# Lead pitch and hole of every THT footprint against the vendor's own.
# Cheap, and it is the only check that survives having no part in hand.
python -I gen/vendorcheck.py

# The two BOMs are derived from one another; prove they still agree.
python -I gen/bomsync.py

python -I gen/fabzip.py "$OUT"
python gen/fabcheck.py
# Last gate: read the archive that actually gets uploaded, not the source.
python -I gen/zipcheck.py
