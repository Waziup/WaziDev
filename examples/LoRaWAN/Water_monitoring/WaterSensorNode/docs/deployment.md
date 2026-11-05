# Node Deployment

From an assembled KijaniSpace Carrier Rev C to a running station at the pond: inspection, modules, wiring to the WaziSense, the bench test, the enclosure and the site. This guide is for the **carrier board**. The jumper-wire prototype is documented in [Node Wiring](prototype-wiring.md); the reasoning behind the board is on [Sensor Carrier](sensor-carrier.md).

- BOARD **Carrier Rev C · 100 × 100 mm**
- MCU   **WaziSense v2**
- SITE  **Lake Victoria · 1135 m**
- AS OF **4 Nov 2026**

> [!WARNING]
> **Six rules that protect hardware**
>
> - **Antenna on before the first power-up.** The node transmits in its first cycle; an open antenna port can destroy the radio.
> - **The cell goes to `X1` on the carrier only**, never straight to the WaziSense’s `BAT_IN`. The carrier feeds the WaziSense through its fuse and ideal diode.
> - **`H1` stays on 3.3 V.** BATT puts up to 4.2 V on a 3.8 V pin.
> - **Pin 1 of every module on the square pad marked `1:…`**. VCC and GND swapped destroys the transceiver and both EZO circuits.
> - **Nothing ever connects `D6−` to GND.** It is the switched return; one wire there and the sensor rail never switches off.
> - **Do not fit `R11`/`R12`, do not close `I2C_J`** on the WaziSense. The isolators bring their own pull-ups. *Only exception: `U4` empty or bridged (step 4).*

## What you need

The full list, item by item with its position on the board, is the [Build List](build-list.md). In short:

| Group | Items |
| --- | --- |
| Boards | Carrier Rev C (assembled by JLCPCB), WaziSense v2, LoRa antenna |
| Plug-in modules | Pololu U3V9F12 (12 V boost), RS485 module (MAX13487), 2 × Atlas Basic EZO Inline Voltage Isolator (both required; a bridge plug is a bench stop-gap only, step 4), Atlas EZO-pH, Atlas EZO-EC |
| Power | 2 × 18650 with welded solder tabs (same type and batch), 1S protection board (e.g. DW01 + 8205A), sealed toggle or rocker switch rated ≥ 3 A DC (IP67), 2-pin polarised connector (JST-PH or XT30), **12 V** solar panel 5–10 W |
| Probes | DS18B20, DFRobot SEN0680 (DO), Yosemitech Y511-A (turbidity), Atlas pH and EC probes with SMA extensions |
| Enclosure | IP67 box, cable glands (UV-stable, black PA), membrane pressure vent, silica gel (plus one sealed spare for the installer), adhesive-lined heat shrink, self-amalgamating tape; optional: electronics potting compound |
| Tools | multimeter, 1 Ω shunt, FTDI adapter set to 3.3 V, laptop with Arduino IDE, buffers pH 4/7/10 and EC 84 µS/cm |

---

### 1 · Inspect the board

Runs: no modules · no cell

- **`U1` first**: the 3-pin socket must sit on the three holes marked `1:VOUT`. An early Rev C placement file had it 5.5 mm off. If it is missing or crooked, fit a 1×3 female header (2.54 mm) by hand.
- All 13 screw terminals and all socket rows present. Terminal openings face the nearest board edge; `X1` faces down, `J8` faces up toward the RS485 socket.
- `C1`, `C4`, `C8`: stripe away from the “+” on the silkscreen.
- `R11`, `R12` and `U2` are empty on purpose.

> [!TIP]
> **Gate**
>
> `J2` pin `D6−` to any GND (e.g. `J6` GND): **open**. With no modules fitted there is no DC path at all. Anything under 100 Ω is a bridge.

### 2 · First power, board alone

Runs: bench supply 3.7 V, limit 100 mA · or the cell

Supply into `X1` (`+` and `−` as printed). Nothing else connected.

> [!TIP]
> **Gate**
>
> `J6` `+BAT` to GND reads the supply minus about **20 mV**. That is the ideal diode `Q5` conducting. A drop of 0.4 V would mean a Schottky, and the wrong board revision.
>
> Optional, with the current-limited supply only: reverse it for a second. `+BAT` must read 0 V and nothing may get warm.

### 3 · Prepare the WaziSense

