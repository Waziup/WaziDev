# -*- coding: utf-8 -*-
"""Block G: ground stitching.

   Three GND pads end up in pockets the pour cannot reach, each walled in by
   the traces around it: Q2's source and R3 behind the +VBATT L-shape, and C6
   between the 3V3P and 3V3REG runs. Rather than trust a half-millimetre
   squeeze between two keepouts, each is wired out and tied to the pour on the
   back layer with a via. pour.py is what found them.
"""

W_SIG = 0.25

ROUTES = [
    # Q2 source and R3, out of the pocket under the gate-driver block
    ('GND', 'F.Cu', W_SIG, [('Q2.2', 'Q2.2'), ('Q2.2', 55.6), (45.5, 55.6), (45.5, 61.5)]),
    ('GND', 'F.Cu', W_SIG, [(45.5, 61.5), (45.5, 62.0)]),
    ('GND', 'F.Cu', W_SIG, [('R3.2', 'R3.2'), ('R3.2', 61.5), (45.5, 61.5)]),
    # U1's ground pin, boxed in between its own two neighbours on both layers.
    # It leaves on the back through the gap between the V12IN and +12V runs.
    # KiCad's own filler found this one; the raster check had let the 0.44 mm
    # squeeze beside the pad count as copper.
    ('GND', 'B.Cu', W_SIG, [('U1.2', 'U1.2'), ('U1.2', 70.9), (36.0, 70.9),
                            (36.0, 67.0)]),
    # C6, out from between the two 3.3 V runs
    ('GND', 'F.Cu', W_SIG, [('C6.2', 'C6.2'), (41.0, 'C6.2'), (41.0, 93.0)]),
]

VIAS = [
    # the front-layer pour breaks into six islands down here; each one that
    # carries a pad needs its own tie to the back. isl.py lists them.
    ('GND', 60.0, 95.0),
    ('GND', 42.0, 79.0),
    ('GND', 70.0, 90.0),
    ('GND', 45.5, 62.0),
    ('GND', 36.0, 67.0),
    ('GND', 41.0, 93.0),
]
