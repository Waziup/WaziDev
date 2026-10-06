# -*- coding: utf-8 -*-
"""Block B: the left column and the runs up to the WaziSense terminals.

   Four B.Cu lanes run north-south down the left edge, ordered so they never
   have to cross each other:

       x=21.5  SWGND   (X2.3 at the bottom up to J2.2 and on to the MAX3485)
       x=24.5  3V3P    (the jumper up to J2.1)
       x=27.5  3V3REG  (down to the DS18B20 terminal)
       x=29.5  A1      (R5.1 up to J3.2)
       x=38.5  OW_DATA (R9.1 up to J3.1)

   The three east-west runs across the top are stacked at y=22 (SWGND),
   y=30 (3V3P) and y=33 (A1), each turning south only after the lanes it
   would otherwise cross have already ended.
"""

W_PWR, W_SIG = 0.5, 0.25

ROUTES = [
    # --- SWGND: the switched return. Every MCU-side device hangs on this. -----
    ('SWGND', 'B.Cu', W_SIG, [('X2.3', 'X2.3'), ('X2.3', 117.0), (21.5, 117.0),
                              (21.5, 22.0), ('J2.2', 22.0), ('J2.2', 'J2.2')]),
    ('SWGND', 'B.Cu', W_SIG, [('J2.2', 22.0), (73.5, 22.0), (73.5, 'U3.4'), ('U3.4', 'U3.4')]),

    # --- 3V3P: the permanently live WaziSense rail ----------------------------
    ('3V3P', 'F.Cu', W_PWR, [('U2.2', 'U2.2'), ('U2.2', 90.8), ('C6.1', 90.8), ('C6.1', 'C6.1')]),
    ('3V3P', 'F.Cu', W_PWR, [('U2.2', 90.8), ('JP1.1', 'JP1.1')]),
    ('3V3P', 'F.Cu', W_PWR, [('JP1.1', 'JP1.1'), (24.5, 'JP1.1')]),
    ('3V3P', 'B.Cu', W_PWR, [(24.5, 'JP1.1'), (24.5, 30.0), ('J2.1', 30.0), ('J2.1', 'J2.1')]),

    # --- 3V3REG: the carrier's own regulated rail -----------------------------
    ('3V3REG', 'F.Cu', W_PWR, [('U2.3', 'U2.3'), (34.5, 'U2.3'), (34.5, 85.0),
                               ('C7.1', 85.0), ('C7.1', 'C7.1')]),
    ('3V3REG', 'F.Cu', W_PWR, [('JP1.2', 'JP1.2'), ('JP1.2', 96.0),
                               ('R9.2', 96.0), ('R9.2', 'R9.2')]),
    ('3V3REG', 'F.Cu', W_PWR, [('R9.2', 'R9.2'), ('C7.1', 'R9.2'), ('C7.1', 'C7.1')]),
    # west to the DS18B20 terminal
    ('3V3REG', 'F.Cu', W_PWR, [(34.5, 85.0), (27.5, 85.0)]),
    ('3V3REG', 'B.Cu', W_PWR, [(27.5, 85.0), (27.5, 110.0), ('X2.1', 110.0), ('X2.1', 'X2.1')]),
    # east to the RS485 section
    ('3V3REG', 'B.Cu', W_PWR, [('C7.1', 90.0), (57.5, 90.0), (57.5, 'C8.1'), ('C8.1', 'C8.1')]),

    # --- OW_DATA: DS18B20 data, with its pull-up at R9 ------------------------
    ('OW_DATA', 'F.Cu', W_SIG, [('R9.1', 'R9.1'), (36.0, 'R9.1')]),
    ('OW_DATA', 'B.Cu', W_SIG, [(36.0, 'R9.1'), (36.0, 105.0), ('X2.2', 105.0), ('X2.2', 'X2.2')]),
    ('OW_DATA', 'F.Cu', W_SIG, [('R9.1', 'R9.1'), ('R9.1', 103.0)]),
    # the lane has to cross U1's +12V output; it steps onto the front layer
    # for 3 mm rather than making that output detour
    ('OW_DATA', 'B.Cu', W_SIG, [('R9.1', 103.0), (38.5, 103.0), (38.5, 74.0), (41.0, 74.0)]),
    ('OW_DATA', 'F.Cu', W_SIG, [(41.0, 74.0), (41.0, 71.0)]),
    ('OW_DATA', 'B.Cu', W_SIG, [(41.0, 71.0), (38.5, 71.0), (38.5, 36.0),
                                ('J3.1', 36.0), ('J3.1', 'J3.1')]),

    # --- A1: the Q4 gate drive back to the MCU --------------------------------
    ('A1', 'F.Cu', W_SIG, [('R5.1', 'R5.1'), (35.5, 'R5.1'), (35.5, 82.0)]),
    ('A1', 'B.Cu', W_SIG, [(35.5, 82.0), (29.5, 82.0), (29.5, 33.0),
                           ('J3.2', 33.0), ('J3.2', 'J3.2')]),

    # --- +12V on down to the two RS485 probe terminals ------------------------
    ('+12V', 'F.Cu', W_PWR, [('C4.1', 'C4.1'), (49.5, 'C4.1'), (49.5, 104.0),
                             (49.5, 108.0), ('X3.1', 108.0), ('X3.1', 'X3.1')]),
    # the second probe terminal is fed underneath, so the RS485 descent can
    # pass down the front layer
    ('+12V', 'B.Cu', W_PWR, [(49.5, 104.0), (54.0, 104.0)]),
    ('+12V', 'F.Cu', W_PWR, [(54.0, 104.0), (59.0, 104.0)]),
    ('+12V', 'B.Cu', W_PWR, [(59.0, 104.0), ('X4.1', 104.0), ('X4.1', 'X4.1')]),
]

VIAS = [
    ('3V3P', 24.5, 'JP1.1'),
    ('3V3REG', 27.5, 85.0),
    ('3V3REG', 'C7.1', 90.0),
    ('OW_DATA', 36.0, 'R9.1'),
    ('OW_DATA', 'R9.1', 103.0),
    ('OW_DATA', 41.0, 74.0),
    ('OW_DATA', 41.0, 71.0),
    ('A1', 35.5, 82.0),
    ('+12V', 49.5, 104.0),
    ('+12V', 54.0, 104.0),
    ('+12V', 59.0, 104.0),
]
