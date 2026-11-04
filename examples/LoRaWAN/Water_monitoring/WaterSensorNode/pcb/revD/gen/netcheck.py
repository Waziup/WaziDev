# -*- coding: utf-8 -*-
"""The requirements that are connections, checked against the board itself.

   The README of Rev C called the netlist check "the one that matters most,
   because no DRC can catch a wrong net" - and it was done by hand. This is
   that check as code, extended for Rev D: the firmware's pins, the two new
   chips pin by pin from their datasheets, the SWGND island, the input
   selector and the probe supply. Every line below is a requirement; a
   failure names it.
"""
from __future__ import print_function
import io, re, sys, codecs
sys.stdout = codecs.getwriter('ascii')(sys.stdout.buffer, 'replace')
from pads import POS
from paths import PCB

net = {}
for (ref, pad), (x, y, n) in POS.items():
    net[(ref, pad.split('#')[0])] = n
members = {}
for (ref, pad), n in net.items():
    members.setdefault(n, set()).add('%s.%s' % (ref, pad))

bad, ok = [], 0


def want(ref_pad, expected, why):
    global ok
    ref, pad = ref_pad.split('.')
    got = net.get((ref, pad))
    if got != expected:
        bad.append('%-8s is %-11s want %-11s  %s' % (ref_pad, got, expected, why))
    else:
        ok += 1


# ---- the firmware's pins (WaterSensorNode.ino) -----------------------------
want('J4.1', 'RS_TX', 'D4 = SoftwareSerial TX')
want('J4.2', 'RS_RX', 'D3 = SoftwareSerial RX')
want('J5B.1', 'D5', 'RAIL12_EN = 5')
want('R2.1', 'D5', 'D5 drives the 12 V switch')
want('J3.2', 'A1', 'ISO_EN = A1')
want('R5.1', 'A1', 'A1 drives the isolator switch')
want('J3.1', 'OW_DATA', 'ONE_WIRE_PIN = A2')
want('J5A.1', 'SCL', 'I2C')
want('J5A.2', 'SDA', 'I2C')
want('J2.1', '3V3P', 'D6(+), permanently live')
want('J2.2', 'SWGND', 'D6(-), the switched return')
want('J6.1', '+VBATT', 'WaziSense battery input')

# ---- THVD1406, SLLSF87A table 5-1 ------------------------------------------
for pad, n, why in (('1', 'RS_RX', 'R -> D3'), ('2', '3V3RS', 'RE high: auto-direction'),
                    ('3', '3V3RS', 'SHDN high: not shut down'), ('4', 'RS_TX', 'D <- D4'),
                    ('5', 'GND', 'GND, the probes\' reference'), ('6', 'RS485_A', 'A'),
                    ('7', 'RS485_B', 'B'), ('8', '3V3RS', 'VCC 3.0-5.5 V, switched')):
    want('U3.' + pad, n, 'THVD1406 ' + why)
want('R7.1', 'RS485_A', 'fail-safe pull-up on A')
want('R7.2', '3V3RS', 'fail-safe pull-up on A, switched: no bias current in sleep')
want('R8.1', 'RS485_B', 'fail-safe pull-down on B')
want('R8.2', 'GND', 'pull-down to GND: the bus rests at 0 V in sleep')
# ---- the sleep rule (Rev C measured 0.5 mA per probe without it): nothing
#      may hold the bus away from GND while D6 is off. The bus is RS485_A/B
#      plus whatever drives it; the only supply allowed near it is 3V3RS,
#      and 3V3RS may only come from Q6, which only SWGND can turn on.
want('Q6.1', 'SWGND', 'Q6 gate: on only while D6 pulls SWGND low')
want('Q6.2', '3V3REG', 'Q6 source')
want('Q6.3', '3V3RS', 'Q6 drain: the switched rail')
want('R17.2', 'SWGND', 'R17: gate held at the source while D6 is off')
want('R17.1', '3V3REG', 'R17: gate held at the source while D6 is off')
rs = members.get('3V3RS', set())
if rs != {'Q6.3', 'U3.8', 'U3.2', 'U3.3', 'C11.1', 'R7.2'}:
    bad.append('3V3RS is %s: only the transceiver, its bias and C11 belong on it'
               % sorted(rs))
else:
    ok += 1
for bus in ('RS485_A', 'RS485_B'):
    other = [p for p in members.get(bus, ()) if p.split('.')[0] not in
             ('U3', 'RT1', 'R7', 'R8', 'X3B', 'X4B', 'D3')]
    if other:
        bad.append('%s reaches %s: nothing else may bias the bus' % (bus, other))
    else:
        ok += 1
