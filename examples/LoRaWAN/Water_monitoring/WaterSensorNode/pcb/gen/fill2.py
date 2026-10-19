# -*- coding: utf-8 -*-
from __future__ import print_function
import pcbnew
from paths import PCB
from paths import FILLED as OUT
bd = pcbnew.LoadBoard(PCB)
pcbnew.ZONE_FILLER(bd).Fill(bd.Zones())
bd.GetConnectivity().RecalculateRatsnest()
print('unconnected after fill:', bd.GetConnectivity().GetUnconnectedCount(True))
pcbnew.SaveBoard(OUT, bd)
print('saved')
