# -*- coding: utf-8 -*-
"""Block R: what Rev D adds - the RS485 transceiver on the board, its TVS, and
   the 12 V boost with its input selector.

   TRANSCEIVER (U3, THVD1406, SOIC-8, rot 270). North row, west to east:
   D SHDN RE R; south row: GND A B VCC. Its supply, 3V3RS, comes from Q6
   (west of the body) and runs under the body between the two rows (the
   body's east and west ends are open), so VCC, RE and SHDN all take it from
   there; tying RE and SHDN high is the auto-direction mode. GND and C11's
   return go to vias at y 49.7 into the GND pours. Q6's gate is SWGND: low
   while D6 is on, held at 3V3REG by R17 once it is off.

   BOOST (U8, TPS61085, MSOP-8, rot 0). East column top to bottom:
   SS FREQ IN SW; west column: COMP FB EN PGND. The power loop - SW, L1, D4,
   C14/C15, PGND - sits on the front layer in one tight ring below the IC;
   the small-signal parts (soft start, compensation, divider) sit above it.
   FREQ and EN are tied to IN: 1.2 MHz, always on while Q1 feeds it.

   BOOST_IN comes from JP3 pad 1 by the Pololu socket. It cannot cross the
   x 46.5-51.5 lanes (+VBATT, +12V, ISO_VIN) on either layer above y 82, so it
   drops to the back at JP3, runs south at x 43 and crosses at y 84, where the
   back layer is clear between ISO_VIN's end and the 3V3REG run at y 90.
"""

W_PWR, W_SIG = 0.5, 0.25
W_HOT = 0.8      # the switching loop: SW node and the output

