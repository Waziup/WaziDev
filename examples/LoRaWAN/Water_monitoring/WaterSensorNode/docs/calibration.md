# Probe Calibration

How the pH and conductivity probes of the KijaniSpace node are calibrated: in the field over LoRa, or on the bench with `EZOCal`. What each route can and cannot do, step by step, and what the node answers. Part of the series with the [Bring-Up](bring-up.md) and [Parts List](parts-list.md) pages.

- pH   **Atlas EZO-pH · 0x63**
- EC   **Atlas EZO-EC · 0x64**
- ROUTES **LoRa · EZOCal**
- AS OF **19 Oct 2026**

## What needs calibrating

| Sensor | Calibration | How often | Route |
| --- | --- | --- | --- |
| **pH** · EZO-pH | three points: mid 7, low 4, high 10. The electrode’s slope drifts as it ages and fouls. | every 4–8 weeks | LoRa or EZOCal |
| **Conductivity** · EZO-EC | probe constant `K` once, dry point once, then a single point near the lake’s ~100 µS/cm | every ~3 months | LoRa (dry + one point) or EZOCal (everything) |
| Dissolved oxygen · SEN0680 | factory-calibrated; site pressure set once (88.42 kPa) | after the yearly film-cap change | not by this node |
| Turbidity · Y511-A | factory-calibrated; the wiper keeps the optics clean | — | — |
| Temperature · DS18B20 | none | — | — |

Both EZO circuits keep their calibration in their own EEPROM. It survives power loss, a firmware update and a move to another node, and it travels with the circuit. The node itself stores nothing; it only forwards the command at the right moment.

## Before either route

> [!NOTE]
> **The rules that decide whether a calibration is any good**
>
> - **Never dilute a buffer or standard.** Its value comes from the exact salt mixture; a diluted one is no longer a reference.
> - **Temperature.** The circuit compensates with the water temperature the node measures (the DS18B20). Stand the bottles in the pond for 15 minutes first, so buffer and water are at the same temperature.
> - **Rinse between solutions**, then shake the probe dry. Never wipe the glass: rubbing charges it and the reading drifts for minutes.
> - **Submerge bulb and reference junction**, about 2–3 cm. A narrow 50 mL tube needs only 20–30 mL.
> - **pH: mid (7) always first.** `Cal,mid` wipes the other two points.
> - **EC: dry always first**, with the probe in air and completely dry.
> - **EC values are in µS/cm**, whatever the bottle says. 12.88 *m*S/cm is 12880. Lake Victoria sits near 100 µS/cm, so the **84 µS/cm** standard is the right one; 1413 is fourteen times above the range the node measures.

---

## Route A — over LoRa, in the field

No cable, no laptop: you push a number to an actuator in the WaziGate dashboard, and the node calibrates the probe that sits in the buffer. Use it for the routine recalibration of a deployed node.

> [!NOTE]
> **Needs, once**
>
> - Firmware with `ENABLE_LORA_CAL 1`. `VERBOSE_LOG` may be 0 or 1; both fit.
> - The device on WaziGate’s built-in **XLPP codec**, with the actuators *pH calibration*, *EC calibration* and *Interval*: run `create_wazigate_device.js` (new device) or `update_wazigate_device.js` (existing device) once in the dashboard’s browser console.
> - `K` already set on the EC circuit (Route B, step 3). LoRa cannot set it.

### What the node understands

WaziGate v3 sends only the number you push, with no channel and no actuator name. **The value alone decides what happens**, whichever actuator you push it to:

| Push | The node does |
| --- | --- |
| 7 | pH `Cal,mid,7.00`. Always first. |
| 4 | pH `Cal,low,4.00` |
| 10 | pH `Cal,high,10.00` |
| -3 | EC `Cal,dry`, probe in air |
| 84 | EC single point `Cal,84.00` (any value from 20 upwards works the same way: 1413, 12880 …) |
| -1 / -2 | clear the pH / EC calibration |
| 300 … 3600 | measurement interval in seconds, whole minutes only |
| 0 | nothing |

### The sequence

A pushed value waits on the gateway until the node’s next uplink, then arrives. The node reads until the value has been steady for 10 s (up to about 2.5 min), calibrates, sleeps, and reports on the uplink after that. **The probe stays in the solution from the push until the result appears.**

### 1 · Shorten the wait

Push `300` to *Interval*. From the next uplink on, the node reports every ~6 min instead of every 30. Optional, but it turns the first step from a half-hour wait into minutes. After any calibration the node cycles every ~5 min for half an hour anyway.

### 2 · pH 7

Probe into pH 7.00 buffer. Push `7` to *pH calibration*. Wait for *pH calibration result*.

> [!TIP]
> **Gate**
>
> **1**: one point stored.

### 3 · pH 4, then pH 10

Rinse, shake. Probe into pH 4.00, push `4`, wait. Rinse, shake. Probe into pH 10.00, push `10`, wait.

> [!TIP]
> **Gate**
>
> **2** after pH 4, **3** after pH 10.

### 4 · EC dry, then 84 µS/cm

EC probe out of the water, rinsed with distilled water and **completely dry**, in air. Push `-3`, wait. Then probe into the 84 µS/cm standard, push `84`, wait.

> [!TIP]
> **Gate**
>
> Any result from **0** to **3** means the circuit accepted it. Anything from 90 up is a refusal; see the table below.

### 5 · Back to normal

