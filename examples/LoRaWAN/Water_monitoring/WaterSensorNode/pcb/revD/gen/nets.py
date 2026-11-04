# -*- coding: utf-8 -*-
"""The netlist, written straight into the board, plus the four copper zones."""

from __future__ import print_function

import sys as _sys
_GENERATOR_GUARD = True
if __name__ != '__main__':
    raise ImportError('nets.py rewrites the board file; run it, do not import it')
import io, re, hashlib

from paths import PCB
from paths import BOM

# SOT-23 here is 1=Gate 2=Source 3=Drain; MCP1700 is 1=GND 2=VIN 3=VOUT;
# diodes 1=cathode 2=anode; SMA 1=centre 2=shield.
NETS = {
    'VB1':      [('X1', 1), ('F1', 1)],
    'VB2':      [('F1', 2), ('Q5', 3)],          # Q5.3 = drain, cell side
    '+VBATT':   [('Q5', 2), ('D2', 1), ('C1', 1), ('Q1', 2), ('Q3', 2),
                 ('R1', 2), ('C3', 2), ('R4', 2), ('C5', 2), ('J6', 1)],
    'GND':      [('X1', 2), ('D2', 2), ('C1', 2), ('Q2', 2), ('Q4', 2),
                 ('R3', 2), ('R6', 2), ('R13', 2), ('J6', 2), ('U1', 2), ('C4', 2),
                 ('U2', 1), ('C6', 2), ('C7', 2), ('C8', 2), ('C9', 2), ('C10', 2),
                 ('X3A', 2), ('X4A', 2), ('J3', 3), ('J4', 3), ('J5B', 2),
                 # Rev D: the boost's grounds and the RS485 TVS
                 ('U8', 4), ('C12', 1), ('C13', 2), ('C14', 2), ('C15', 2),
                 ('C16', 2), ('C17', 2), ('R16', 1), ('D3', 3),
                 # the transceiver and its bias are GND-referenced since
                 # 4 Nov 2026 (see 3V3RS)
                 ('U3', 5), ('C11', 2), ('R8', 2)],

    # SWGND also drives Q6's gate; R17 holds it at 3V3REG once D6 lets go.
    'SWGND':    [('J2', 2), ('X2', 3), ('U4', 3), ('U5', 3), ('Q6', 1), ('R17', 2)],
    '3V3P':     [('J2', 1), ('U2', 2), ('C6', 1), ('JP1', 1)],
    '3V3REG':   [('U2', 3), ('JP1', 2), ('C7', 1), ('C8', 1), ('C9', 1), ('C10', 1),
                 ('R9', 2), ('R11', 2), ('R12', 2), ('X2', 1),
                 ('Q6', 2), ('R17', 1)],
    # The RS485 side, switched. Rev C ran the transceiver between 3V3REG and
    # SWGND: in sleep SWGND floated up through the bias to 3.3 V, and so did
    # the whole bus - straight into the unpowered probes' receiver inputs,
    # which sit on GND. 0.53 mA sleep current with one probe on the bus,
    # 0.03 mA with its A/B wires out (Rev C, 4 Nov 2026). Now Q6 cuts the
    # supply and the bias pulls A and B to GND, where the probes are.
    '3V3RS':    [('Q6', 3), ('U3', 8), ('C11', 1), ('R7', 2),
                 # RE and SHDN high: the THVD1406's auto-direction mode
                 ('U3', 2), ('U3', 3)],

    'D5':       [('J5B', 1), ('R2', 1)],
    'Q2_G':     [('R2', 2), ('Q2', 1), ('R3', 1)],
    # The ideal diode's gate sits at GND through R13, so V_GS is -V_batt
    # whenever the cell is the right way round and 0 when it is not.
    'Q5_G':     [('Q5', 1), ('R13', 1)],
    'Q1_G':     [('Q2', 3), ('R1', 1), ('C3', 1), ('Q1', 1)],
    # Rev D: Q1's switched cell voltage goes to JP3, which hands it to the
    # on-board boost (1-2, the default) or to the Pololu socket (2-3), never
    # to both.
    'V12IN':    [('Q1', 3), ('JP3', 2)],
    'POLOLU_VIN': [('JP3', 3), ('U1', 3)],
    'BOOST_IN': [('JP3', 1), ('U8', 6), ('U8', 7), ('U8', 3), ('L1', 2), ('C12', 2), ('C13', 1)],
    'BOOST_SW': [('U8', 5), ('L1', 1), ('D4', 2)],
    'BOOST_FB': [('U8', 2), ('R15', 1), ('R16', 2)],
    'BOOST_COMP': [('U8', 1), ('R14', 1)],
    'BOOST_RC': [('R14', 2), ('C17', 1)],
    'BOOST_SS': [('U8', 8), ('C16', 1)],
    '+12V':     [('U1', 1), ('C4', 1), ('X3A', 1), ('X4A', 1),
                 ('D4', 1), ('C14', 1), ('C15', 1), ('R15', 2)],

    'A1':       [('J3', 2), ('R5', 1)],
    'Q4_G':     [('R5', 2), ('Q4', 1), ('R6', 1)],
    'Q3_G':     [('Q4', 3), ('R4', 1), ('C5', 1), ('Q3', 1)],
    'ISO_VIN':  [('Q3', 3), ('U4', 1), ('U5', 1)],

    'RS_TX':    [('J4', 1), ('U3', 4)],      # THVD1406 D
    'RS_RX':    [('J4', 2), ('U3', 1)],      # THVD1406 R
    'OW_DATA':  [('J3', 1), ('X2', 2), ('R9', 1)],
    'SDA':      [('J5A', 2), ('U4', 4), ('U5', 4), ('R11', 1)],
    'SCL':      [('J5A', 1), ('U4', 5), ('U5', 5), ('R12', 1)],

    'RS485_A':  [('U3', 6), ('RT1', 1), ('R7', 1), ('X3B', 1), ('X4B', 1), ('D3', 2)],
    'RS485_B':  [('U3', 7), ('RT1', 2), ('R8', 1), ('X3B', 2), ('X4B', 2), ('D3', 1)],

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
    # Rev D: no SWGND island any more. The transceiver block it served is on
    # GND now; what is left on SWGND (isolators, DS18B20, Q6's gate) is wired.
    # the two isolated domains, BELOW each isolator's barrier slot
    Z += zone('ISO1_GND', lay, 78, 72, 94, 119, 40, 'i1' + lay)
    Z += zone('ISO2_GND', lay, 96, 72, 116, 119, 40, 'i2' + lay)

s = s.rstrip()[:-1].rstrip() + '\n' + Z + ')\n'
io.open(PCB, 'w', encoding='utf-8', newline='\n').write(s)
print('%d zones x 2 layers written' % (Z.count('(zone') // 2))

# ---- BOM ------------------------------------------------------------------
PARTS = [
    ('C1', 'CP_Radial_D5.0mm_P2.00mm', '100uF 16V electrolytic', 'THT', 'ERG16V100M5X11, LCSC C47344171 - D5 x L11 mm, 2.00 mm pitch. The footprint follows the part JLCPCB stocks: 89k in stock. A D8/3.50 footprint would not take it, the leads are on the wrong pitch.'),
    ('C3,C5,C10,C11', 'C_0603', '100nF X7R 50V', 'SMT', 'C11 decouples the RS485 transceiver'),
    ('C4,C8', 'CP_Radial_D10.0mm_P5.00mm', '470uF 25V electrolytic', 'THT', 'ERA25V470M10X13, LCSC C49256798 - D10 x L13 mm, 5.00 mm pitch, 21k in stock. KEEP THE 5.00 mm PITCH: the +VBATT and ISO_VIN runs pass BETWEEN this cap/s two pads, and a 3.50 mm pitch leaves them nowhere to go. One 25 V part serves both positions; C8 only needs 10 V.'),
    ('C6,C7,C13,C16', 'C_0603', '1uF X5R 50V', 'SMT', 'C6/C7 required by the MCP1700; C13 is the TPS61085 IN bypass, as close to the pin as it goes; C16 is its soft start - 1 uF, not the 100 nF of the TI example, because the output also carries C4 (470 uF): the current limit then ramps over 50 ms instead of 5, and the cell sees about 1.1 A instead of 2.4 A while C4 charges (averaged start-up model, 4 Nov 2026)'),
    ('C9,C12', 'C_0805', '10uF X5R 25V', 'SMT', 'C12 is the boost input capacitor'),
    ('C14,C15', 'C_1206', '10uF X5R 50V', 'SMT', 'boost output, two 10 uF as in the TI example. 50 V rating so that 12.5 V DC bias still leaves real capacitance'),
    ('C17', 'C_0603', '1.5nF X7R 50V', 'SMT', 'boost compensation with R14. TI table: 47k / 1.6 nF for 3.3 uH, 12 V, 1.2 MHz'),
    ('U8', 'MSOP-8', 'TPS61085DGKR', 'SMT', '12 V boost, TI datasheet example 3.3 V -> 12 V at 1.2 MHz (FREQ and EN tied to IN). Behind the Q1 high-side switch, so it has no supply at all in sleep'),
    ('L1', 'L_Sunlord_SWPA4030S', '3.3uH SWPA4030S3R3MT', 'SMT', 'Isat 3.6 A - above the TPS61085 switch limit of 3.2 A, the conservative choice in the datasheet'),
    ('D4', 'D_SMA', 'SS14', 'SMT', 'boost rectifier, 40 V 1 A. Cathode to +12V. Also blocks the Pololu fallback from back-feeding the dead converter'),
    ('R14', 'R_0603', '47k 1%', 'SMT', 'boost compensation'),
    ('R15', 'R_0603', '200k 1%', 'SMT', 'boost feedback, top. 1.238 V x (1 + 200k/22k) = 12.49 V'),
    ('R16', 'R_0603', '22k 1%', 'SMT', 'boost feedback, bottom. 56 uA through the divider, above the 50 uA TI asks for'),
    ('U3', 'SOIC-8', 'THVD1406DR', 'SMT', 'RS485 transceiver, 3.0-5.5 V, auto-direction from D. RE and SHDN tied to VCC. Replaces the MAX13487 plug-in module, which ran outside its 4.75-5.25 V rating and whose TXD/RXD labels differ between batches'),
    ('D3', 'SOT-23', 'SM712', 'SMT', 'RS485 TVS, -7/+12 V asymmetric, A and B to GND at the probe terminals'),
    ('JP3', 'SolderJumper-3 bridged 1-2', 'copper', 'mech', 'boost select. 1-2 bridged by a trace = on-board TPS61085 (default). Pololu fallback: cut the 1-2 trace, solder 2-3, plug the U3V9F12 into U1. Never both'),
    ('Q5', 'SOT-23', 'AO3401A', 'SMT', 'IDEAL DIODE - replaces the SS34 that used to be D1. Drain (pin 3) faces F1 and the cell, source (pin 2) faces the load. Fitted the other way round it loses the reverse-polarity protection entirely and the board still appears to work.'),
    ('R13', 'R_0603', '100k 1%', 'SMT', 'gate pull-down for Q5. Without it the ideal diode never turns on and the whole node runs through the body diode at 0.8 V - which looks like a flat cell.'),
    ('D2', 'D_SMB', 'SMBJ5.0A', 'SMT', 'cathode to +VBATT'),
    ('F1', 'R_1812', 'PTC 2.6A hold', 'SMT', 'resettable'),
    ('JP1', 'R_0603', '0R', 'SMT', 'links 3V3P straight to 3V3REG. Fitted by default; remove it if U2 is ever populated'),
    ('J2,J6', 'TerminalBlock 1x02 P3.50', 'screw terminal 2P, 3.50 mm (KF350-3.5-2P)', 'THT',
     'to the WaziSense SW_1 and BAT_IN. pitch 3.50 mm EXACTLY (not 3.81, not 5.08), THT, pin <=1.0 mm, body depth <=9.5 mm - measured on the finished board the tightest terminal neighbour is J2 to J3 at 1.50 mm courtyard to courtyard, X1 to C1 is 2.00 mm'),
    ('J3,J4', 'TerminalBlock 1x03 P3.50', 'screw terminal 3P, 3.50 mm (KF350-3.5-3P)', 'THT',
     'to the WaziSense ANALOG and DIGITAL. pitch 3.50 mm EXACTLY (not 3.81, not 5.08), THT, pin <=1.0 mm, body depth <=9.5 mm - measured on the finished board the tightest terminal neighbour is J2 to J3 at 1.50 mm courtyard to courtyard, X1 to C1 is 2.00 mm'),
    ('J5A,J5B', 'TerminalBlock 1x02 P3.50', 'screw terminal 2P, 3.50 mm (KF350-3.5-2P)', 'THT',
     'to the WaziSense I2C_PORT, J5A = SCL SDA, J5B = D5 GND. TWO 2-POLE BLOCKS GANGED, 7.00 mm apart: they dovetail together and their four pads land on the 3.50 mm grid the 4-pole part used, so not one track moved. JLCPCB stocks no 4-pole at this pitch - that is the whole reason. pitch 3.50 mm EXACTLY (not 3.81, not 5.08), THT, pin <=1.0 mm, body depth <=9.5 mm - measured on the finished board the tightest terminal neighbour is J2 to J3 at 1.50 mm courtyard to courtyard, X1 to C1 is 2.00 mm'),
    ('J7,J9', 'SMA_Vertical_5pin_P5.08', 'SMA jack, vertical','THT', 'pH and EC electrodes. BWSMA-KE-Z001 - the Amphenol 132134 fits the same holes but costs nine times as much, and its 18 GHz rating is wasted on an electrode'),
    ('Q1,Q3,Q6', 'SOT-23', 'AO3401A', 'SMT', 'P-channel pass element. Q6 switches the RS485 transceiver and its bias (3V3RS), gate on SWGND'),
    ('Q2,Q4', 'SOT-23', '2N7002', 'SMT', 'gate level shifter'),
    ('R1,R3,R4,R6,R17', 'R_0603', '100k 1%', 'SMT', 'gate pull-up / pull-down. R17 holds Q6 off while D6 is off'),
    ('R2,R5', 'R_0603', '10k 1%', 'SMT', 'GPIO series'),
    ('R7,R8', 'R_0603', '470R 1%', 'SMT', 'RS485 fail-safe bias, 470R for the on-board transceiver (Rev D). R7 to the switched 3V3RS, R8 to GND: in sleep both lines sit at 0 V'),
    ('R9', 'R_0603', '4k7 1%', 'SMT', '1-Wire pull-up - MANDATORY'),
    ('R11,R12', 'R_0603', '47k 1%', 'DNP', 'isolators carry their own 4k7 both sides'),
    ('RT1', 'R_0603', '120R 1%', 'SMT',
     'FITTED - it is the only terminator in the system. The probes bring none of their own (the Rev C module carried its own 120R on an open jumper). With the 470R bias (Rev D) idle A-B is about 0.37 V, over the 0.2 V a receiver needs. Should a probe ever be added that terminates, the pair gives 60R and the bias falls to about 0.2 V: then change R7/R8 to 330R'),
    ('U1', 'PinSocket_1x03_P2.54mm', 'female header 1x3, 2.54 mm', 'THT',
     'socket for the FALLBACK only - the on-board TPS61085 normally makes 12 V. The Pololu U3V9F12 plugs in here: 1=VOUT 2=GND 3=VIN, and gets its input only once JP3 is moved to 2-3'),
    ('U2', 'SOT-23', 'MCP1700T-3302E/TT', 'DNP', 'alternative to JP1, NOT both. Only useful if 3V3P is ever fed from a higher rail - an MCP1700 cannot regulate 3.3 V out of 3.3 V in'),
    ('U4A,U4B,U5A,U5B', 'PinSocket_1x05_P2.54mm', 'female header 1x5, 2.54 mm', 'THT',
     'socket, two rows per isolator 17.78 mm apart. NOT a board designator - the board has one footprint per module, U4 and U5, and these name its two rows so the assembler gets one part per line. The BE-IVI isolator plugs in, OFF pin left open'),
    ('U6A,U6B,U7A,U7B', 'PinSocket_1x03_P2.54mm', 'female header 1x3, 2.54 mm', 'THT',
     'socket, two rows per EZO circuit 17.78 mm apart. NOT a board designator - see U4A. The Atlas EZO-pH and EZO-EC plug in'),
    ('X1', 'TerminalBlock 1x02 P3.50', 'screw terminal 2P, 3.50 mm (KF350-3.5-2P)', 'THT',
     '18650 cell, + on the left. pitch 3.50 mm EXACTLY (not 3.81, not 5.08), THT, pin <=1.0 mm, body depth <=9.5 mm - measured on the finished board the tightest terminal neighbour is J2 to J3 at 1.50 mm courtyard to courtyard, X1 to C1 is 2.00 mm'),
    ('X2', 'TerminalBlock 1x03 P3.50', 'screw terminal 3P, 3.50 mm (KF350-3.5-3P)', 'THT',
     'DS18B20: 3V3REG / DQ / SWGND. pitch 3.50 mm EXACTLY (not 3.81, not 5.08), THT, pin <=1.0 mm, body depth <=9.5 mm - measured on the finished board the tightest terminal neighbour is J2 to J3 at 1.50 mm courtyard to courtyard, X1 to C1 is 2.00 mm'),
    ('X3A,X3B,X4A,X4B', 'TerminalBlock 1x02 P3.50', 'screw terminal 2P, 3.50 mm (KF350-3.5-2P)', 'THT',
     '+12V GND on the A block, RS485 A/B on the B block, one pair per probe. TWO 2-POLE BLOCKS GANGED, 7.00 mm apart: they dovetail together and their four pads land on the 3.50 mm grid the 4-pole part used, so not one track moved. JLCPCB stocks no 4-pole at this pitch - that is the whole reason. pitch 3.50 mm EXACTLY (not 3.81, not 5.08), THT, pin <=1.0 mm, body depth <=9.5 mm - measured on the finished board the tightest terminal neighbour is J2 to J3 at 1.50 mm courtyard to courtyard, X1 to C1 is 2.00 mm'),
    ('H1,H2,H3,H4', 'MountingHole_3.2mm_M3', 'M3', 'mech', ''),
    # The module positions are bare plated holes - nothing is fitted there
    # unless you fit it. The modules themselves carry male pins, so the
    # board side takes sockets.
]
#  ####################################################################
#  ##  LCSC PART NUMBERS - read off JLCPCB's own stock listing on   ##
#  ##  12 Oct 2026, not guessed. An empty entry means DNP or        ##
#  ##  mechanical - never that nobody looked.                       ##
#  ##                                                               ##
#  ##  The screw terminals ARE stocked: KF350-3.5, 2P C474892 and   ##
#  ##  3P C474893. An earlier note here said 3.50 mm was consign-   ##
#  ##  only; that came from searching KF128, one brand that happens ##
#  ##  not to carry this pitch, and it was wrong. The board keeps   ##
#  ##  its 3.50 mm footprints.                                      ##
#  ####################################################################
LCSC = {
    # designator group            LCSC        stock     what it is
    'C1':                        'C47344171',   # 89035  ERG16V100M5X11, 100uF 16V D5x11
    'C3,C5,C10,C11':             'C14663',      #        100nF X7R 0603, Basic
    'C4,C8':                     'C49256798',   # 21338  ERA25V470M10X13, 470uF 25V D10x13
    'C6,C7,C13,C16':             'C15849',      #        1uF X7R 0603, Basic
    'C9,C12':                    'C15850',      #        10uF X7R 0805, Basic
    #  Rev D additions, read off JLCPCB's stock listing on 4 Nov 2026
    'C14,C15':                   'C13585',      # 3.98M  CL31A106KBHNNNE 10uF 50V 1206, Basic
    'C17':                       'C1595',       # 182k   1.5nF X7R 0603
    'U8':                        'C113659',     # 26333  TPS61085DGKR
    'L1':                        'C15269',      # 21889  SWPA4030S3R3MT 3.3uH
    'D4':                        'C2480',       # 1.71M  SS14 SMA, Basic
    'R14':                       'C25819',      # 2.64M  47k 1% 0603, Basic
    'R15':                       'C25811',      # 2.06M  200k 1% 0603, Basic
    'R16':                       'C31850',      # 2.78M  22k 1% 0603, Basic
    'U3':                        'C5215918',    # 5827   THVD1406DR SOIC-8
    'D3':                        'C293966',     # 121k   SM712 SOT-23 (DOWO)
    'Q5':                        'C15127',      #        AO3401A, Basic - the ideal diode
    'R13':                       'C25803',      #        100k 1% 0603, Basic - its gate pulldown
    'D2':                        'C19077558',   #        SMBJ5.0A TVS
    'F1':                        'C70129',      # 7204   mSMD260-16V, 2.6A hold 16V
    'JP1':                       'C21189',      # 24.9M  0R 0603, Basic
    'J7,J9':                     'C496549',     # 147774 BWSMA-KE-Z001 SMA jack
    #  Was C3174425, Amphenol 132134, at EUR 3.58 each - more than the rest of
    #  the board's components and the bare PCB put together. Replaced 14 Oct
    #  2026 after checking the footprint against the board rather than the
    #  datasheet: ground posts on a 5.10 mm square needing 1.50 mm holes
    #  against the board's 5.08 mm and 1.70 mm, centre pin needing 1.35 mm
    #  against the board's 1.50 mm. Every hole on the board is the larger one,
    #  so it drops in. A second candidate, C504007, was rejected: its centre
    #  pin wants 1.60 mm and the board has 1.50 mm.
    'Q1,Q3,Q6':                  'C15127',      #        AO3401A, Basic
    'Q2,Q4':                     'C8545',       #        2N7002, Basic
    'R1,R3,R4,R6,R17':           'C25803',      # 20.5M  100k 1% 0603, Basic
    'R2,R5':                     'C25804',      # 32.8M  10k 1% 0603, Basic
    'R7,R8':                     'C23179',      # 4.29M  470R 1% 0603, Basic (checked 4 Nov 2026)
    'R9':                        'C23162',      # 20.6M  4k7 1% 0603, Basic
    'RT1':                       'C22787',      # 2.35M  120R 1% 0603, Basic
    'U1':                        'C146243',     # 31180  2.54-1*3PFemale
    'U4A,U4B,U5A,U5B':           'C50950',      # 60888  2.54-1*5PFemale
    'U6A,U6B,U7A,U7B':           'C146243',     # 31180  2.54-1*3PFemale
    # the terminals - one 2P number covers ten blocks on both edges
    'J2,J6':                     'C474892',     # 13420  KF350-3.5-2P
    'X1':                        'C474892',     # 13420  KF350-3.5-2P
    'J5A,J5B':                   'C474892',     # 13420  KF350-3.5-2P, ganged pair
    'X3A,X3B,X4A,X4B':           'C474892',     # 13420  KF350-3.5-2P, two ganged pairs
    'J3,J4':                     'C474893',     #  3707  KF350-3.5-3P
    'X2':                        'C474893',     #  3707  KF350-3.5-3P
}


#  Basic or Extended, off the same stock listing as the part numbers.
#
#  WHAT IT COSTS. The feeder loading fee of about 3 USD per Extended TYPE is
#  charged ONCE PER ORDER, not per board, and it buys a feeder on a
#  pick-and-place machine - so it applies to SMT only. Through-hole parts are
#  hand soldered and charged differently: 3.50 USD once plus 0.0173 USD a
#  joint. Rev D has 7 Extended SMT types (Rev C: 2 - the transceiver, the
#  boost IC, its inductor, its 1.5 nF and the RS485 TVS are new) and 78
#  through-hole joints (Rev C: 84 - J8 and the RS485 socket are gone):
#
#      7 x 3.00  feeders           21.00
#      3.50 + 78 x 0.0173  THT      4.85
#                                  -----
#                                  25.85 USD, once, for the whole order
#
#  Against that, each board saves the Pololu (about 10 EUR) and the RS485
#  module. From the second assembled board on, Rev D is the cheaper one.
#
#  WHY NOTHING MORE IS BASIC. Checked against JLCPCB's Basic & Promotional
#  Extended library (1586 parts) on 12 Oct 2026, with a control search each
#  time so a zero means absence and not a broken query. That library has no
#  Connectors category at all, and no hit for electrolytic, aluminum, TVS,
#  fuse or header. Every Extended line here is a class it simply does not
#  carry; none of them is a part that was picked carelessly.
LIB = {
    'C14663':    'Basic',     # 100nF 0603
    'C15849':    'Basic',     # 1uF 0603
    'C15850':    'Basic',     # 10uF 0805
    'C21189':    'Basic',     # 0R 0603
    'C15127':    'Basic',     # AO3401A
    'C8545':     'Basic',     # 2N7002
    'C25803':    'Basic',     # 100k 0603
    'C25804':    'Basic',     # 10k 0603
    'C23179':    'Basic',     # 470R 0603
    'C23162':    'Basic',     # 4k7 0603
    'C22787':    'Basic',     # 120R 0603
    'C13585':    'Basic',     # 10uF 50V 1206
    'C2480':     'Basic',     # SS14
    'C25819':    'Basic',     # 47k 0603
    'C25811':    'Basic',     # 200k 0603
    'C31850':    'Basic',     # 22k 0603
    'C1595':     'Extended',  # 1.5nF 0603
    'C113659':   'Extended',  # TPS61085DGKR
    'C15269':    'Extended',  # SWPA4030S3R3MT
    'C5215918':  'Extended',  # THVD1406DR
    'C293966':   'Extended',  # SM712
    'C19077558': 'Extended',  # SMBJ5.0A TVS
    'C70129':    'Extended',  # mSMD260-16V PPTC
    'C47344171': 'Extended',  # 100uF 16V leaded
    'C49256798': 'Extended',  # 470uF 25V leaded
    'C496549':   'Extended',  # BWSMA-KE-Z001 SMA jack
    'C146243':   'Extended',  # 1x3 female header
    'C50950':    'Extended',  # 1x5 female header
    'C474892':   'Extended',  # KF350-3.5-2P
    'C474893':   'Extended',  # KF350-3.5-3P
}


#  Unit price in EUR, from JLCPCB's own quote of 14 Oct 2026 (5-board
#  Economic run), divided by the piece count that quote listed - so it
#  already carries the assembler's spares. NOT the LCSC catalogue price;
#  the two differ and LCSC's stock is retail stock, not assembly stock.
#
#  An empty entry means that line came back with quantity 0 and no price.
#  Every through-hole line needing 15 pieces or more did. That is an open
#  question with JLCPCB, not a fact about the part - their LCSC list prices
#  are noted alongside so the figure is not lost.
PRICE = {
    'C47344171': 0.0257,   'C49256798': 0.0851,   'C15849':  0.0141,
    'C15850':    0.0614,   'C14663':    0.0125,   'C19077558': 0.0419,
    'C70129':    0.0633,   'C21189':    0.0020,   'C15127':  0.0909,
    'C8545':     0.0156,   'C25803':    0.0028,   'C25804':  0.0046,
    'C23179':    0.0018,   'C23162':    0.0025,   'C22787':  0.0017,
    #  Rev D additions: JLCPCB list price for a single piece in USD on 4 Nov
    #  2026, taken 1:1 as EUR - not yet quoted. None = not read off.
    'C113659':   0.55,     'C5215918':  1.14,     'C15269':  0.0612,
    'C2480':     0.0192,   'C1595':     0.0038,   'C25819':  0.0019,
    'C25811':    0.0021,   'C31850':    0.0042,
    'C13585':    None,     'C293966':   None,
    'C496549':   0.3516,   # LCSC list - not yet quoted, the swap is new
    # quantity 0 in the quote, LCSC list price for reference:
    'C474892':   None,     # 0.0713 LCSC
    'C474893':   None,     # 0.1069 LCSC
    'C146243':   None,     # 0.0445 LCSC
    'C50950':    None,     # 0.0457 LCSC
}

rows = ['Designator,Footprint,Value,Assembly,LCSC,Lib,UnitEUR,PerBoardEUR,Note']
total = 0.0
for r in PARTS:
    code = LCSC.get(r[0], '')
    unit = PRICE.get(code)
    n = len([d for d in r[0].split(',') if d.strip()])
    per = (unit * n) if unit else None
    if per:
        total += per
    rows.append('"%s","%s","%s","%s","%s","%s","%s","%s","%s"'
                % (r[0], r[1], r[2], r[3], code, LIB.get(code, ''),
                   ('%.4f' % unit) if unit else '',
                   ('%.4f' % per) if per else '',
                   r[4]))
print('Bauteilkosten je Board, soweit bepreist: EUR %.2f' % total)
io.open(BOM, 'w', encoding='utf-8', newline='\n').write('\n'.join(rows) + '\n')
print('BOM.csv written: %d lines' % len(rows))
