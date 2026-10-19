# Node Deployment

From an assembled KijaniSpace Carrier Rev C to a running station at the pond: inspection, modules, wiring to the WaziSense, the bench test, the enclosure and the site. This guide is for the **carrier board**. The jumper-wire prototype is documented in [Node Wiring](prototype-wiring.md); the reasoning behind the board is on [Sensor Carrier](sensor-carrier.md).

- BOARD **Carrier Rev C · 100 × 100 mm**
- MCU   **WaziSense v2**
- SITE  **Lake Victoria · 1135 m**
- AS OF **19 Oct 2026**

> [!WARNING]
> **Six rules that protect hardware**
>
> - **Antenna on before the first power-up.** The node transmits in its first cycle; an open antenna port can destroy the radio.
> - **The cell goes to `X1` on the carrier only**, never straight to the WaziSense’s `BAT_IN`. The carrier feeds the WaziSense through its fuse and ideal diode.
> - **`H1` stays on 3.3 V.** BATT puts up to 4.2 V on a 3.8 V pin.
> - **Pin 1 of every module on the square pad marked `1:…`**. VCC and GND swapped destroys the transceiver and both EZO circuits.
> - **Nothing ever connects `D6−` to GND.** It is the switched return; one wire there and the sensor rail never switches off.
> - **Do not fit `R11`/`R12`, do not close `I2C_J`** on the WaziSense. The isolators bring their own pull-ups.

## What you need

| Group | Items |
| --- | --- |
| Boards | Carrier Rev C (assembled by JLCPCB), WaziSense v2, LoRa antenna |
| Plug-in modules | Pololu U3V9F12 (12 V boost), RS485 module (MAX13487), 2 × Atlas Basic EZO Inline Voltage Isolator, Atlas EZO-pH, Atlas EZO-EC |
| Power | 18650 cell with holder, **12 V** solar panel 5–10 W |
| Probes | DS18B20, DFRobot SEN0680 (DO), Yosemitech Y511-A (turbidity), Atlas pH and EC probes with SMA extensions |
| Enclosure | IP67 box, cable glands (UV-stable, black PA), membrane pressure vent, silica gel, adhesive-lined heat shrink, self-amalgamating tape |
| Tools | multimeter, 1 Ω shunt, FTDI adapter set to 3.3 V, laptop with Arduino IDE, buffers pH 4/7/10 and EC 84 µS/cm |

---

### 1 · Inspect the board

Runs: no modules · no cell

- **`U1` first**: the 3-pin socket must sit on the three holes marked `1:VOUT`. An early Rev C placement file had it 5.5 mm off. If it is missing or crooked, fit a 1×3 female header (2.54 mm) by hand.
- All 13 screw terminals and all socket rows present. Terminal openings face the nearest board edge; `X1` faces down, `J8` faces up toward the RS485 socket.
- `C1`, `C4`, `C8`: stripe away from the “+” on the silkscreen.
- `R11`, `R12` and `U2` are empty on purpose.

> [!TIP]
> **Gate**
>
> `J2` pin `D6−` to any GND (e.g. `J6` GND): **open**. With no modules fitted there is no DC path at all. Anything under 100 Ω is a bridge.

### 2 · First power, board alone

Runs: bench supply 3.7 V, limit 100 mA · or the cell

Supply into `X1` (`+` and `−` as printed). Nothing else connected.

> [!TIP]
> **Gate**
>
> `J6` `+BAT` to GND reads the supply minus about **20 mV**. That is the ideal diode `Q5` conducting. A drop of 0.4 V would mean a Schottky, and the wrong board revision.
>
> Optional, with the current-limited supply only: reverse it for a second. `+BAT` must read 0 V and nothing may get warm.

### 3 · Prepare the WaziSense

- **`H1` on 3.3 V.** **`I2C_J` open.**
- **10 kΩ between FTDI header pins 3 (RXD) and 4 (3.3 V)**, soldered directly on the header. Without it a floating receive pin can hang the bootloader, and only a power cycle clears that. Mandatory if an FTDI lead-out cable is fitted later.
- **Antenna on.**
- Upload `WaterSensorNode.ino` with this node’s LoRaWAN keys, `VERBOSE_LOG 0` and `ENABLE_LORA_CAL 1`.