- **`H1` on 3.3 V.** **`I2C_J` open.**
- **Optional: 10 kΩ between FTDI header pins 3 (RXD) and 4 (3.3 V)**, soldered directly on the header. It is a precaution for an FTDI lead-out cable: it holds the bootloader's receive pin at its idle level, so noise on the open cable cannot reach it. A hang from a floating receive pin has not been seen on a WaziSense. Without a lead-out, leave it out.
- **Antenna on.**
- Upload `WaterSensorNode.ino` with this node’s LoRaWAN keys, `VERBOSE_LOG 0`, `ENABLE_LORA_CAL 1` and `SLEEP_DEFAULT_S 1800`. **Check `VERBOSE_LOG` before every deployment upload:** a sketch left from bench work may still be at 1.

### 4 · Fit the modules

Lay each module on the board as it will sit: component side up, pins down. Its pin 1 goes on the square pad with the `1:…` label.

| Position | Module | Pin-1 label |
| --- | --- | --- |
| `U1` | Pololu U3V9F12, 12 V boost. Its labels are printed on the back, so read them as mirrored. | `1:VOUT` |
| `U3` | RS485 module, `VCC TXD RXD GND`. Its `120_S` jumper stays **open**: `RT1` on the carrier is the only terminator. | `1:VCC` |
| `U4` / `U5` | isolator for pH / for EC, MCU row on top | `1:VCC` |
| `U6` / `U7` | EZO-pH (above `J7` pH) / EZO-EC (above `J9` EC), bus row on top | `1:GND` |

- **The EZO circuits must be in I²C mode** (`0x63` / `0x64`). A new circuit ships in UART mode. Switch it with `EZOSwitch`, on the carrier itself:
  - **only the one circuit in its socket**, the other one pulled;
  - **the cell connected**: the sketch powers `A1` and `D6` (since November 2026; before that it powered `D6` only and found nothing on the carrier);
  - it tries every EZO baud rate, factory 9600 first, and sends `I2C,99` / `I2C,100` only after the circuit has named itself;
  - LED: **green** = UART, **blue** = I²C, **red flashes** = "command not understood" (usually a circuit set to another baud rate), **dark** = no supply: measure VCC to GND at its socket (`U6`/`U7` pin 4 to pin 1).
- **No isolator at hand?** A position can be bridged; with `U4` empty or bridged the bus then needs 2 × 4.7 kΩ to `3V3P`. Pin map, pull-ups and which bridge may stay: [Sensor Carrier](sensor-carrier.md), *Running without an isolator*. In short: both are bench-only. A bridged EZO runs on the raw cell, and the EZO-EC read 0–10 µS/cm at 3.38 V (measured 4 Nov 2026; until then this step let EC stay bridged after a gate).
- **RS485 module to `J8`:** two short wires from the module’s own `A`/`B` screw terminal to `J8` `A`/`B`.

### 5 · Wire carrier to WaziSense, then the probes

Five short cables join the boards. **Wire by name, not by position**: the blocks carry the same pin numbering on both boards, but they face each other, so the left-to-right order looks mirrored. Check each conductor with the meter before power.

| Carrier | Silkscreen | WaziSense block | Note |
| --- | --- | --- | --- |
| `J2` SW\_1 | `D6− · D6+` | **Sensor Power 1** (SW\_1): − to −, + to + | not Sensor Power 2 |
| `J3` ANALOG | `GND · A1 · A2` | ANALOG\_PINS | A2 = 1-Wire, A1 = isolators |
| `J4` DIGITAL | `GND · D3 · D4` | DIGITAL\_PINS | RS485 RX / TX. **Run `RS485Pins` before the probes**: if it reports `REVERSED`, cross `D3` and `D4` here (below) |
| `J5B` + `J5A` I2C\_PORT | `GND · D5 · SDA · SCL` | I2C\_PORT | D5 = 12 V switch |
| `J6` POWER\_OUT | `GND · +BAT` | BAT\_IN | the WaziSense’s only battery feed |

> [!WARNING]
> **The RS485 module’s `TXD`/`RXD` labels are not the same on every module** *(November 2026)*
>
> The board joins `D4` (transmit) to the module pad labelled `TXD`, which is right for the module the prototype used. On a second carrier the module fitted the same way, `VCC` on `1:VCC`, but its `TXD` pin was the receiver *output*. Nothing was sent, both probes were silent on every baud rate and address, and A/B, power, terminator and bias all measured correct. `RS485Drive` showed it (A–B did not move while transmitting) and `RS485Pins` named it (`REVERSED`).
>
> - **Fix:** swap the two wires on `J4`, so `D4` lands on the pin that was `D3` and vice versa. `GND` stays. Then `RS485Pins` must report `CORRECT AS WIRED`.
> - **Not in software**, although the sketch once suggested it: the firmware and every diagnostic sketch expect `D4` = TX, and a carrier-specific build is a trap for the next flash.
> - Swap only *after* `RS485Pins` has spoken. Wired the wrong way, `D4` is fighting a push-pull output; guessing twice can exceed the AVR’s per-pin limit.

