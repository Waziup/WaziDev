# -*- coding: utf-8 -*-
from __future__ import print_function
import pcbnew
from paths import FILLED
bd = pcbnew.LoadBoard(FILLED)
for z in bd.Zones():
    if z.GetNetname() != 'GND':
        continue
    bb = z.GetBoundingBox()
    if pcbnew.ToMM(bb.GetTop()) < 60:
        continue
    for lid in z.GetLayerSet().Seq():
        poly = z.GetFilledPolysList(lid)
        print('%s %s: %d island(s)' % (z.GetNetname(), bd.GetLayerName(lid), poly.OutlineCount()))
        for i in range(poly.OutlineCount()):
            o = poly.Outline(i)
            xs = [pcbnew.ToMM(o.CPoint(k).x) for k in range(o.PointCount())]
            ys = [pcbnew.ToMM(o.CPoint(k).y) for k in range(o.PointCount())]
            print('   x %6.1f..%6.1f  y %6.1f..%6.1f' % (min(xs), max(xs), min(ys), max(ys)))
