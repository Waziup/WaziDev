# -*- coding: utf-8 -*-
"""The netlist, written straight into the board, plus the four copper zones."""

from __future__ import print_function

import sys as _sys
_GENERATOR_GUARD = True
if __name__ != '__main__':
    raise ImportError('nets.py rewrites the board file; run it, do not import it')
import io, re, hashlib

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
BOM = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\BOM.csv'

# SOT-23 here is 1=Gate 2=Source 3=Drain; MCP1700 is 1=GND 2=VIN 3=VOUT;
# diodes 1=cathode 2=anode; SMA 1=centre 2=shield.
NETS = {
    'VB1':      [('X1', 1), ('F1', 1)],
    'VB2':      [('F1', 2), ('D1', 2)],
    '+VBATT':   [('D1', 1), ('D2', 1), ('C1', 1), ('Q1', 2), ('Q3', 2),
                 ('R1', 2), ('C3', 2), ('R4', 2), ('C5', 2), ('J6', 1)],
    'GND':      [('X1', 2), ('D2', 2), ('C1', 2), ('Q2', 2), ('Q4', 2),
                 ('R3', 2), ('R6', 2), ('J6', 2), ('U1', 2), ('C4', 2),
                 ('U2', 1), ('C6', 2), ('C7', 2), ('C8', 2), ('C9', 2), ('C10', 2),
                 ('X3', 2), ('X4', 2), ('J3', 3), ('J4', 3), ('J5', 4)],

    'SWGND':    [('J2', 2), ('U3', 4), ('X2', 3), ('R8', 2), ('U4', 3), ('U5', 3)],
    '3V3P':     [('J2', 1), ('U2', 2), ('C6', 1), ('JP1', 1)],
    '3V3REG':   [('U2', 3), ('JP1', 2), ('C7', 1), ('C8', 1), ('C9', 1), ('C10', 1),
                 ('U3', 1), ('R7', 2), ('R9', 2), ('R11', 2), ('R12', 2), ('X2', 1)],

    'D5':       [('J5', 3), ('R2', 1)],
    'Q2_G':     [('R2', 2), ('Q2', 1), ('R3', 1)],
    'Q1_G':     [('Q2', 3), ('R1', 1), ('C3', 1), ('Q1', 1)],
    'V12IN':    [('Q1', 3), ('U1', 3)],
    '+12V':     [('U1', 1), ('C4', 1), ('X3', 1), ('X4', 1)],

    'A1':       [('J3', 2), ('R5', 1)],
    'Q4_G':     [('R5', 2), ('Q4', 1), ('R6', 1)],
    'Q3_G':     [('Q4', 3), ('R4', 1), ('C5', 1), ('Q3', 1)],
    'ISO_VIN':  [('Q3', 3), ('U4', 1), ('U5', 1)],

    'RS_TX':    [('J4', 1), ('U3', 2)],
    'RS_RX':    [('J4', 2), ('U3', 3)],
    'OW_DATA':  [('J3', 1), ('X2', 2), ('R9', 1)],
    'SDA':      [('J5', 2), ('U4', 4), ('U5', 4), ('R11', 1)],
    'SCL':      [('J5', 1), ('U4', 5), ('U5', 5), ('R12', 1)],

    'RS485_A':  [('J8', 1), ('RT1', 1), ('R7', 1), ('X3', 3), ('X4', 3)],
    'RS485_B':  [('J8', 2), ('RT1', 2), ('R8', 1), ('X3', 4), ('X4', 4)],

    'ISO1_3V9': [('U4', 6), ('U6', 4)],
    'ISO1_GND': [('U4', 8), ('U6', 1)],
    'ISO1_A':   [('U4', 9), ('U6', 2)],
    'ISO1_B':   [('U4', 10), ('U6', 3)],
    'PH_PRB':   [('U6', 5), ('J7', 1)],
    'PH_PGND':  [('U6', 6), ('J7', 2)],

    'ISO2_3V9': [('U5', 6), ('U7', 4)],
    'ISO2_GND': [('U5', 8), ('U7', 1)],
    'ISO2_A':   [('U5', 9), ('U7', 2)],
    'ISO2_B':   [('U5', 10), ('U7', 3)],
    'EC_PRB':   [('U7', 5), ('J9', 1)],
    'EC_PGND':  [('U7', 6), ('J9', 2)],
}

