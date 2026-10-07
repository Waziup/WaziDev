# -*- coding: utf-8 -*-
"""What does the series Schottky cost per day, and what would a P-MOSFET save?

   D1 (SS34) sits in the one path every milliamp takes: cell -> PTC -> D1 ->
   +VBATT. Everything downstream of it is a switching converter, so a lower
   input voltage is paid for in extra input CURRENT, not in a lower output.
   That is what makes the drop expensive rather than merely inconvenient.

   Figures are Felix's own measurements where they exist; assumptions are
   named. This is an estimate, not a budget.
"""
from __future__ import print_function

CELL_V = 3.7          # nominal over the discharge
CYCLE_S = 1800.0      # the log says 1800 s sleep
ACTIVE_S = 48.0       # 11:21:36 banner to 11:22:24 "will sleep now"
I_SLEEP = 1.08e-3     # measured
I_ACTIVE = 100e-3     # assumed average over the window; the measured PEAK is 160 mA
VF_ACTIVE = 0.30      # SS34 at ~100 mA
VF_SLEEP = 0.15       # SS34 at ~1 mA
RDSON = 0.050         # AO3401A as an ideal-diode pass element

cycles = 86400.0 / CYCLE_S
sleep_s = CYCLE_S - ACTIVE_S

print('Annahmen: %.0f Zyklen/Tag, je %.0f s aktiv bei %.0f mA, %.0f s Schlaf bei %.2f mA'
      % (cycles, ACTIVE_S, I_ACTIVE * 1e3, sleep_s, I_SLEEP * 1e3))
print()

# --- charge drawn from the cell -------------------------------------------
q_active = I_ACTIVE * ACTIVE_S * cycles / 3.6      # mAh/day
q_sleep = I_SLEEP * sleep_s * cycles / 3.6
print('Ladung pro Tag ohne Diodenaufschlag:  %5.1f mAh  (%.1f aktiv + %.1f Schlaf)'
      % (q_active + q_sleep, q_active, q_sleep))

# --- what the diode burns --------------------------------------------------
e_d = (VF_ACTIVE * I_ACTIVE * ACTIVE_S + VF_SLEEP * I_SLEEP * sleep_s) * cycles
e_f = (RDSON * I_ACTIVE ** 2 * ACTIVE_S + RDSON * I_SLEEP ** 2 * sleep_s) * cycles
print('Verlust Schottky:  %5.1f J/Tag' % e_d)
print('Verlust P-MOSFET:  %5.2f J/Tag' % e_f)

saved_J = e_d - e_f
saved_mAh = saved_J / CELL_V / 3.6
print('Ersparnis:         %5.1f J/Tag  =  %.1f mAh/Tag am Akku' % (saved_J, saved_mAh))
print()

total = q_active + q_sleep
print('Das sind %.1f %% des Tagesverbrauchs von %.0f mAh.' % (100.0 * saved_mAh / total, total))
for cap in (2500.0, 3400.0):
    print('   %4.0f mAh Zelle:  %.1f Tage statt %.1f Tage  (+%.1f Tage)'
          % (cap, cap / (total - saved_mAh), cap / total,
             cap / (total - saved_mAh) - cap / total))
print()
print('Wichtiger als die Energie ist die Spannungsreserve:')
print('   bei 3,0 V Zellenspannung sieht der Boost mit Diode  %.2f V' % (3.0 - VF_ACTIVE))
print('   ohne Diode                                          %.2f V' % (3.0 - RDSON * I_ACTIVE))
print('   der U3V9F12 braucht mindestens                      2,50 V')
