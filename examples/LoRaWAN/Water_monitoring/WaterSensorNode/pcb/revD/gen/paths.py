# -*- coding: utf-8 -*-
"""Every path the generator uses, derived from where this file sits.

   The repository can be cloned anywhere: nothing here names a user or a
   drive. Only the KiCad installation is machine-specific, and it can be
   overridden from the environment:

     KICAD_DIR         install root            (default: C:\\Program Files\\KiCad\\10.0)
     KICAD_CLI         kicad-cli executable    (default: <KICAD_DIR>\\bin\\kicad-cli.exe)
     KICAD_FOOTPRINTS  stock footprint library (default: <KICAD_DIR>\\share\\kicad\\footprints)

   Scripts run with `python -I` cannot import this file - isolated mode leaves
   the script's own folder off sys.path - so those compute their paths from
   __file__ themselves, the same way.
"""
import os
import tempfile

GEN = os.path.dirname(os.path.abspath(__file__))
PCBDIR = os.path.normpath(os.path.join(GEN, '..'))

PCB = os.path.join(PCBDIR, 'KijaniCarrier.kicad_pcb')
BOM = os.path.join(PCBDIR, 'BOM.csv')
FAB = os.path.join(PCBDIR, 'out', 'fab')
MYLIB = os.path.join(PCBDIR, 'KijaniSpace.pretty')

# a filled copy of the board, for the diagnostic scripts fill2.py / isl.py
FILLED = os.path.join(tempfile.gettempdir(), 'KijaniCarrier-filled.kicad_pcb')

KICAD_DIR = os.environ.get('KICAD_DIR', r'C:\Program Files\KiCad\10.0')
KICAD_CLI = os.environ.get('KICAD_CLI', os.path.join(KICAD_DIR, 'bin', 'kicad-cli.exe'))
KICAD_FOOTPRINTS = os.environ.get('KICAD_FOOTPRINTS',
                                  os.path.join(KICAD_DIR, 'share', 'kicad', 'footprints'))
