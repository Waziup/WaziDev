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
    ('+VBATT', 'F.Cu', W_PWR, [('D2.1', 48.0), (61.0, 48.0), (61.0, 23.0),
                               ('J6.1', 23.0), ('J6.1', 'J6.1')]),

    # --- D5: the gate line for the switched 3.3 V rail ------------------------
    ('D5', 'F.Cu', W_SIG, [('R2.1', 'R2.1'), ('R2.1', 61.0), (39.8, 61.0)]),
    ('D5', 'B.Cu', W_SIG, [(39.8, 61.0), (39.8, 43.0), ('J5.3', 43.0), ('J5.3', 'J5.3')]),

    # --- the transceiver's logic pair ----------------------------------------
    ('RS_TX', 'F.Cu', W_SIG, [('U3.2', 'U3.2'), ('U3.2', 36.0), ('J4.1', 36.0), ('J4.1', 'J4.1')]),
    ('RS_RX', 'B.Cu', W_SIG, [('U3.3', 'U3.3'), ('U3.3', 33.0), ('J4.2', 33.0), ('J4.2', 'J4.2')]),
]

VIAS = [
    ('D5', 39.8, 61.0),
]