names = sorted(NETS)
NUM = dict((n, i + 1) for i, n in enumerate(names))
pad2net = {}
for n, pins in NETS.items():
    for ref, pad in pins:
        pad2net[(ref, str(pad))] = n

s = io.open(PCB, encoding='utf-8').read()

# ---- net declarations -----------------------------------------------------
decl = '\t(net 0 "")\n' + ''.join('\t(net %d "%s")\n' % (NUM[n], n) for n in names)
s = s.replace('\t(net 0 "")\n', decl, 1)

# ---- give every pad its net ----------------------------------------------
hit = [0, 0]

def spans(txt, token):
    """Every (token ...) block, located by counting brackets."""
    out, i = [], 0
    while True:
        i = txt.find(token, i)
        if i < 0:
            return out
        d, j = 0, i
        while j < len(txt):
            if txt[j] == '(':
                d += 1
            elif txt[j] == ')':
                d -= 1
                if d == 0:
                    break
            j += 1
        out.append((i, j + 1))
        i = j + 1

pieces, last = [], 0
for a, b in spans(s, '(footprint "'):
    blk = s[a:b]
    rm = re.search(r'\(property "Reference" "([^"]*)"', blk)
    if rm:
        ref = rm.group(1)
        parts, pl = [], 0
        for pa, pb in spans(blk, '(pad "'):
            pm = re.match(r'\(pad "([^"]*)"', blk[pa:pb])
            net = pad2net.get((ref, pm.group(1)))
            ins = blk[pa:pb]
            if net is not None:
                hit[0] += 1
                k = ins.rfind(')')
                ins = ins[:k] + '\t\t\t(net %d "%s")\n\t\t' % (NUM[net], net) + ins[k:]
            else:
                hit[1] += 1
            parts.append(blk[pl:pa] + ins)
            pl = pb
        blk = ''.join(parts) + blk[pl:]
    pieces.append(s[last:a] + blk)
    last = b
s = ''.join(pieces) + s[last:]
print('pads given a net: %d   pads left unassigned: %d' % (hit[0], hit[1]))

def uid(seed):
    h = hashlib.md5(seed.encode('utf-8')).hexdigest()
    return '%s-%s-%s-%s-%s' % (h[0:8], h[8:12], h[12:16], h[16:20], h[20:32])

def zone(net, layer, x1, y1, x2, y2, prio, seed):
    return ('\t(zone\n\t\t(net %d)\n\t\t(net_name "%s")\n\t\t(layer "%s")\n\t\t(uuid "%s")\n'
            '\t\t(hatch edge 0.5)\n\t\t(priority %d)\n'
            '\t\t(connect_pads\n\t\t\t(clearance 0.3)\n\t\t)\n'
            '\t\t(min_thickness 0.25)\n'
            '\t\t(fill yes\n\t\t\t(thermal_gap 0.5)\n\t\t\t(thermal_bridge_width 0.5)\n'
            '\t\t\t(island_removal_mode 0)\n\t\t)\n'
            '\t\t(polygon\n\t\t\t(pts\n\t\t\t\t(xy %s %s) (xy %s %s) (xy %s %s) (xy %s %s)\n\t\t\t)\n\t\t)\n\t)\n'
            ) % (NUM[net], net, layer, uid(seed), prio, x1, y1, x2, y1, x2, y2, x1, y2)

# board occupies KiCad x 20..120, y 20..120; the slot is at y 82.6..85.0, x 72..116
Z = ''
for lay in ('F.Cu', 'B.Cu'):
    # main ground: everything above the slot, left of the isolated columns
    Z += zone('GND', lay, 21, 21, 119, 67, 10, 'gndT' + lay)
    Z += zone('GND', lay, 21, 67, 72, 119, 11, 'gndB' + lay)
    # the switched-ground island, middle block
    Z += zone('SWGND', lay, 56, 38, 76, 64, 30, 'swg' + lay)
    # the two isolated domains, BELOW each isolator's barrier slot
    Z += zone('ISO1_GND', lay, 78, 72, 94, 119, 40, 'i1' + lay)
    Z += zone('ISO2_GND', lay, 96, 72, 116, 119, 40, 'i2' + lay)

s = s.rstrip()[:-1].rstrip() + '\n' + Z + ')\n'
io.open(PCB, 'w', encoding='utf-8', newline='\n').write(s)
print('4 zones x 2 layers written')

