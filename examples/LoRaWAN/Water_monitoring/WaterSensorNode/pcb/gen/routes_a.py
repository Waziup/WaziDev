# -*- coding: utf-8 -*-
"""Block A: the power section on the left.

   Q1/Q2 (y 53..58) switch the boost input, Q3/Q4 (y 75..80) the isolator
   supply. Both are the same circuit 22 mm apart, so the routing is the same
   shape twice. Points are written as 'REF.PAD' wherever they touch a pad, so
   no coordinate is ever transcribed by hand.
"""

W_PWR, W_SIG = 0.5, 0.25

ROUTES = [
    # --- cell -> PTC -> reverse-polarity diode --------------------------------
    ('VB1', 'F.Cu', W_PWR, [('X1.1', 'X1.1'), ('X1.1', 34.5), ('F1.1', 34.5), ('F1.1', 'F1.1')]),
    ('VB2', 'F.Cu', W_PWR, [('F1.2', 'F1.2'), ('F1.2', 31.5), ('D1.2', 31.5), ('D1.2', 'D1.2')]),

    # --- +VBATT: diode -> bulk cap -> TVS -> both gate blocks ------------------
    ('+VBATT', 'F.Cu', W_PWR, [('D1.1', 'D1.1'), ('D1.1', 41.5), ('C1.1', 41.5), ('C1.1', 'C1.1')]),
    ('+VBATT', 'F.Cu', W_PWR, [('C1.1', 'C1.1'), ('C1.1', 48.0), ('D2.1', 48.0), ('D2.1', 'D2.1')]),
    ('+VBATT', 'F.Cu', W_PWR, [('D2.1', 'D2.1'), ('D2.1', 'C3.2'), ('C3.2', 'C3.2')]),
    ('+VBATT', 'F.Cu', W_PWR, [('C3.2', 'C3.2'), (46.5, 'C3.2'), (46.5, 64.5),
                               ('R1.2', 64.5), ('R1.2', 'R1.2')]),
    # tap for the lower block, taken east of C4 so it clears the +12V run
    ('+VBATT', 'F.Cu', W_PWR, [('D2.1', 'C3.2'), (50.5, 'C3.2'), (50.5, 62.0)]),
    ('+VBATT', 'F.Cu', W_PWR, [('R1.2', 'R1.2'), ('R1.2', 56.8), ('Q1.2', 56.8), ('Q1.2', 'Q1.2')]),
    # down to the lower gate block on the back layer, clear of C4 and U1
    ('+VBATT', 'B.Cu', W_PWR, [(50.5, 62.0), (50.5, 71.5), (47.5, 71.5), (47.5, 'C5.2')]),
    ('+VBATT', 'F.Cu', W_PWR, [(47.5, 'C5.2'), ('C5.2', 'C5.2')]),
    ('+VBATT', 'F.Cu', W_PWR, [(47.5, 'C5.2'), (47.5, 83.5), ('R4.2', 83.5), ('R4.2', 'R4.2')]),
    ('+VBATT', 'F.Cu', W_PWR, [('R4.2', 'R4.2'), ('R4.2', 78.8), ('Q3.2', 78.8), ('Q3.2', 'Q3.2')]),

    # --- gate nets: each MOSFET pair with its pull-up and speed-up cap ---------
    ('Q1_G', 'F.Cu', W_SIG, [('Q1.1', 'Q1.1'), ('Q1.1', 50.5), ('Q2.3', 50.5), ('Q2.3', 'Q2.3')]),
    ('Q1_G', 'F.Cu', W_SIG, [('Q2.3', 'Q2.3'), ('C3.1', 'C3.1')]),
    ('Q1_G', 'F.Cu', W_SIG, [('R1.1', 'R1.1'), (28.5, 'R1.1'), (28.5, 'Q1.1'), ('Q1.1', 'Q1.1')]),

    ('Q2_G', 'F.Cu', W_SIG, [('Q2.1', 'Q2.1'), (35.5, 'Q2.1'), (35.5, 56.5),
                             ('R2.2', 56.5), ('R2.2', 'R2.2')]),
    ('Q2_G', 'F.Cu', W_SIG, [('R2.2', 'R2.2'), ('R3.1', 'R3.1')]),

    ('Q3_G', 'F.Cu', W_SIG, [('Q3.1', 'Q3.1'), (28.5, 'Q3.1'), (28.5, 70.0),
                             ('Q4.3', 70.0), ('Q4.3', 'Q4.3')]),
    ('Q3_G', 'F.Cu', W_SIG, [('Q4.3', 'Q4.3'), ('C5.1', 'C5.1')]),
    ('Q3_G', 'F.Cu', W_SIG, [('R4.1', 'R4.1'), (28.5, 'R4.1'), (28.5, 'Q3.1')]),

    ('Q4_G', 'F.Cu', W_SIG, [('Q4.1', 'Q4.1'), (35.5, 'Q4.1'), (35.5, 78.5),
                             ('R5.2', 78.5), ('R5.2', 'R5.2')]),
    ('Q4_G', 'F.Cu', W_SIG, [('R5.2', 'R5.2'), ('R6.1', 'R6.1')]),

    # --- boost input: Q1 drain to the Pololu VIN, round U1 on the back --------
    ('V12IN', 'F.Cu', W_PWR, [('Q1.3', 'Q1.3'), (34.5, 'Q1.3')]),
    ('V12IN', 'B.Cu', W_PWR, [(34.5, 'Q1.3'), (34.5, 70.0), ('U1.3', 70.0), ('U1.3', 'U1.3')]),

    # --- boost output: out of U1 on the back, under the Q3 gate lane ----------
    ('+12V', 'B.Cu', W_PWR, [('U1.1', 'U1.1'), (42.0, 'U1.1'), (42.0, 'C4.1'), ('C4.1', 'C4.1')]),
]

VIAS = [
    ('V12IN', 34.5, 'Q1.3'),
    ('+VBATT', 50.5, 62.0),
    ('+VBATT', 47.5, 'C5.2'),
]