Push `1800` to *Interval*. **The interval does not return to 30 min by itself** if you changed it in step 1. Rinse the probes and put them back in the water. Check that the next pH and EC readings look like the lake again.

### What the result means

| Result | Meaning | What to do |
| --- | --- | --- |
| 0 – 3 | accepted; the number is how many points the circuit now holds | next step |
| 90 | refused: the steady reading does not fit the solution (pH more than 1.0 away, EC outside half to double) | check which bottle the probe is in. A new or badly drifted probe can also cause this: use Route B. |
| 91 | the circuit did not answer or rejected the command | push again; if it repeats, Route B and check the circuit with `i` and `Status` |
| 92 | the reading never settled within ~2.5 min | leave the probe in longer, push the same value again |
| 93 | refused: the cell is below 3.6 V and the node is hibernating | wait for sun, or charge |

> [!WARNING]
> **Limitations of the LoRa route**
>
> - **The actuator name never reaches the node.** Pushing 7 to *EC calibration* still calibrates pH. The names are labels for you.
> - **EC is dry + one point only.** Two-point EC (`Cal,low` / `Cal,high`) is bench work. For the lake’s narrow range one point at 84 is the better choice anyway.
> - **No `K`, no queries.** The probe constant, `Cal,?` on demand, the pH slope check and the circuit status are only reachable with EZOCal.
> - **One step at a time.** The gateway keeps only the latest pushed value; a second push before the node picked up the first replaces it.
> - **It waits for the next uplink**, up to the full interval. The probe has to be in the solution when the command arrives, not just when you pushed it.
> - **The temperature used is the lake’s** (DS18B20), not the buffer’s. Hence the 15 minutes in the pond for the bottles.
> - **The plausibility check can refuse a genuinely wrong probe:** more than 1 pH off, or EC outside half to double. That is the price of refusing lake water by mistake.
> - **The result is sent once.** If that one uplink is lost, the calibration may still have happened. Judge by the next readings, or check with `Cal,?` in EZOCal.
> - **A 1500 µS/cm standard** would be read as a 25 min interval. 84, 1413 and 12880 are not affected.
> - **Not below 3.6 V** (result 93).

---

## Route B — the standard procedure with EZOCal

`EZOCal/EZOCal.ino` is an I²C console to both circuits. Use it for **the first calibration of a new probe**, for setting `K`, for two-point EC, for checking the pH slope, and whenever Route A refuses. It works on the carrier board and on the jumper-wire build alike: same pins, `A1` for the isolators and `D6` for the rail.

> [!WARNING]
> **It replaces the node firmware**
>
> Uploading EZOCal overwrites `WaterSensorNode.ino` on the board. The node measures and transmits nothing until the firmware is uploaded again (step 5). On a potted node this needs the FTDI lead-out.

### 1 · Connect and upload

- **Cell connected.** EZOCal powers the isolators from it through `A1`; USB alone is not enough.
- FTDI adapter set to **3.3 V**, on the WaziSense FTDI header (or the M8/M12 lead-out).
- Upload `EZOCal/EZOCal.ino`. Serial monitor at **38400 baud**. Type a command and press Enter; the line ending does not matter.

> [!TIP]
> **Gate**
>
> The banner ends with `Isolators ON (A1)` and `Ready.`

### 2 · pH

```
# select the pH circuit, check it
#63
i                -> ?i,pH,<firmware>
T,24.5           the buffer's temperature
R                repeat until it stops moving, 1-2 min in fresh buffer
Cal,mid,7.00     in pH 7 - always first
Cal,low,4.00     rinse, pH 4, R until steady, then this
Cal,high,10.00   rinse, pH 10, R until steady, then this
Cal,?            -> ?CAL,3
Slope,?          electrode health: acid % and base % of ideal
```
A slope well below 90 % on either side means an aging electrode: calibrate more often, and plan a replacement.

### 3 · Conductivity

```
# select the EC circuit
#64
i
K,?              what the circuit thinks the probe constant is
K,1.0            set it to YOUR probe's K (0.1, 1.0 or 10), once per probe
Cal,clear
Cal,dry          probe in air, completely dry - always first
T,24.5
R                in the 84 uS/cm standard, until steady
Cal,84           single point, the right choice for this lake
Cal,?            -> ?CAL,1
```
**Two-point instead**, for water that ranges widely: after `Cal,dry`, `Cal,low,84` in the 84 standard, then `Cal,high,1413` in the 1413 standard. `Cal,?` then answers `?CAL,2`.

**Correction:** EZOCal’s help used to say `Cal,one,<value>` for a single point. That is not an Atlas command; the single point is `Cal,<value>`. If an earlier single point is in doubt, `Cal,?` shows whether it was stored.

### 4 · Write it down

Date, the `Cal,?` answers, the pH slope, the buffer lot. A drifting slope over months is the earliest warning of a dying electrode, and nothing else records it.

### 5 · Put the firmware back

Upload `WaterSensorNode.ino` with `VERBOSE_LOG 0` and `ENABLE_LORA_CAL 1`. For a bench log, set `VERBOSE_LOG 1`; remote calibration stays available.

> [!TIP]
> **Gate**
>
> The next uplink carries pH and conductivity in plausible ranges for whatever the probes now sit in.

## Which route when

| Situation | Route |
| --- | --- |
| New probe, or a circuit that has never been calibrated | B |
| Routine pH recalibration of a deployed node | A |
| Routine EC check: dry + 84 | A |
| Setting `K`, two-point EC, slope check | B |
| Route A answers 90 or 91 twice | B |