# ---- BOM ------------------------------------------------------------------
PARTS = [
    ('C1', 'CP_Radial_D8.0mm_P3.50mm', '100uF 10V electrolytic', 'hand', ''),
    ('C3,C5,C10', 'C_0603', '100nF X7R 50V', 'SMT', ''),
    ('C4,C8', 'CP_Radial_D10.0mm_P5.00mm', '470uF 25V / 10V electrolytic', 'hand', 'C4 is 25V, C8 is 10V'),
    ('C6,C7', 'C_0603', '1uF X7R 16V', 'SMT', 'required by the MCP1700'),
    ('C9', 'C_0805', '10uF X7R 16V', 'SMT', ''),
    ('D1', 'D_SMA', 'SS34', 'SMT', 'cathode ring faces the load'),
    ('D2', 'D_SMB', 'SMBJ5.0A', 'SMT', 'cathode to +VBATT'),
    ('F1', 'R_1812', 'PTC 2.6A hold', 'SMT', 'resettable'),
    ('JP1', 'R_0603', '0R', 'SMT', 'links 3V3P straight to 3V3REG. Fitted by default; remove it if U2 is ever populated'),
    ('J2,J6', 'TerminalBlock 1x02 P3.50', 'pluggable 3.5mm, 2-way', 'hand', 'to the WaziSense'),
    ('J3,J4', 'TerminalBlock 1x03 P3.50', 'pluggable 3.5mm, 3-way', 'hand', 'to the WaziSense'),
    ('J5', 'TerminalBlock 1x04 P3.50', 'pluggable 3.5mm, 4-way', 'hand', 'to the WaziSense'),
    ('J7,J9', 'SMA_Amphenol_132134-10', 'SMA jack, vertical', 'hand', 'pH and EC electrodes'),
    ('J8', 'TerminalBlock 1x02 P3.50', '2-way', 'hand', 'A/B down from the MAX3485 screw terminal'),
    ('Q1,Q3', 'SOT-23', 'AO3401A', 'SMT', 'P-channel pass element'),
    ('Q2,Q4', 'SOT-23', '2N7002', 'SMT', 'gate level shifter'),
    ('R1,R3,R4,R6', 'R_0603', '100k 1%', 'SMT', 'gate pull-up / pull-down'),
    ('R2,R5', 'R_0603', '10k 1%', 'SMT', 'GPIO series'),
    ('R7,R8', 'R_0603', '680R 1%', 'SMT', 'RS485 fail-safe bias. R8 returns to SWGND'),
    ('R9', 'R_0603', '4k7 1%', 'SMT', '1-Wire pull-up - MANDATORY'),
    ('R11,R12', 'R_0603', '47k 1%', 'DNP', 'isolators carry their own 4k7 both sides'),
    ('RT1', 'R_0603', '120R 1%', 'SMT', 'the only terminator on the bus'),
    ('U1', 'Pololu_U3V9F12', 'Pololu U3V9F12', 'hand', '1=VOUT 2=GND 3=VIN'),
    ('U2', 'SOT-23', 'MCP1700T-3302E/TT', 'DNP', 'alternative to JP1, NOT both. Only useful if 3V3P is ever fed from a higher rail - an MCP1700 cannot regulate 3.3 V out of 3.3 V in'),
    ('U3', 'MAX3485_Module', 'MAX3485 auto-direction module', 'hand', 'stands 53mm tall, fix with adhesive pad'),
    ('U4,U5', 'BE_IVI_Isolator', 'Atlas Basic EZO inline voltage isolator', 'hand', 'OFF pin left open'),
    ('U6,U7', 'EZO_Circuit', 'Atlas EZO-pH / EZO-EC', 'hand', ''),
    ('X1', 'TerminalBlock 1x02 P3.50', '2-way screw', 'hand', '18650 cell'),
    ('X2', 'TerminalBlock 1x03 P3.50', '3-way screw', 'hand', 'DS18B20: 3V3REG / DQ / SWGND'),
    ('X3,X4', 'TerminalBlock 1x04 P3.50', '4-way screw', 'hand', '+12V GND A B, one block per probe'),
    ('H1,H2,H3,H4', 'MountingHole_3.2mm_M3', 'M3', 'mech', ''),
]
rows = ['Designator,Footprint,Value,Assembly,Note']
for r in PARTS:
    rows.append('"%s","%s","%s","%s","%s"' % r)
io.open(BOM, 'w', encoding='utf-8', newline='\n').write('\n'.join(rows) + '\n')
print('BOM.csv written: %d lines' % len(rows))