| Carrier | Connects | Note |
| --- | --- | --- |
| `X1` 18650 CELL IN | the battery pack, through its switch and connector (step 8) | soldered leads; a sprung contact costs voltage |
| `X2` DS18B20 | `3V3 · DQ · GND` | the 4.7 kΩ pull-up is on the board (`R9`). Remove any external one. |
| `X3A/X3B` PROBE 1 | `+12V · GND · A · B` | DO or turbidity, either one |
| `X4A/X4B` PROBE 2 | `+12V · GND · A · B` | the other probe. Bias and termination are on the board. |
| `J7` pH · `J9` EC | SMA extension cables | finger-tight; keep the white insulator clean and dry |
| WaziSense SLR\_IN | 12 V solar panel | the panel charges the cell back through `J6` and `Q5` |

### 6 · Bench test, whole node

Runs: cell connected · probes in a bucket of water · FTDI for the log only

1. Connect the cell. Watch one cycle in the serial monitor (38400 baud). A cell under 3.5 V hibernates from the first cycle on and prints `HIBERNATE - low cell` (the firmware checks the cell before the first cycle since 4 November 2026): charge it first.
2. **Gateway, once per device:** fill in the node's address and key in `create_wazigate_device.js` and paste it into the WaziGate dashboard’s browser console. It creates the device with the XLPP codec, the named sensors and the actuators *pH calibration*, *EC calibration*, *Interval*. For a device that already exists, use `update_wazigate_device.js`.
3. Wait for the next uplink. All seven values should appear on the named sensors.
4. Test one LoRa calibration: pH probe in pH 7, push `7`; the result should read **1** on the uplink after (see [Probe Calibration](calibration.md)).
5. Unplug the FTDI and measure sleep current across a 1 Ω shunt in the cell lead, not with the meter in series — or, if the meter must go in series, short it with a clip lead during boot and send and take the lead off once the node sleeps; unshorted, it browns the node out (five bootloops in a row on 4 Nov 2026). Record it; this is the carrier’s first real figure. On a Rev C carrier expect about **0.5 mA per probe** on the bus on top of a ~0.03 mA base: in sleep the bus floats to 3.3 V into the unpowered probes (measured 4 Nov 2026; fixed in Rev D).

> [!TIP]
> **Gate**
>
> One full cycle with no `FAIL`, every value on the dashboard in a plausible range, a calibration result of 0–3, and the sleep current noted.

### 7 · Enclosure and cables

- **All glands on the bottom face**, a drip loop below each cable, UV-stable black glands.
- **One gland per cable**, or a multi-hole insert matched to the cable diameters. Never several cables through a single-hole seal.
- **pH and EC coax: connectors stay inside.** The pH electrode sits behind 100–500 MΩ; a damp SMA face outside pulls the reading toward pH 7. An SMA plug does not pass a PG7: use a larger gland with a slotted multi-hole insert, or build the coax up with adhesive heat shrink where the seal grips.
- **The outdoor probe-to-extension joint:** clean, dry, then one piece of adhesive heat shrink over the whole joint, 15–20 mm onto each jacket. Tie it up off the ground, in shade.
- **Membrane pressure vent** (e.g. M12) and a **silica gel** pack. Without the vent the box breathes humid air in through the seals every day–night cycle.
- **Antenna:** a bulkhead jack on the wall is fine for the 50 Ω radio line. Wrap the joint with self-amalgamating tape.

### 8 · Battery pack

Two cells in parallel double the autonomy without sun, from about 25 days to roughly 7 weeks at a 30 min interval. The chain is:

**cells → protection board → switch → connector → `X1`**

