#!/bin/sh
set -e
cd "$(dirname "$0")"
python mkfp.py >/dev/null
python mkpcb.py >/dev/null
python nets.py
python route.py
python zonecon.py
python silk.py
python check.py
python pour.py
# KiCad's own Python, for pcbnew. Override with KICAD_PYTHON.
"${KICAD_PYTHON:-/c/Program Files/KiCad/10.0/bin/python.exe}" fillboard.py

# A silent failure once produced a board with no nets at all, because the
# exit status was lost through a pipe. Refuse to finish on an empty netlist.
python -c "
from pads import POS
n = sum(1 for v in POS.values() if v[2])
assert n > 100, 'sanity: only %d of %d pads carry a net' % (n, len(POS))
print('sanity: %d of %d pads carry a net' % (n, len(POS)))"