### 4 · Fit the modules

Lay each module on the board as it will sit: component side up, pins down. Its pin 1 goes on the square pad with the `1:…` label.

| Position | Module | Pin-1 label |
| --- | --- | --- |
| `U1` | Pololu U3V9F12, 12 V boost. Its labels are printed on the back, so read them as mirrored. | `1:VOUT` |
| `U3` | RS485 module, `VCC TXD RXD GND`. Its `120_S` jumper stays **open**: `RT1` on the carrier is the only terminator. | `1:VCC` |
| `U4` / `U5` | isolator for pH / for EC, MCU row on top | `1:VCC` |
| `U6` / `U7` | EZO-pH (above `J7` pH) / EZO-EC (above `J9` EC), bus row on top | `1:GND` |

- **The EZO circuits must already be in I²C mode** (`0x63` / `0x64`). The two from the prototype are. A new circuit ships in UART mode: switch it first with `EZOSwitch` on the bench, one circuit at a time.
- **RS485 module to `J8`:** two short wires from the module’s own `A`/`B` screw terminal to `J8` `A`/`B`.

### 5 · Wire carrier to WaziSense, then the probes

Five short cables join the boards. **Wire by name, not by position**: the blocks carry the same pin numbering on both boards, but they face each other, so the left-to-right order looks mirrored. Check each conductor with the meter before power.

| Carrier | Silkscreen | WaziSense block | Note |
| --- | --- | --- | --- |
| `J2` SW\_1 | `D6− · D6+` | **Sensor Power 1** (SW\_1): − to −, + to + | not Sensor Power 2 |
| `J3` ANALOG | `GND · A1 · A2` | ANALOG\_PINS | A2 = 1-Wire, A1 = isolators |
| `J4` DIGITAL | `GND · D3 · D4` | DIGITAL\_PINS | RS485 RX / TX |
| `J5B` + `J5A` I2C\_PORT | `GND · D5 · SDA · SCL` | I2C\_PORT | D5 = 12 V switch |
| `J6` POWER\_OUT | `GND · +BAT` | BAT\_IN | the WaziSense’s only battery feed |

| Carrier | Connects | Note |
| --- | --- | --- |
| `X1` 18650 CELL IN | cell holder, `+` and `−` | solder the holder leads; a sprung contact costs voltage |
| `X2` DS18B20 | `3V3 · DQ · GND` | the 4.7 kΩ pull-up is on the board (`R9`). Remove any external one. |
| `X3A/X3B` PROBE 1 | `+12V · GND · A · B` | DO or turbidity, either one |
| `X4A/X4B` PROBE 2 | `+12V · GND · A · B` | the other probe. Bias and termination are on the board. |
| `J7` pH · `J9` EC | SMA extension cables | finger-tight; keep the white insulator clean and dry |
| WaziSense SLR\_IN | 12 V solar panel | the panel charges the cell back through `J6` and `Q5` |

### 6 · Bench test, whole node

Runs: cell connected · probes in a bucket of water · FTDI for the log only

1. Connect the cell. Watch one cycle in the serial monitor (38400 baud).
2. **Gateway, once per device:** paste `setup_wazigate_device.js` into the WaziGate dashboard’s browser console. It switches the device to the XLPP codec and creates the named sensors and the actuators *pH calibration*, *EC calibration*, *Interval*.
3. Wait for the next uplink. All seven values should appear on the named sensors.
4. Test one LoRa calibration: pH probe in pH 7, push `7`; the result should read **1** on the uplink after (see [Probe Calibration](calibration.md)).
5. Unplug the FTDI and measure sleep current across a 1 Ω shunt in the cell lead, not with the meter in series. Record it; this is the carrier’s first real figure.

> [!TIP]
> **Gate**
>
> One full cycle with no `FAIL`, every value on the dashboard in a plausible range, a calibration result of 0–3, and the sleep current noted.