- **Parallel (1S2P), never in series.** The node expects one Li-ion cell, 3.0–4.2 V; two in series give 8.4 V.
- **Same type, same batch, same voltage.** Bring both cells to within **0.05 V** of each other before joining them, or a large equalising current flows the moment they touch.
- **Solder to the welded tabs only, never to the can.** Iron heat on the can damages the seal and the separator.
- **Protection board** directly at the cells. Its undervoltage cut-off (~2.5 V) is the last line of defence: the node's hibernation stops the measurements but **switches nothing off**. The node keeps drawing about 1.0–1.1 mA asleep on a Rev C carrier with both probes *(this step said about 1.5 mA, the jumper-wire build's figure, until 4 November 2026)* and would run the cells down to deep discharge over a long dull spell. **Prefer a board with a higher cut-off**, around 3.0 V instead of the usual 2.4–2.5 V: it keeps more reserve and still lets the charger bring the pack back. It is the node's hardware cut-off; a switch on the carrier was considered and rejected, because the charger feeds the cell back through `BAT_IN`, so such a switch would cut the charge path too, and a small hysteresis trips on the 12 V load dips. The board also limits a short circuit. It draws only a few µA and may be potted; lead its wires out of the resin with strain relief.
- **The switch** turns the node on at installation and keeps it off in transit.
  - **Rated ≥ 3 A DC.** The radio and the 12 V rail draw a few hundred mA in peaks. A miniature slide switch (0.5 A class) has enough contact resistance to brown the node out on a weak cell.
  - **Sealed (IP67).** Open contacts corrode in a humid box within a few years.
- **The connector** lets the pack be changed in a few years without soldering. Keep it and the switch outside any resin.
- **Placement:** the coolest spot in the box, not under a lid in full sun. Heat is what ages the cells fastest, and a solar node sits near full charge all day. **Plus cap into free air**, never against a wall or the resin. Fix the cells with silicone adhesive or foam tape, **never epoxy or cyanoacrylate on the can**.
- **Condensation** forms on the lid at night and drips. Cover the tabs, the joints and the protection board with conformal coating or heat shrink.

> [!WARNING]
> **The cells never go into the resin**
>
> - **The safety vent sits under the plus cap.** If a cell is overcharged, shorted or deep-discharged and then recharged, the vent must release the gas. Under resin it cannot open properly, and a failure ends more violently.
> - **Curing heat.** A large epoxy pour can exceed 60 °C while it cures.
> - **The cells wear out.** After 2–4 years of a hot, mostly full life they will no longer carry the node through a rainy season. Loose, that is a cheap swap; potted, the whole node is scrap.
> - **Stress and moisture.** Hard resin and the steel can expand differently. Daily temperature cycles crack the bond, and water then sits against the can and its seal.

### 9 · Potting the electronics (optional)

The box, the glands and the vent keep the water out; potting adds protection against condensation and vibration, at the price of every repair. Decide before pouring that the build is final.

- **Material:** a soft **PU or silicone electronics potting compound** is preferred. Hard epoxy passes the daily temperature cycles on as stress to solder joints and ceramic capacitors, which fail months later where nothing can be repaired. If it has to be epoxy, use a potting grade with low shrinkage and pour in layers.
- **Pour only enough to cover the boards and modules**, not the whole box. Less heat while curing, less weight, less material.
- **Clean first:** flux off the pH input (`J7`, `U6`) and every SMA joint with isopropanol, then dry. The pH electrode sits behind 100–500 MΩ, and a residue film in the resin interface becomes a permanent leakage path: the reading drifts toward pH 7.
- **SMA joints inside the resin:** tighten every joint fully before pouring so no resin runs into the threads. The probes then change at the outer extension joint.

**Keep free of resin:**

| Part | Why |
| --- | --- |
| FTDI lead-out | `DTR · TXD · RXD · GND`, 1:1 from the WaziSense header, **not 3.3 V**, to an IP67 M8/M12 connector with a sealing cap. Needed for EZOCal and firmware updates. |
| Reset button | the only way to restart without opening the power |
| Battery pack, switch, connector | step 8 |
| Antenna bulkhead jack | the radio line must stay serviceable |
| Electrolytic capacitors | see below |

**Electrolytic capacitors** have their own vent, the cross scored into the top of the can.
- **Check the voltage rating first.** The bulk capacitor on the 12 V rail must be rated **≥ 25 V**. A 10 V part there is a fault with or without resin; a 16 V part is below the specified margin and worth replacing before it is buried.
- **Best:** replace it with a **polymer capacitor** (solid electrolyte; 470 µF / 25 V is enough). It has no vent and does not dry out in the heat.
- **Otherwise:** mount the electrolytic upright so its top stays **above** the resin.
- **Fully potted** only with the right rating, the right polarity and a 105 °C part. It vents only on failure, but then it can split the resin.

> [!TIP]
> **Gate · before pouring**
>
> - One firmware upload through the FTDI lead-out.
> - Sleep current unchanged with the lead-out fitted.
> - Ten power cycles with the switch, all of which start.
> - Several days in a bucket with a LoRa calibration and an interval change, without a `FAIL` or an unexpected *Restart cause*.