ROUTES = [
    # ======================== RS485 transceiver =============================
    # ---- Q6, the switch: 3V3REG comes up the x 62.5 run from C8 to the
    #      source and on to R17; the drain feeds 3V3RS down to y 45 and east
    #      under the transceiver's body
    ('3V3REG', 'F.Cu', W_PWR, [(62.5, 45.0), ('Q6.2', 'Q6.2'), ('Q6.2', 'R17.1'), ('R17.1', 'R17.1')]),
    ('SWGND', 'F.Cu', W_SIG, [('Q6.1', 'Q6.1'), ('Q6.1', 'R17.2'), ('R17.2', 'R17.2')]),
    ('3V3RS', 'F.Cu', 0.4, [('Q6.3', 'Q6.3'), ('Q6.3', 45.0), ('U3.8', 45.0), ('U3.8', 'U3.8')]),
    ('3V3RS', 'F.Cu', W_SIG, [('U3.3', 43.3), ('U3.2', 43.3)]),
    ('3V3RS', 'F.Cu', W_SIG, [(68.0, 43.3), (68.0, 45.0)]),
    ('3V3RS', 'F.Cu', W_SIG, [('U3.8', 'C11.1'), ('C11.1', 'C11.1')]),
    # R7's top, down the west side of the body between the A via and R7's pads
    ('3V3RS', 'F.Cu', W_SIG, [(65.0, 45.0), (65.0, 'R7.2'), ('R7.2', 'R7.2')]),
    # Q6's gate: SWGND from the y 22 trunk, down the back at x 60.25 (clear of
    # J3 and J4's pads) and east at y 37.6 to a via beside the gate
    ('SWGND', 'B.Cu', W_SIG, [(60.25, 22.0), (60.25, 37.6), (65.3, 37.6)]),
    ('SWGND', 'F.Cu', W_SIG, [(65.3, 37.6), ('Q6.1', 37.6)]),
    # ---- the grounds that used to be SWGND: straight to the GND pours
    ('GND', 'F.Cu', W_SIG, [('C11.2', 'C11.2'), ('C11.2', 49.7)]),
    ('GND', 'F.Cu', W_SIG, [('U3.5', 'U3.5'), ('U3.5', 49.7)]),
    ('GND', 'B.Cu', W_SIG, [('U3.5', 49.7), ('C11.2', 49.7)]),
    # ---- 3V3REG on to the I2C pull-ups: the run under the body is 3V3RS now,
    #      so 3V3REG crosses on the back at y 42.4, between the D5 lane (y 43)
    #      and the SWGND via at (73.5, 41.5)
    ('3V3REG', 'B.Cu', W_SIG, [(62.5, 42.2), (62.5, 42.4), (79.5, 42.4), (79.5, 42.2)]),
    ('3V3REG', 'F.Cu', 0.4, [(79.5, 42.2), (79.5, 45.0), ('R12.2', 45.0), ('R12.2', 'R12.2')]),

    # ---- the TVS, beside the probe terminals: A from the front run at
    #      y 100, B from the back run at y 100 through a via
    ('RS485_A', 'F.Cu', W_SIG, [('D3.2', 100.0), ('D3.2', 'D3.2')]),
    ('RS485_B', 'B.Cu', W_SIG, [('D3.1', 100.0), ('D3.1', 101.0)]),
    ('RS485_B', 'F.Cu', W_SIG, [('D3.1', 101.0), ('D3.1', 'D3.1')]),
    ('GND', 'F.Cu', W_SIG, [('D3.3', 'D3.3'), ('D3.3', 105.4)]),

    # ======================== 12 V boost ====================================
    # ---- input: JP3 pad 1 -> back layer -> across at y 84 -> C12 ---------
    ('BOOST_IN', 'F.Cu', 0.6, [('JP3.1', 'JP3.1'), (43.0, 68.6)]),
    ('BOOST_IN', 'B.Cu', 0.6, [(43.0, 68.6), (43.0, 84.0), (56.5, 84.0)]),
    ('BOOST_IN', 'F.Cu', 0.6, [(56.5, 84.0), ('C12.2', 84.0), ('C12.2', 'C12.2')]),
    # C12 straight down into L1's input pad
    ('BOOST_IN', 'F.Cu', 0.6, [('C12.2', 'C12.2'), (65.4, 'C12.2'), (65.4, 'L1.2')]),
    # the IC's own supply: a thin branch through C13, the IN bypass
    ('BOOST_IN', 'F.Cu', 0.3, [('C13.1', 'C13.1'), ('C13.1', 88.8), (65.4, 88.8)]),
    # leaves from FREQ, not IN: one pin further from the SW track below
    ('BOOST_IN', 'F.Cu', 0.3, [('U8.7', 'U8.7'), (60.7, 'U8.7'), (60.7, 'C13.1'), ('C13.1', 'C13.1')]),
    ('BOOST_IN', 'F.Cu', W_SIG, [('U8.7', 'U8.7'), ('U8.6', 'U8.6')]),     # FREQ = IN: 1.2 MHz
    ('BOOST_IN', 'F.Cu', W_SIG, [('U8.3', 'U8.3'), ('U8.6', 'U8.6')]),     # EN = IN, under the body

    # ---- the switching loop: SW -> L1, L1 -> D4 anode ----------------------
    # 0.65 mm pitch: a narrow neck off the pin, full width once clear of IN
    ('BOOST_SW', 'F.Cu', 0.4, [('U8.5', 'U8.5'), (60.3, 'U8.5')]),
    ('BOOST_SW', 'F.Cu', 0.6, [(60.3, 'U8.5'), ('L1.1', 'U8.5')]),
    ('BOOST_SW', 'F.Cu', W_HOT, [('L1.1', 'L1.1'), ('L1.1', 'D4.2')]),
    # D4 cathode -> both output capacitors -> the +12V run at x 49.5
    ('+12V', 'F.Cu', W_HOT, [('D4.1', 'D4.1'), ('C14.1', 'D4.1'), ('C14.1', 'C14.1'),
                             ('C15.1', 'C15.1')]),
    ('+12V', 'F.Cu', W_HOT, [('C15.1', 'C15.1'), (49.5, 'C15.1')]),
    # the divider's top, from the same run
    ('+12V', 'F.Cu', W_SIG, [(49.5, 'R15.2'), ('R15.2', 'R15.2')]),

    # ---- power ground: PGND -> output capacitors, and down to the back ----
    # PGND straight down into C14's ground pad; narrow at the pin, which sits
    # 0.65 mm from EN
    ('GND', 'F.Cu', 0.4, [('U8.4', 'U8.4'), ('U8.4', 'C14.2')]),
    ('GND', 'F.Cu', W_HOT, [('C14.2', 'C14.2'), ('C15.2', 'C15.2')]),
    ('GND', 'F.Cu', 0.4, [('U8.4', 'U8.4'), (53.3, 'U8.4')]),
    ('GND', 'F.Cu', 0.5, [(53.0, 'C14.2'), (53.0, 94.3)]),
    # the divider's foot joins the output ground at C15
    ('GND', 'F.Cu', W_SIG, [('R16.1', 'R16.1'), ('R16.1', 'C15.2')]),
    # small-signal grounds: soft start and compensation share one via
    ('GND', 'F.Cu', W_SIG, [('C17.2', 'C17.2'), ('C16.2', 'C16.2')]),
    ('GND', 'F.Cu', W_SIG, [('C16.2', 'C16.2'), (59.0, 85.25)]),
    # input ground: C13 and C12 together, one via
    ('GND', 'F.Cu', 0.5, [('C13.2', 'C13.2'), ('C12.1', 'C12.1')]),
    ('GND', 'F.Cu', 0.5, [('C12.1', 'C12.1'), ('C12.1', 85.75)]),

    # ---- control: feedback divider, compensation, soft start --------------
    ('BOOST_FB', 'F.Cu', W_SIG, [('U8.2', 'U8.2'), ('R16.2', 'R16.2')]),
    ('BOOST_FB', 'F.Cu', W_SIG, [('R16.2', 'R16.2'), ('R15.1', 'R15.1')]),
    ('BOOST_COMP', 'F.Cu', W_SIG, [('U8.1', 'U8.1'), (55.2, 'U8.1'), ('R14.1', 'R14.1')]),
    ('BOOST_RC', 'F.Cu', W_SIG, [('R14.2', 'R14.2'), ('C17.1', 'C17.1')]),
    ('BOOST_SS', 'F.Cu', W_SIG, [('U8.8', 'U8.8'), (59.6, 'U8.8'), (59.6, 'C16.1'), ('C16.1', 'C16.1')]),
]

VIAS = [
    ('GND', 'C11.2', 49.7),
    ('GND', 'U3.5', 49.7),
    ('SWGND', 65.3, 37.6),
    ('3V3REG', 62.5, 42.2),
    ('3V3REG', 79.5, 42.2),
    ('RS485_B', 'D3.1', 101.0),
    ('GND', 'D3.3', 105.4),
    ('BOOST_IN', 43.0, 68.6),
    ('BOOST_IN', 56.5, 84.0),
    ('GND', 53.3, 'U8.4'),
    ('GND', 53.0, 94.3),
    ('GND', 59.0, 85.25),
    ('GND', 'C12.1', 85.75),
]
