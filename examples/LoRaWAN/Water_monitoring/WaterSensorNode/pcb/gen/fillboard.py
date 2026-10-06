# -*- coding: utf-8 -*-
"""Fill the zones in place with KiCad's own filler and report connectivity.

   This is the last step of the build. Without it the board on disk has empty
   pours, every GND pad reads as unconnected, and the stitching vias read as
   dangling -- none of which is true of the real board.
"""
from __future__ import print_function
import pcbnew

PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
bd = pcbnew.LoadBoard(PCB)
pcbnew.ZONE_FILLER(bd).Fill(bd.Zones())
conn = bd.GetConnectivity()
conn.RecalculateRatsnest()
n = conn.GetUnconnectedCount(True)
print('zones filled, unconnected: %d' % n)
pcbnew.SaveBoard(PCB, bd)
print('saved in place')
