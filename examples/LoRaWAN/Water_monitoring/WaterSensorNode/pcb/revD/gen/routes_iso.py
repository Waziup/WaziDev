# -*- coding: utf-8 -*-
"""Block ISO: probe runs and the isolated side of both chains."""

# All coordinates are KiCad board coordinates (mm). Plan -> KiCad: x+20, 120-y.
#
# Pad order follows the modules as they sit on the board, component side up,
# read left to right -- isolator "VCC OFF GND A B" / "3V9 NC GND A B",
# EZO "GND TX RX" / "VCC PRB PGND".
#
#   isolator isolated row, y=78.89:  3V9 78.92 | NC 81.46 | GND 84.00 | A 86.54 | B 89.08
#   EZO bus row,           y=87.11:              GND 81.46 | TX 84.00 | RX 86.54
#   EZO probe row,        y=104.89:              VCC 81.46 | PRB 84.00 | PGND 86.54
#   SMA centre (84,113), ground posts at x 81.46 / 86.54, y 110.46 / 115.54
#
# The EC chain repeats all of this 22 mm to the right.
ROUTES = [
    # --- pH electrode: PRB straight down to the SMA centre pin ---------------
    ('PH_PRB',  'F.Cu', 0.25, [('U6.5', 'U6.5'), ('J7.1', 'J7.1')]),
    ('PH_PGND', 'F.Cu', 0.25, [('U6.6', 'U6.6'), ('J7.2#2', 'J7.2#2')]),
    # tie the four SMA ground posts together, clear of the centre pin
    ('PH_PGND', 'F.Cu', 0.25, [('J7.2#2', 'J7.2#2'), ('J7.2#3', 'J7.2#3'),
                               ('J7.2#1', 'J7.2#1'), ('J7.2', 'J7.2')]),

    # --- EC electrode: same geometry, 22 mm to the right ---------------------
    ('EC_PRB',  'F.Cu', 0.25, [('U7.5', 'U7.5'), ('J9.1', 'J9.1')]),
    ('EC_PGND', 'F.Cu', 0.25, [('U7.6', 'U7.6'), ('J9.2#2', 'J9.2#2')]),
    ('EC_PGND', 'F.Cu', 0.25, [('J9.2#2', 'J9.2#2'), ('J9.2#3', 'J9.2#3'),
                               ('J9.2#1', 'J9.2#1'), ('J9.2', 'J9.2')]),

    # --- isolated side of each chain: isolator -> EZO ------------------------
    # All of these stay BELOW the barrier slot. Nothing here may cross y=70.
    ('ISO1_GND', 'F.Cu', 0.25, [('U4.8', 'U4.8'), ('U6.1', 'U6.1')]),
    ('ISO1_A',   'F.Cu', 0.25, [('U4.9', 'U4.9'), ('U6.2', 'U6.2')]),
    ('ISO1_B',   'F.Cu', 0.25, [('U4.10', 'U4.10'), ('U6.3', 'U6.3')]),
    # VCC sits on the EZO's probe-side row, so it runs down the clear lane at x=78.92
    ('ISO1_3V9', 'F.Cu', 0.25, [('U4.6', 'U4.6'), ('U4.6', 'U6.4'), ('U6.4', 'U6.4')]),

    ('ISO2_GND', 'F.Cu', 0.25, [('U5.8', 'U5.8'), ('U7.1', 'U7.1')]),
    ('ISO2_A',   'F.Cu', 0.25, [('U5.9', 'U5.9'), ('U7.2', 'U7.2')]),
    ('ISO2_B',   'F.Cu', 0.25, [('U5.10', 'U5.10'), ('U7.3', 'U7.3')]),
    ('ISO2_3V9', 'F.Cu', 0.25, [('U5.6', 'U5.6'), ('U5.6', 'U7.4'), ('U7.4', 'U7.4')]),
]


VIAS = []
