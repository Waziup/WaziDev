# License

This project combines hardware and software, and each is licensed on its own terms.

| Part | Files | License |
|---|---|---|
| **Hardware** | everything in [`pcb/`](pcb/): board, footprints, the generator scripts that produce the design, fabrication output | [CERN Open Hardware Licence Version 2 – Strongly Reciprocal](LICENSES/CERN-OHL-S-2.0.txt) (`CERN-OHL-S-2.0`) |
| **Hardware documentation** | [`docs/`](docs/), including the drawings in `docs/img/` | [CERN-OHL-S-2.0](LICENSES/CERN-OHL-S-2.0.txt) |
| **Software** | the firmware `WaterSensorNode.ino`, all diagnostic sketches, the JavaScript files | [GNU General Public License v3.0 or later](LICENSES/GPL-3.0-or-later.txt) (`GPL-3.0-or-later`) |

Copyright © 2026 the KijaniSpace contributors.

## Hardware (CERN-OHL-S-2.0)

You may use, study, modify, make and distribute the hardware design. If you distribute the design
or products made from it, you must make the complete source of your version available under the
same licence. You must also keep the existing notices and mark your changes. The design is provided
without warranty. The licence text in [`LICENSES/CERN-OHL-S-2.0.txt`](LICENSES/CERN-OHL-S-2.0.txt)
is authoritative.

## Software (GPL-3.0-or-later)

This program is free software: you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
[`LICENSES/GPL-3.0-or-later.txt`](LICENSES/GPL-3.0-or-later.txt) for details.

## Third-party components

Libraries used by the firmware (WaziDev, xlpp, LowPower, Vcc, OneWire, DallasTemperature) and the
stock KiCad footprint libraries referenced by the board remain under their own licences. Product
names and trademarks belong to their respective owners.