### 7 · Enclosure and cables

- **All glands on the bottom face**, a drip loop below each cable, UV-stable black glands.
- **One gland per cable**, or a multi-hole insert matched to the cable diameters. Never several cables through a single-hole seal.
- **pH and EC coax: connectors stay inside.** The pH electrode sits behind 100–500 MΩ; a damp SMA face outside pulls the reading toward pH 7. An SMA plug does not pass a PG7: use a larger gland with a slotted multi-hole insert, or build the coax up with adhesive heat shrink where the seal grips.
- **The outdoor probe-to-extension joint:** clean, dry, then one piece of adhesive heat shrink over the whole joint, 15–20 mm onto each jacket. Tie it up off the ground, in shade.
- **Membrane pressure vent** (e.g. M12) and a **silica gel** pack. Without the vent the box breathes humid air in through the seals every day–night cycle.
- **Antenna:** a bulkhead jack on the wall is fine for the 50 Ω radio line. Wrap the joint with self-amalgamating tape.

> [!NOTE]
> **If the electronics are potted in resin**
>
> - **Keep the 18650 out of the resin.** It must stay replaceable, and a swelling cell cracks solid resin.
> - Electronics-grade potting compound, poured in layers. Clean flux off the pH input (`J7`, `U6`) with isopropanol first.
> - **FTDI lead-out:** `DTR · TXD · RXD · GND`, 1:1 from the WaziSense header, **not 3.3 V**, to an IP67 M8/M12 connector with a sealing cap. Needed for EZOCal and for firmware updates.
> - Before pouring: one upload through the lead-out, sleep current unchanged with the lead-out fitted, and ten power cycles that all start.
> - Everything inside is final afterwards. That includes the modules, the sockets and `U1`.

### 8 · At the site

- **pH and EC probes** fully submerged, bulb and junction covered, clear of the mud, in a protective cage or perforated tube.
- **DO probe and DS18B20 side by side.** Channel 1 and channel 7 are a cross-check on each other only when they sit in the same water.
- **Turbidity probe** with the wiper free to move; window facing sideways or down, out of direct sun and away from settling sediment.
- **Solar panel** unshaded all day. At the equator a tilt of 10–15° is enough, and it lets rain wash the glass.
- **Antenna** vertical, as high as practical, clear of metal.
- **Strain relief** on every cable before it reaches a gland.

### 9 · Commission

1. Calibrate in place over LoRa: pH 7, 4, 10, then EC dry and 84 µS/cm.
2. Check the *Interval* is back at `1800`.
3. Watch the first day of data.

> [!TIP]
> **Gate · plausible values in Lake Victoria**
>
> | Value | Expect | Suspect |
> | --- | --- | --- |
> | Water temperature (ch 1, ch 7) | 24–28 °C, within ~0.5 K of each other | a growing gap: one probe drifted or came loose |
> | pH | 6.5–9 | stuck near 7.0: damp connector or dead electrode |
> | Dissolved oxygen | 4–10 mg/L | exactly 0.00 together with 0.00 °C on ch 7: the probe answered before it had booted |
> | Conductivity | ~100 µS/cm | 0: not calibrated, or no dry point |
> | Turbidity | low, rising after rain | 327: off the scale |
> | Battery | 3.7–4.2 V, recovering by day | falling over days in full sun: the panel is not charging |

## Maintenance

| When | What |
| --- | --- |
| weekly | glance at the data: gaps, flat lines, the two temperatures apart, the battery trend |
| every 4–8 weeks | pH recalibration over LoRa; clean the probes of biofilm |
| every ~3 months | EC check: dry + 84 µS/cm; compare against a handheld meter |
| every visit | replace the silica gel; check glands, the heat-shrunk joint, the panel glass |
| yearly | DO fluorescent film cap; pH electrode if its slope is falling (EZOCal `Slope,?`) |

From a full cell the node runs about 25 days with no sun at all. Below 3.6 V it stops measuring and reports only its battery once an hour; above 3.8 V it resumes by itself.
