# -*- coding: utf-8 -*-
from __future__ import print_function
import pcbnew, math
bd = pcbnew.LoadBoard(r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb')
pcbnew.ZONE_FILLER(bd).Fill(bd.Zones())

def shapes(name):
    out = []
    for z in bd.Zones():
        if z.GetNetname() != name:
            continue
        for lid in z.GetLayerSet().Seq():
            poly = z.GetFilledPolysList(lid)
            for i in range(poly.OutlineCount()):
                o = poly.Outline(i)
                pts = [(pcbnew.ToMM(o.CPoint(k).x), pcbnew.ToMM(o.CPoint(k).y))
                       for k in range(o.PointCount())]
                out.append((bd.GetLayerName(lid), pts))
    return out

G, S = shapes('GND'), shapes('SWGND')
print('GND %d Inseln, SWGND %d Inseln' % (len(G), len(S)))

def seg_d(p, a, b):
    dx, dy = b[0]-a[0], b[1]-a[1]
    L = dx*dx + dy*dy
    t = 0.0 if L == 0 else max(0.0, min(1.0, ((p[0]-a[0])*dx + (p[1]-a[1])*dy)/L))
    return math.hypot(p[0]-a[0]-t*dx, p[1]-a[1]-t*dy)

best, where = 1e9, None
for lg, A in G:
    for ls, B in S:
        if lg != ls:
            continue                      # different layers cannot touch
        for p in B:
            for i in range(len(A)):
                d = seg_d(p, A[i], A[(i+1) % len(A)])
                if d < best:
                    best, where = d, (lg, p)
print('kleinster Abstand GND-Flaeche zu SWGND-Flaeche: %.3f mm auf %s bei (%.2f, %.2f)'
      % (best, where[0], where[1][0], where[1][1]))