> [!WARNING]
> **A probe that dies while the resin cures** *(November 2026)*
>
> On one node every probe read correctly right before pouring; after curing the turbidity probe answered no more, while the oxygen probe on the same bus and the same 12 V rail still read correctly. That combination clears the rail, the transceiver and the bus: the fault is in that probe's own four leads inside the resin (a joint pulled by shrinkage or curing heat, a lead moved while the resin set), or in the probe.
>
> - **Strain-relieve and fix every probe lead before pouring**, and do not move the cables until the resin has set.
> - **Repair without opening:** both Modbus probes are wired in parallel to the same four nets (`+12V`, `GND`, `A`, `B`) and the firmware tells them apart by baud rate. So the dead probe can be spliced onto the working probe's cable **outside** the box, by function, not by colour (the two makers use different colour codes; find `+12V` with a meter during a cycle). Test with temporary clamps first: if the probe answers, the fault was inside; if not, it is the probe. Make the splice above the waterline in a gel-filled or IP68 junction box, add no second 120 Ω terminator, and insulate each core of the dead lead separately: its `+12V` core is still switched on every cycle.

### 10 · Shipping

- **Cells at about half charge, 3.7–3.8 V.** Li-ion ages least there and loses only a few percent over several months.
- **Switch OFF**, taped in position and labelled: *OFF — switch on at installation*.
  - **Why it must be off:** the node draws about 1.0–1.1 mA even asleep or hibernating (Rev C carrier, both probes; this step said about 1.5 mA until 4 November 2026). A shipment that takes longer than its autonomy ends in a deep-discharged pack, and no firmware setting prevents that.
  - **Air freight:** lithium cells inside equipment have their own rules (UN3481), including protection against accidental activation. Check with the carrier.
- **Pack with the node:**
  - one fresh silica gel pack in a sealed bag;
  - a printed copy of the switch-on steps in step 11.

### 11 · At the site

Mount and connect:

- **pH and EC probes** fully submerged, bulb and junction covered, clear of the mud, in a protective cage or perforated tube.
- **DO probe and DS18B20 side by side.** Channel 1 and channel 7 are a cross-check on each other only when they sit in the same water.
- **Turbidity probe** with the wiper free to move; window facing sideways or down, out of direct sun and away from settling sediment.
- **Solar panel** unshaded all day. At the equator a tilt of 10–15° is enough, and it lets rain wash the glass.
- **Antenna** vertical, as high as practical, clear of metal.
- **Strain relief** on every cable before it reaches a gland.
- **Box** glands down, ideally in shade or under a small roof.

Then switch on:

1. **Open the lid. Switch ON.** The LED blinks 20 times at start-up. In opaque resin the LED is hidden; then step 5 is the proof.
2. **Connect the solar panel after switching on**, if it has a plug. Fed from the panel alone, with no cell, the node does not run stably.
3. **Replace the silica gel** with the fresh pack. The one inside is saturated after the journey.
4. **Wipe the gasket clean, close the lid** and tighten the screws crosswise.
5. **On the gateway:** the first uplink shows *Restart cause* **1** (power-on; **5**, power-on plus brown-out while the supply came up, is also normal). That proves the node was freshly switched on and transmits.
6. **Set the *Interval*** over LoRa if the default is not wanted (60–7200 s; see [Probe Calibration](calibration.md)).

### 12 · Commission

1. Calibrate in place over LoRa: pH 7, 4, 10, then EC dry and 84 µS/cm.
2. Check the *Interval* is back at `0` (firmware default, 30 min) or `1800`.
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
| weekly | glance at the data: gaps, flat lines, the two temperatures apart, the battery trend, and any new *Restart cause* (meaning in [Bring-Up](bring-up.md#restart-cause-and-the-watchdog-channel-8)) |
| every 4–8 weeks | pH recalibration over LoRa; clean the probes of biofilm |
| every ~3 months | EC check: dry + 84 µS/cm; compare against a handheld meter |
| every visit | replace the silica gel; check glands, the heat-shrunk joint, the panel glass |
| every 2–4 years | the battery pack: replace it when it no longer carries the node through a dull week (step 8) |
| yearly | DO fluorescent film cap; pH electrode if its slope is falling (EZOCal `Slope,?`) |

From a full single cell the node runs about 25 days with no sun at all; with the two-cell pack of step 8 roughly 7 weeks (estimated, not yet measured). Both figures were computed for about 1.5 mA sleep; a Rev C carrier with both probes sleeps at about 1.0–1.1 mA, so they still hold roughly. *Rev D (laid out, not yet built; see [`pcb/revD/README.md`](../pcb/revD/README.md)): very roughly a third more autonomy is expected, because it removes most of the sleep current; not measured.* Below 3.5 V it stops measuring and reports only its battery every 2 hours; above 3.7 V it resumes by itself.