want('RT1.1', 'RS485_A', 'the only terminator')
want('RT1.2', 'RS485_B', 'the only terminator')
want('D3.1', 'RS485_B', 'SM712 line')
want('D3.2', 'RS485_A', 'SM712 line')
want('D3.3', 'GND', 'SM712 common')
want('C11.1', '3V3RS', '100 nF at VCC')
want('C11.2', 'GND', '100 nF at VCC')
for t in ('X3B', 'X4B'):
    want(t + '.1', 'RS485_A', 'probe terminal A')
    want(t + '.2', 'RS485_B', 'probe terminal B')

# ---- TPS61085, SLVS859B pin functions ---------------------------------------
for pad, n, why in (('1', 'BOOST_COMP', 'COMP'), ('2', 'BOOST_FB', 'FB'),
                    ('3', 'BOOST_IN', 'EN tied to IN'), ('4', 'GND', 'PGND'),
                    ('5', 'BOOST_SW', 'SW'), ('6', 'BOOST_IN', 'IN'),
                    ('7', 'BOOST_IN', 'FREQ high: 1.2 MHz'), ('8', 'BOOST_SS', 'SS')):
    want('U8.' + pad, n, 'TPS61085 ' + why)
want('L1.1', 'BOOST_SW', 'inductor, switch side')
want('L1.2', 'BOOST_IN', 'inductor, input side')
want('D4.1', '+12V', 'SS14 cathode (KiCad pad 1) to the output')
want('D4.2', 'BOOST_SW', 'SS14 anode on the switch node')
want('R15.2', '+12V', 'divider top')
want('R15.1', 'BOOST_FB', 'divider top')
want('R16.2', 'BOOST_FB', 'divider bottom')
want('R16.1', 'GND', 'divider bottom')
want('R14.1', 'BOOST_COMP', 'compensation R')
want('C17.1', 'BOOST_RC', 'compensation C')
want('C16.1', 'BOOST_SS', 'soft start')
for c in ('C14', 'C15'):
    want(c + '.1', '+12V', 'output capacitor')
    want(c + '.2', 'GND', 'output capacitor')
want('C12.2', 'BOOST_IN', 'input capacitor')
want('C13.1', 'BOOST_IN', 'IN bypass')

# ---- the selector: on-board by default, Pololu only after cut-and-bridge ----
want('JP3.1', 'BOOST_IN', 'pad 1 -> on-board boost')
want('JP3.2', 'V12IN', 'centre <- Q1')
want('JP3.3', 'POLOLU_VIN', 'pad 3 -> Pololu')
want('Q1.3', 'V12IN', 'Q1 drain')
want('U1.3', 'POLOLU_VIN', 'Pololu VIN only through JP3')
want('U1.1', '+12V', 'Pololu VOUT')
want('U1.2', 'GND', 'Pololu GND')
s = io.open(PCB, encoding='utf-8').read()
m = re.search(r'\(property "Reference" "JP3"', s)
a = s.rfind('(footprint', 0, m.start())
b = s.find('\n\t(footprint', m.start())
# the generator writes "1,2", pcbnew saves it back as "1, 2"
if not re.search(r'\(net_tie_pad_groups "1,\s*2"\)', s[a:b]):
    bad.append('JP3 is not the bridged-1-2 net tie: the default would be open')
else:
    ok += 1

# ---- the probe supply --------------------------------------------------------
for t in ('X3A', 'X4A'):
    want(t + '.1', '+12V', 'probe terminal +12V')
    want(t + '.2', 'GND', 'probe terminal GND')
vout = 1.238 * (1 + 200.0 / 22.0)
if not 12.0 <= vout <= 13.0:
    bad.append('12 V rail would be %.2f V: Y511-A needs >= 12 V' % vout)
else:
    ok += 1

# ---- the islands -------------------------------------------------------------
expect_sw = {'J2.2', 'X2.3', 'U4.3', 'U5.3', 'Q6.1', 'R17.2'}
if members.get('SWGND') != expect_sw:
    bad.append('SWGND is %s, want exactly %s (trap 1: one exit, J2.2)'
               % (sorted(members.get('SWGND', ())), sorted(expect_sw)))
else:
    ok += 1
for n, mem in sorted(members.items()):
    # ISO_VIN is the isolators' supply on the MCU side, not an isolated net
    if n.startswith(('ISO1_', 'ISO2_', 'PH_', 'EC_')):
        foreign = [p for p in mem if p.split('.')[0] not in ('U4', 'U5', 'U6', 'U7', 'J7', 'J9')]
        if foreign:
            bad.append('isolated net %s reaches %s' % (n, foreign))
        else:
            ok += 1
    if n and len(mem) < 2:
        bad.append('net %s has a single pad: %s' % (n, sorted(mem)))

print('netcheck: %d requirements hold' % ok)
if bad:
    print('%d FAIL:' % len(bad))
    for b in bad:
        print('  ' + b)
    sys.exit(1)
