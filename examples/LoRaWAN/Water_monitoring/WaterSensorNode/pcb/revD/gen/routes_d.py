# -*- coding: utf-8 -*-
"""Block D: the battery terminal, the D6 gate line and the transceiver's
   two logic signals.

   RS_TX and RS_RX have to swap sides between the module and the terminal
   (U3.2 is west of U3.3, but J4.1 is east of J4.2), so one of them changes
   layer: RS_TX stays on the front, RS_RX goes underneath. Both ends are
   through-hole, so neither needs a via.
"""

W_PWR, W_SIG = 0.5, 0.25

ROUTES = [
    # --- +VBATT across the top to the WaziSense battery terminal --------------
    # Rev D: taken off the Q5 run at y 37 instead of the y 48 rail, so it stays
    # north of the WaziSense standoff H6 (58.12, 45.00).
    ('+VBATT', 'F.Cu', W_PWR, [(54.3, 'Q5.2'), (61.0, 'Q5.2'), (61.0, 23.0),
                               ('J6.1', 23.0), ('J6.1', 'J6.1')]),

    # --- D5: the gate line for the switched 3.3 V rail ------------------------
    ('D5', 'F.Cu', W_SIG, [('R2.1', 'R2.1'), ('R2.1', 61.0), (39.8, 61.0)]),
    # Rev D: a loop south round the WaziSense standoff H6 (58.12, 45.00). North
    # is blocked: the SWGND hop at (73.5, 41.5) and 3V3REG at y 42.4 hold the
    # lanes above y 43.
    ('D5', 'B.Cu', W_SIG, [(39.8, 61.0), (39.8, 43.0), (54.0, 43.0), (54.0, 48.8),
                           (62.2, 48.8), (62.2, 43.0), ('J5B.1', 43.0), ('J5B.1', 'J5B.1')]),

    # --- the transceiver's logic pair ----------------------------------------
    # Rev D: D and R of the THVD1406, both on its north pad row. R is an SMD
    # pad, so RS_RX surfaces through a via just above it.
    ('RS_TX', 'F.Cu', W_SIG, [('U3.4', 'U3.4'), ('U3.4', 36.0), ('J4.1', 36.0), ('J4.1', 'J4.1')]),
    ('RS_RX', 'B.Cu', W_SIG, [('J4.2', 'J4.2'), ('J4.2', 33.0), ('U3.1', 33.0), ('U3.1', 40.4)]),
    ('RS_RX', 'F.Cu', W_SIG, [('U3.1', 40.4), ('U3.1', 'U3.1')]),
]

VIAS = [
    ('D5', 39.8, 61.0),
    ('RS_RX', 'U3.1', 40.4),
]
