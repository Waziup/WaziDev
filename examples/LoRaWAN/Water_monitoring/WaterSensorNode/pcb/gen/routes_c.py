# -*- coding: utf-8 -*-
"""Block C: everything east of the power section.

   The four MCU-side nets reach the isolators' top row in a fixed order so
   they never have to cross one another: each one drops onto its own pad and
   only then continues east to the second isolator, on the back layer, in
   lanes stacked 63 / 65 / 67 mm. All three stay well north of the isolation
   slot at y 68.8.

   ISO_VIN crosses the whole board from Q3's drain. It steps onto the front
   layer for 7 mm to get past the OW_DATA lane, then runs east at y=58.5,
   north of both isolators' pads.
"""

W_PWR, W_SIG = 0.5, 0.25

ROUTES = [
    # --- ISO_VIN: switched supply for both isolators --------------------------
    ('ISO_VIN', 'F.Cu', W_PWR, [('Q3.3', 'Q3.3'), (34.3, 'Q3.3'), (34.3, 78.0)]),
    ('ISO_VIN', 'B.Cu', W_PWR, [(34.3, 78.0), (37.0, 78.0), (37.0, 81.5)]),
    ('ISO_VIN', 'F.Cu', W_PWR, [(37.0, 81.5), (44.0, 81.5)]),
    ('ISO_VIN', 'B.Cu', W_PWR, [(44.0, 81.5), (51.5, 81.5), (51.5, 58.5),
                                ('U4.1', 58.5), ('U4.1', 'U4.1')]),
    ('ISO_VIN', 'B.Cu', W_PWR, [('U4.1', 58.5), ('U5.1', 58.5), ('U5.1', 'U5.1')]),

    # --- SWGND out to both isolators -----------------------------------------
    ('SWGND', 'F.Cu', W_SIG, [('R8.2', 'R8.2'), ('R8.2', 61.0)]),
    ('SWGND', 'B.Cu', W_SIG, [('R8.2', 61.0), ('R8.2', 63.0), ('U4.3', 63.0),
                              ('U5.3', 63.0), ('U5.3', 'U5.3')]),
    ('SWGND', 'B.Cu', W_SIG, [('U4.3', 63.0), ('U4.3', 'U4.3')]),
    # the transceiver's ground used to reach the isolators only through the
    # SWGND pour, and the RS485 descents cut that pour into pieces. Wire it.
    ('SWGND', 'F.Cu', W_SIG, [('U3.4', 'U3.4'), ('U3.4', 44.0)]),
    ('SWGND', 'B.Cu', W_SIG, [('U3.4', 44.0), (75.0, 44.0), (75.0, 56.0)]),
    ('SWGND', 'F.Cu', W_SIG, [(75.0, 56.0), ('U4.3', 56.0), ('U4.3', 'U4.3')]),

    # --- SDA: down from the I2C terminal, then east on the back layer --------
    ('SDA', 'F.Cu', W_SIG, [('J5.2', 'J5.2'), ('J5.2', 34.0), ('U4.4', 34.0),
                            ('U4.4', 'R11.1'), ('U4.4', 'U4.4')]),
    ('SDA', 'F.Cu', W_SIG, [('R11.1', 'R11.1'), ('U4.4', 'R11.1')]),
    ('SDA', 'F.Cu', W_SIG, [('U4.4', 'U4.4'), ('U4.4', 65.0)]),
    ('SDA', 'B.Cu', W_SIG, [('U4.4', 65.0), ('U5.4', 65.0), ('U5.4', 'U5.4')]),

    # --- SCL: the same one lane further out ----------------------------------
    ('SCL', 'F.Cu', W_SIG, [('J5.1', 'J5.1'), ('J5.1', 31.0), ('U4.5', 31.0),
                            ('U4.5', 44.0), ('U4.5', 'U4.5')]),
    ('SCL', 'F.Cu', W_SIG, [('R12.1', 'R12.1'), (85.5, 'R12.1'), (85.5, 44.0)]),
    ('SCL', 'B.Cu', W_SIG, [(85.5, 44.0), ('U4.5', 44.0)]),
    ('SCL', 'F.Cu', W_SIG, [('U4.5', 'U4.5'), ('U4.5', 67.0)]),
    ('SCL', 'B.Cu', W_SIG, [('U4.5', 67.0), ('U5.5', 67.0), ('U5.5', 'U5.5')]),

    # --- 3V3REG around the transceiver and the two pull-ups -------------------
    ('3V3REG', 'F.Cu', W_PWR, [('C8.1', 'C8.1'), ('C10.1', 'C10.1')]),
    ('3V3REG', 'F.Cu', W_PWR, [('C10.1', 'C10.1'), ('C10.1', 70.0),
                               ('C9.1', 70.0), ('C9.1', 'C9.1')]),
    ('3V3REG', 'F.Cu', W_PWR, [('C8.1', 'C8.1'), ('C8.1', 62.0),
                               ('R7.2', 62.0), ('R7.2', 'R7.2')]),
    ('3V3REG', 'F.Cu', W_PWR, [('U3.1', 'U3.1'), ('U3.1', 48.0), (62.5, 48.0),
                               (62.5, 'C8.1'), ('C8.1', 'C8.1')]),
    ('3V3REG', 'F.Cu', W_PWR, [('U3.1', 48.0), ('U3.1', 46.0),
                               ('R12.2', 46.0), ('R12.2', 'R12.2')]),
    ('3V3REG', 'F.Cu', W_PWR, [('R12.2', 'R12.2'), ('R11.2', 'R11.2')]),
]

VIAS = [
    ('ISO_VIN', 34.3, 78.0),
    ('ISO_VIN', 37.0, 81.5),
    ('ISO_VIN', 44.0, 81.5),
    ('SDA', 'U4.4', 65.0),
    ('SCL', 'U4.5', 67.0),
    ('SCL', 85.5, 44.0),
    ('SCL', 'U4.5', 44.0),
    ('SWGND', 'R8.2', 61.0),
    ('SWGND', 'U3.4', 44.0),
    ('SWGND', 75.0, 56.0),
]
