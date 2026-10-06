# -*- coding: utf-8 -*-
from __future__ import print_function
import pcbnew
PCB = r'C:\Users\felix\Claude_temp\WaterSensorNode\pcb\KijaniCarrier.kicad_pcb'
OUT = r'C:\Users\felix\AppData\Local\Temp\claude\C--Users-felix-Claude-temp\3070bea0-d933-40fe-bdde-4ac8c0de3861\scratchpad\filled.kicad_pcb'
bd = pcbnew.LoadBoard(PCB)
pcbnew.ZONE_FILLER(bd).Fill(bd.Zones())
bd.GetConnectivity().RecalculateRatsnest()
print('unconnected after fill:', bd.GetConnectivity().GetUnconnectedCount(True))
pcbnew.SaveBoard(OUT, bd)
print('saved')
