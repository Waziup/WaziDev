# -*- coding: utf-8 -*-
"""Block E: the RS485 bus.

   Both nets have five nodes: the termination resistor, the bias resistor, the
   screw terminal at J8, and the two probe terminals at the bottom edge. They
   come down the front layer in two columns, A at x=67 and B at x=72.5, both
   west of the pH isolation slot at x=74 so neither crosses a barrier. The
   short link across the resistor row changes layer, because the 3V3REG feed
   to R7 sits between RT1 and R8.
"""

W_SIG = 0.25

ROUTES = [
    # --- RS485_A: termination, bias, terminal ---------------------------------
    ('RS485_A', 'F.Cu', W_SIG, [('RT1.1', 'RT1.1'), ('RT1.1', 54.0)]),
    ('RS485_A', 'B.Cu', W_SIG, [('RT1.1', 54.0), ('R7.1', 54.0), ('J8.1', 54.0),
                                ('J8.1', 'J8.1')]),
    ('RS485_A', 'F.Cu', W_SIG, [('R7.1', 54.0), ('R7.1', 'R7.1')]),
    # down the front layer to the two probe terminals
    ('RS485_A', 'F.Cu', W_SIG, [('J8.1', 'J8.1'), (67.0, 52.0), (67.0, 100.0)]),
    ('RS485_A', 'F.Cu', W_SIG, [(67.0, 100.0), ('X3.3', 100.0), ('X3.3', 'X3.3')]),
    ('RS485_A', 'F.Cu', W_SIG, [(67.0, 100.0), (67.0, 104.0), (71.0, 104.0),
                                (71.0, 111.0), ('X4.3', 111.0), ('X4.3', 'X4.3')]),

    # --- RS485_B: the same, one column further east ---------------------------
    ('RS485_B', 'F.Cu', W_SIG, [('RT1.2', 'RT1.2'), ('RT1.2', 55.5)]),
    ('RS485_B', 'B.Cu', W_SIG, [('RT1.2', 55.5), ('R8.1', 55.5), ('J8.2', 55.5),
                                (72.5, 55.5)]),
    ('RS485_B', 'F.Cu', W_SIG, [('R8.1', 55.5), ('R8.1', 'R8.1')]),
    ('RS485_B', 'B.Cu', W_SIG, [('J8.2', 55.5), ('J8.2', 'J8.2')]),
    ('RS485_B', 'F.Cu', W_SIG, [(72.5, 55.5), (72.5, 100.0), (72.5, 110.0)]),
    ('RS485_B', 'B.Cu', W_SIG, [(72.5, 100.0), ('X3.4', 100.0), ('X3.4', 'X3.4')]),
    ('RS485_B', 'F.Cu', W_SIG, [(72.5, 110.0), ('X4.4', 110.0), ('X4.4', 'X4.4')]),
]

VIAS = [
    ('RS485_A', 'RT1.1', 54.0),
    ('RS485_A', 'R7.1', 54.0),
    ('RS485_B', 'RT1.2', 55.5),
    ('RS485_B', 'R8.1', 55.5),
    ('RS485_B', 72.5, 55.5),
    ('RS485_B', 72.5, 100.0),
]
