# -*- coding: utf-8 -*-
"""Prove the two BOMs say the same thing.

   There are two, and there have to be:

     BOM.csv                      the master. Every designator on the board,
                                  including the three parts deliberately not
                                  fitted and the four mounting holes.
     out/fab/..-bom-jlcpcb.csv    what JLCPCB is given. Only what the
                                  assembler places, and through-hole lines
                                  split so each stays inside what the quote
                                  will price.

   Those two differences are intended. Everything else must agree exactly,
   per designator: value, footprint, LCSC number, library type, unit price.
   This file turns that from a claim into a test, because the second file is
   derived from the first and derived files drift the moment nobody looks.
"""
from __future__ import print_function
import csv, io, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..')
MASTER = os.path.join(ROOT, 'BOM.csv')
FAB = os.path.join(ROOT, 'out', 'fab', 'KijaniCarrier-bom-jlcpcb.csv')


def by_designator(path, cols):
    out = {}
    for r in csv.DictReader(io.open(path, encoding='utf-8')):
        for d in [x.strip() for x in r['Designator'].split(',') if x.strip()]:
            out[d] = tuple((r.get(c) or '').strip() for c in cols)
    return out



#  Why each unfitted position stays unfitted. Checked against the netlist,
#  not against the BOM's own prose: H1-H4 carry no net at all, R11/R12 sit
#  on SDA/SCL where the isolator sockets U4/U5 already bring 4k7 pull-ups
#  of their own, and U2 shares 3V3P/3V3REG with JP1 - which IS fitted, and
#  would short the LDO's output onto its input if both were there.
WHY_EMPTY = {
    'H1': 'Befestigungsbohrung M3, ein Pad, kein Netz - nichts zu loeten',
    'H2': 'Befestigungsbohrung M3, ein Pad, kein Netz - nichts zu loeten',
    'H3': 'Befestigungsbohrung M3, ein Pad, kein Netz - nichts zu loeten',
    'H4': 'Befestigungsbohrung M3, ein Pad, kein Netz - nichts zu loeten',
    'R11': 'I2C-Pullup - die Isolatoren U4/U5 bringen eigene 4k7 mit',
    'R12': 'I2C-Pullup - die Isolatoren U4/U5 bringen eigene 4k7 mit',
    'U2': 'LDO, Alternative zu JP1. JP1 ist bestueckt - beide waeren ein Kurzschluss',
}

m_all = {}
not_fitted = set()
for r in csv.DictReader(io.open(MASTER, encoding='utf-8')):
    for d in [x.strip() for x in r['Designator'].split(',') if x.strip()]:
        m_all[d] = r
        if r['Assembly'] in ('DNP', 'mech'):
            not_fitted.add(d)

master = by_designator(MASTER, ['Value', 'Footprint', 'LCSC', 'Lib', 'UnitEUR'])
fab = by_designator(FAB, ['Comment', 'Footprint', 'LCSC Part #', 'Lib', 'Unit EUR'])

print('BOM.csv            %3d Designatoren' % len(master))
print('bom-jlcpcb.csv     %3d Designatoren' % len(fab))
print('nicht bestueckt    %3d  %s' % (len(not_fitted), ', '.join(sorted(not_fitted))))

bad = []

# 1. the fab file may omit only what is deliberately not fitted
only_master = set(master) - set(fab) - not_fitted
if only_master:
    bad.append('fehlt in der Fertigungs-BOM: %s' % sorted(only_master))

# 2. the fab file may not invent a designator
only_fab = set(fab) - set(master)
if only_fab:
    bad.append('steht nur in der Fertigungs-BOM: %s' % sorted(only_fab))

# 3. nothing deliberately unfitted may have slipped in
slipped = not_fitted & set(fab)
if slipped:
    bad.append('DNP/mech in der Fertigungs-BOM: %s' % sorted(slipped))

# 4. every shared designator must agree field by field
for d in sorted(set(master) & set(fab)):
    if master[d] != fab[d]:
        bad.append('%s weicht ab: Master %s / Fertigung %s' % (d, master[d], fab[d]))

print()
if bad:
    for b in bad:
        print('  ABWEICHUNG  ' + b)
    sys.exit(1)
print('Beide BOMs stimmen ueberein: Wert, Footprint, LCSC, Lib und Stueckpreis')
print('sind je Designator identisch.')
print()
print('Nicht an den Bestuecker gegeben, und das ist richtig so - an keiner')
print('dieser %d Positionen bleibt eine Loetstelle fuer Hand offen:' % len(not_fitted))
#  If someone marks a part DNP without saying why, say so rather than
#  printing a list that silently got shorter.
unexplained = sorted(not_fitted - set(WHY_EMPTY))
assert not unexplained, 'ohne Begruendung nicht bestueckt: %s' % unexplained
stale_reason = sorted(set(WHY_EMPTY) - not_fitted)
assert not stale_reason, 'Begruendung ohne Position: %s' % stale_reason
for d in sorted(WHY_EMPTY):
    print('  %-12s %s' % (d, WHY_EMPTY[d]))
print()
print('Das Board kommt also fertig bestueckt: jede Position, die ein Bauteil')
print('braucht, bekommt eines.')
