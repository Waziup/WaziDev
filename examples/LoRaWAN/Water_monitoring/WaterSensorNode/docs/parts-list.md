# Parts list — solar LoRaWAN water node

Everything in the build as it stands, grouped the way it is wired. Quantities are per node. Values that were measured on this unit rather than read off a datasheet are marked as such.

- SITE  **Lake Victoria / fish farm**
- MCU   **WaziSense v2 (ATmega328P)**
- RADIO **SX1276 · EU868 · SF12**
- PAIRS **wiring + bring-up guides**

> [!NOTE]
> ### Status · 7 October 2026
>
> **There is now a carrier board**, and its own bill of materials is generated from the board file: [`pcb/BOM.csv`](../pcb/BOM.csv), with [`pcb/out/FACTS.md`](../pcb/out/FACTS.md) as the authoritative description — that one is generated from the board file on every build. `README.md` is ordinary prose and is not generated, which this guide claimed until 13 October 2026. **Where this guide and that BOM disagree, the BOM is right.** This guide stays the list for the whole node — panel, cell, probes, consumables — which the board’s BOM does not cover.
>
> What the carrier changes for ordering: the resistors, capacitors, the P-MOSFET, the terminals and the sockets are on the board’s BOM and are placed by the assembler, so they no longer need buying loose. The six plug-in modules — two Atlas circuits, two isolators, the Pololu converter and the RS485 module — are *not* stocked by the assembler; the board carries **female headers** at those positions and the modules are plugged in afterwards. Three positions are left empty on purpose: `R11`, `R12` and `U2`.
>
> **Rev C assembly note.** An early placement file put the 3-pin socket for the Pololu converter (`U1`) 5.5 mm off its holes. The build now catches this (`pcb/gen/cplcheck.py`). On boards made from early files, check that socket first.
>
> Corrected on this guide: the transceiver’s part number (Group 04), the isolated-side pull-ups (Group 05), the second isolator, which Group 08 still listed as dropped while Group 05 listed it as fitted, and the whole energy budget.

`fitted` in the node as built `to order` decided, not yet chosen `ordered` on its way `bench` bring-up and calibration only `dropped` rejected — so it is not re-bought

---

## GROUP 01Energy — harvest, storage, protection

| Qty | Part | Spec | Role | Status |
| --- | --- | --- | --- | --- |
| 1 | Solar panel | **12 V nominal (18 V Voc), 5–10 W** | **A 6 V panel is not enough, measured.** The WaziSense’s CN3791 only reaches full charge current from about **6.5 V at its input**; at 6 V the charging LED is already lit but only ~0.13 W goes in, which looks like charging and is not. A 12 V panel keeps the MPPT above that point through cloud and dirt. *(This row said “6 V, 5–10 W” and called it fitted.)* | `to order` |
| 1 | MPPT charge controller | solar → 1S Li-ion | Charge management for the cell | `fitted` |
| 1 | 18650 Li-ion cell | 2500 mAh, 3.0–4.2 V. **UNPROTECTED — no PCM, confirmed.** | Storage. About 25 days from full with no sun at all — but only down to the **3.5 V working floor**, not to 3.0 V. Below that nothing disconnects the cell, so a long dark spell discharges it past the point of damage rather than merely flat. Either fit a protected cell, or set the firmware hibernate threshold and trust it. | `fitted` |
| 1 | Cell holder | 18650, sprung contacts | **Solder directly for the final build.** A sprung contact adds an ohm or so, and 100 mA through it costs voltage the 3.3 V regulator does not have spare. | `fitted` |
| 1 | PTC resettable fuse | 2.6 A hold | First element after the cell | `fitted` |
| 1 | Ideal diode — **AO3401A + 100 k** | P-channel MOSFET, 50 mΩ, SOT-23. Drain to the cell, source to the load, gate to ground through 100 kΩ. | **Replaced the SS34 on 9 October 2026.** The Schottky dropped **0.45–0.5 V** at the ~400 mA the 12 V converter pulls, which is exactly why pH, EC and turbidity dropped out at a 3.4 V cell while the LoRa uplink still worked. The MOSFET drops **20 mV** and moves the working floor from ~3.5 V to ~3.2 V. On the carrier it is `Q5` + `R13`; both merge into BOM lines that already existed, so the board got *cheaper* to source. **Orientation is everything** — source-to-cell would conduct on a reversed cell and the board would still appear to work. | `fitted` |
| 1 | TVS diode | SMBJ5.0A | Clamp across the protected battery rail. **Was under suspicion, now cleared — keep it.** Its datasheet leakage is alarming for this node (IR **800 µA max at 5.0 V**, permanently across the cell) but the measurement acquits it: sleep current went 4.2 V → 1.46 mA, 3.0 V → 1.17 mA, where a purely resistive load would give 1.04 mA. Avalanche leakage collapses much faster than that, so the TVS is at most ~0.4 mA and probably far less. `D2` on the carrier board stays as specified. | `fitted` |
| 1 | TVS diode | for the panel's open-circuit voltage | Clamp at the solar input | `fitted` |

## GROUP 02Power conversion and gating

| Qty | Part | Spec | Role | Status |
| --- | --- | --- | --- | --- |
| 2 | Pololu #2810 | Mini MOSFET Slide Switch with reverse-voltage protection, **LV**. 1.8–16 V, 3.0 A at 55 °C, `ON` pin switches above ~1 V. | **(A)** gates the boost converter's input from `D5`. **(B)** gates the pH isolator's supply from `A1`. **D7 is not available as a logic pin on this board** — it exists only as the "Sensor Power 2" terminal pair, which stays unused. **Both slide switches go to OFF**, or they override the `ON` pin. The `ON` pin is the alternative to the slide switch, which is what makes these usable under firmware control. | `fitted` |
| 1 | Pololu #5585 | U3V9F12 step-up. 2.6–20 V in, **12 V fixed ±4 %**, input current to ~900 mA, 75–95 % efficient. | Battery to 12 V for the two RS485 probes. Named by Pololu as the drop-in replacement for the discontinued U3V12F12. | `fitted` |
| 1 | Electrolytic capacitor | 1000 µF, ≥25 V | Bulk at the U3V9F12's output, on `X2-1`. A 470 µF does the same job here — but it must be a *second* capacitor, not the one from the 3.3 V rail. | `fitted` |
| 1 | Electrolytic capacitor | 470 µF, ≥10 V | Bulk across the 3.3 V rail, `D6(+)` to a GND pin. Fitting this ended a cold-start brownout. | `fitted` |
| 1 | Ceramic capacitor | 10 µF X7R | Parallel to the 470 µF, short leads. The electrolytic’s ESR cannot carry the first microseconds of a load step; an extra electrolytic is not a substitute. **On the carrier board this is in the released BOM**, so it only needs buying for the jumper-wire build. | `to order` |
| 1 | Ceramic capacitor | 100 nF | Same node, for the fastest edge. **On the carrier board, in the released BOM.** | `to order` |

> [!NOTE]
> ### Why the switch goes before the converter
>
> A boost converter passes its input through to its output when disabled, through the inductor and the diode — measured on this build as **7.2 V on the rail with 2.836 V at the input**. So an enable pin does not make the probes safe to leave connected, and a converter gated only at its output still draws its own quiescent current all night.
>
> Switching the *input* removes both problems at once, and it is why the opto-isolated module that used to sit after the converter is no longer in this list.

## GROUP 03Controller and radio

| Qty | Part | Spec | Role | Status |
| --- | --- | --- | --- | --- |
| 1 | WaziSense v2 | ATmega328P + RFM95W (SX1276). `H1` on **3.3 V**. Charger **CN3791** MPPT; regulator **HE2031A33MPR**, 3.5–25 V in, 0.18 V dropout at 100 mA, 2.0 µA quiescent, >400 mA out. Sleep **29 µA** with the solar circuit active, **6.5 µA** without. | The node. Flash is at 90 % with the current sketch. **It carries no protection on the battery input** — no fuse, no reverse-polarity element, no TVS — so the PTC, the **AO3401A ideal diode** and the TVS above are the only protection in the whole node, not a second line of defence. *(This row named the SS34 until 9 October 2026; that part is gone.)* **Keep `H1` on 3.3 V** even though the manual advises BATT with a solar panel: the ATmega sits behind the regulator either way, so BATT extends nothing, and it would put up to 4.2 V from the RS485 module’s push-pull `RO` straight onto `D3`. | `fitted` |
| 1 | Antenna | λ/2 dipole, 868 MHz | Outdoor. A patch antenna is fine on the bench only. | `fitted` |
| 1 | SMA bulkhead + pigtail | coax | Enclosure feed-through. Seal the outdoor joint with self-amalgamating tape. | `fitted` |
| 1 | Resistor | 10 kΩ | **From `RXI` to `D6(+)`.** Without it the bootloader's receive pin floats when no FTDI is attached, reads phantom start bits and waits for an upload forever — a hang that neither the reset pin nor a watchdog can clear. | `to order` |
| 1 | FTDI adapter | **3.3 V** setting | Upload and serial log. Cannot power the 12 V chain: on USB alone the battery domain measured 2.836 V. | `bench` |

## GROUP 04RS485 / Modbus side

| Qty | Part | Spec | Role | Status |
| --- | --- | --- | --- | --- |
| 1 | RS485 module — **MAX13487** | **auto-direction**, run at 3.3 V — only `RXD`, `TXD`, `GND`, `3.3 V` brought out | Bus master side. No `DE`/`RE` to drive, which is what freed `D5`. Verified on the bench not to echo its own transmission. **The chip is marked MAX13487, not MAX3485** — this list said MAX3485 for months. Analog specifies the MAX13487 at **4.75–5.25 V**; this module nonetheless works at 3.3 V, and the whole build runs that way. Its on-board terminator jumper `120_S` is **open** (10.8 kΩ across A/B), and it adds its own fail-safe bias on top of the external network. | `fitted` |
| 1 | DFRobot SEN0680 | Dissolved oxygen, RS485, **DC 10–30 V**, 4800 baud, address `0x01`, float ABCD | Oxygen and its own water temperature. **15 s** optics warm-up per reading, then a readiness check on the probe’s own temperature register — a probe that is not ready answers 0.00 mg/L, which is indistinguishable from genuinely anoxic water unless you ask it something else. *(Was 30 s; the bench run showed 15 s costs 0.04 mg/L against a probe that resolves ±0.6.)* | `fitted` |
| 1 | Yosemitech Y511-A | Turbidity with wiper, RS485, **12 V**, 9600 baud, address `0x01`, float **DCBA** | Turbidity and anti-fouling. Survived 30 V from the failed converter, but it is a 12 V part. | `fitted` |
| 1 | Resistor | 120 Ω | Bus termination — **one only**, at the transceiver. Confirmed by measurement rather than assumed: the module’s own `120_S` jumper is open, and the probe cable reads **116.5 Ω** with this resistor fitted and no sensors attached, so the probes do not terminate either. On the carrier board this is `RT1`. | `fitted` |
| 2 | Resistor | 680 Ω | Fail-safe bias: `A` to `D6(+)`, `B` to `D6(−)`. Measured 1.845 V / 1.52 V, 271 mV differential on the bare divider. **With the module in circuit, `A`−`B` idles at 0.36 V** — the transceiver’s own fail-safe bias adds to it. | `fitted` |

> [!NOTE]
> ### Two probes, one address, and why that is fine
>
> Both probes ship as address `0x01` and neither was reconfigured. They are separated by **baud rate instead**: the firmware switches the port to 4800 for the oxygen probe and 9600 for the turbidity probe, and a frame arriving at the wrong rate fails its CRC rather than being answered. That removes a configuration step that can only be done once, with a tool, and is easy to get wrong in the field.

## GROUP 05I²C side — pH, conductivity, isolation

| Qty | Part | Spec | Role | Status |
| --- | --- | --- | --- | --- |
| 1 | Atlas EZO-pH | I²C address `0x63` | pH. Ships in UART mode; switched with `I2C,99`. | `fitted` |
| 1 | Atlas pH probe | + connector board to SMA/BNC | `PRB` to the centre conductor, `PGND` to the shell. `PGND` is the reference electrode, not a ground. | `fitted` |
| 1 | Atlas EZO-EC | I²C address `0x64` | Conductivity. Switched with `I2C,100`. Needs its probe's `K` value and `Cal,dry` before it reads anything but zero. | `fitted` |
| 1 | Atlas EC probe | K value per probe | Lake Victoria sits near 100 µS/cm, so a low-range probe suits it. | `fitted` |
| 2 | SMA bulkhead jack + pigtail | panel-mount, with nut and O-ring; short SMA pigtail to `J7`/`J9` | **Enclosure feed-through for the pH and EC electrodes, decided 13 October 2026.** The carrier’s own jacks stay **vertical**; the part fitted is the **BWSMA-KE-Z001 (LCSC C496549)**, not the Amphenol 132134 this row named until 16 October 2026 — same holes, a ninth of the price. A board-mounted right-angle jack was considered and rejected: a board-side jack has no panel thread, so it cannot seal a wall penetration, and the torque of screwing an electrode cable on goes straight into the solder joints. A bulkhead seals properly and takes the torque into the wall. The two extra gold junctions are harmless inside a dry enclosure. **Reconsidered 16 October 2026, not yet decided:** for the pH line a **cable gland with the connector kept inside** is the safer choice. The pH electrode sits behind some 100–500 MΩ, so a damp film across an outdoor SMA face pulls the reading toward pH 7, and two shells side by side in the rain bridge the pH and EC grounds the isolator separates. The plug passes an M20 gland; build the thin coax up to about 7–8 mm with adhesive-lined heat shrink where the seal clamps. | `to order` |
| 2 | Basic EZO Inline Voltage Isolator | BE-IVI. 3.0–5.0 V in, isolated 3.9 V out, ADM3260, `EN` pin. | **Two fitted; only the pH one is required.** A loop needs two grounded ends, so floating the pH side already opens it. The second costs ~82 mA and adds one more device that clamps the I²C bus when unpowered — **it is the first thing to remove if autonomy ever binds.** Both run from the cell through #2810 (B), *not* from `D6(+)`. | `fitted` |
| 2 | Resistor | 4.7 kΩ | I²C pull-ups, `A4`/`A5` to `D6(+)`. Fitted because `I2C_J` measured **5.9 MΩ** on this board despite the manual. | `fitted` |
| 2 | Resistor | 4.7 kΩ | **Settled: not needed.** The isolator carries its own pull-ups on the isolated side. This is why `R11` and `R12` are deliberately left unfitted on the carrier board — adding a second pair would only load the isolated bus. Do not buy these. | `dropped` |

> [!NOTE]
> ### What the isolators draw, and why they are not on `D6(+)`
>
> Measured on this build: **97.1 mA** with the pH isolator and EZO-pH connected, **15 mA** without — so **82 mA for that pair**. Later, **130 mA was measured for both pairs together**, which means the EC pair adds only about 48 mA rather than the 82 mA its datasheet suggests. The two measurements were taken at different times and under different loads; if the exact split ever matters, meter it again rather than trusting either figure. What both agree on is the order of magnitude, and that is what decides the wiring.
>
> Switching that onto the board’s 3.3 V regulator — the same one feeding the ATmega — browns the MCU out at every rail switch-on, with or without USB attached.
>
> The isolator’s input is specified for 3.0–5.0 V, so it runs straight off the cell behind its own #2810. **The isolated output was metered at 3.825 V with either 3.0 V or 3.7 V on the input**: the isolated side regulates, so a cell that sags across its discharge does not drag the EZO supply down with it. The current and its inrush never reach the MCU’s supply, which is why this is the second switch in Group 02 rather than a capacitor somewhere.
>
> **A switched-off isolator is not a branch that draws nothing.** Disconnecting its `VOUT` drops the node’s sleep current from 1.46 mA to 1.36 mA and the module’s LED goes out — 0.1 mA back-feeding through the LED’s ESD diodes.

## GROUP 06Temperature

| Qty | Part | Spec | Role | Status |
| --- | --- | --- | --- | --- |
| 1 | DS18B20 | waterproof probe, 1-Wire on `A2` | Water temperature. Single device, index 0, no addressing. Reads within 0.3 K of the oxygen probe's own sensor, which is a useful cross-check on both. | `fitted` |
| 1 | Resistor | 4.7 kΩ | **Mandatory** 1-Wire pull-up, `A2` to `D6(+)`. To the plus terminal — wiring it to `D6(−)` makes it a pull-down and the sensor never answers. | `fitted` |

## GROUP 07Calibration consumables

Buffers are references, not concentrates. **Never dilute one** — the value comes from the buffer capacity of a specific salt mixture, and a diluted buffer is no longer a known reference. An error taken in here lands in every reading the node ever sends.

| Qty | Part | Spec | Role | Status |
| --- | --- | --- | --- | --- |
| 1 | pH 7.00 buffer | 125 mL | Midpoint — always calibrated first, because `Cal,mid` clears the other two. | `bench` |
| 1 | pH 4.00 buffer | — | Low point for the slope — and it doubles as **storage solution**: acidic, keeps the membrane hydrated, discourages growth. That removes the need to wait for a dedicated storage bottle. | `bench` |
| 1 | pH 10.00 buffer | — | High point for the slope. The important one here: Lake Victoria and fish ponds sit alkaline, so this anchors the side the node actually measures on. | `bench` |
| 1 | pH storage solution | — | Purpose-made, and the cleanest long-term answer — but **pH 4.00 buffer does the same job**, so this is a convenience rather than a blocker. What matters either way: never store the probe in distilled or tap water. Low ionic strength pulls the KCl out of the reference through the junction, and the probe goes slow and drifty. A night in air is recoverable; weeks dry crystallises the junction. | `to order` |
| 1 | EC standard set | 3 × 50 mL: **1413 µS/cm · 12.88 mS/cm · 111.3 mS/cm** — a decade series of KCl standards, 0.01 M / 0.1 M / 1 M | **NOTE THE UNITS, and note that none of these suits this node.** An earlier version of this guide listed all three as µS/cm and told you to calibrate at 111.3 as “this water’s own range”. That was wrong twice over: 111.3 is **mS/cm** (the 1 M KCl standard, 111,300 µS/cm, for brine), and Lake Victoria sits near **100 µS/cm** — so the *lowest* bottle here, 1413 µS/cm, is already fourteen times above the measuring range. **Order an 84 µS/cm standard** (Atlas’ own low-range point) before deployment; until it arrives, calibrate single-point on 1413 and verify on 12.88 mS/cm. Values go into the circuit in **µS/cm**: 1413, 12880, 111300. All three are specified at 25 °C / 77 °F, so set `T,` before reading. Pour a portion, cap the bottle, measure quickly. | `ordered` |
| 1 | DO fluorescent film | spare cap for the SEN0680 | **A consumable, not a permanent part — about one year of normal use.** That sets a yearly maintenance trip on its own, so order a spare with the sensor rather than after it fails. Between uses: protective cap on, its sponge damp with plain water and squeezed out, kept above 0 °C. Plain water here, *not* the storage solution the pH probe needs — the two are not interchangeable. | `bench` |
| 1 | Distilled water | — | Rinsing between buffers. Shake off, never wipe — rubbing charges the glass and the drift takes minutes to settle. Until it arrives: rinse with tap water, then with a little of the buffer you are about to measure in. Seconds of tap water do no harm; it is only *storage* in low-ionic-strength water that leaches the reference. | `ordered` |
| 1 | Narrow vessel | 50 mL tube or slim beaker | Both the bulb *and* the reference junction must be submerged, about 2–3 cm. A narrow vessel reaches that depth on 20–30 mL instead of five times as much. | `bench` |

## GROUP 08Considered and dropped

Kept in the list so that none of it gets re-bought, and so the reasoning survives the decision.

| Qty | Part | Why it is not in the build | Status |
| --- | --- | --- | --- |
| — | PT1000 + MAX31865 | Replaced by the DS18B20. The 1-Wire probe is one part instead of two, needs no SPI, and its accuracy is far inside what a fish pond calls for. The firmware still carries the code behind `ENABLE_PT1000`. | `dropped` |
| — | Opto-isolated 22 A module | It existed only to cut the boost converter's output, because a disabled converter passes its input through. Gating the converter's input instead leaves it nothing to do. | `dropped` |
| — | XL6009 boost module | **Failed dangerously.** At 3.6 V input it put out **30 V**, at 3.7 V input 14 V. That is inside the SEN0680's 10–30 V window and well outside the Y511-A's, so the turbidity probe went silent while the oxygen probe kept answering. A fault that tracks the sensors' voltage ratings is a supply fault. | `dropped` |
| — | Second isolator for EC | **Reinstated — it is fitted.** One isolator does open the loop, and the EC circuit is the aggressor rather than the victim, so this entry was correct as a decision. It was then reversed: both are fitted, and the carrier board has a position for each. It stays listed here because **the EC one is still the first thing to remove if autonomy ever binds**. (Group 05 and this row contradicted each other for some time; Group 05 was the current one.) | `fitted` |
| — | Series resistor for inrush | At the isolator's measured 82 mA even 10 Ω drops 0.82 V. The arithmetic ruled it out before anything was soldered. | `dropped` |

---

## BUDGETWhat it all draws

| State | Current | Duration per 30 min cycle | Note |
| --- | --- | --- | --- |
| Sleep | 1.46 mA | ~1752 s | Both rails off, both #2810 switches off, radio and TWI down, pins parked by role. Was 3.2 mA when this table was first written, and 20.8 mA before the rails were gated at all. Traced so far: **29 µA** is the board itself (per its manual), **447 µA** is the I²C pull-ups — in sleep the lines measure 2.25 V instead of 3.3 V, so (3.3−2.25)/4.7 kΩ per line — of which 100 µA was traced to the isolator branch. That leaves **984 µA unexplained**. The TVS in Group 01 was the prime suspect and has been **cleared** — at 3.0 V the sleep current was 1.17 mA where a resistive load would give 1.04 mA, and avalanche leakage falls far faster than that. The pull-up term does not exist on the carrier board, where the only pull-ups are the isolators’ own and `A1` switches them. **Measure with the FTDI unplugged** — it feeds the battery domain and hides exactly this. |
| Measuring | ~100 mA avg160 mA peak | ~48 s | The peak is the isolators, and `A1` now raises them only for the two seconds the pH and EC reads need rather than for the whole window — about 78 mAh a day saved. |
| Average | ~4.1 mA | — | **≈98 mAh a day, so ≈25 days from a full 2500 mAh cell with no sun at all** (was ≈19 days at 3.2 mA sleep). The panel covers the rest with room to spare. |
| Airtime | 2.0–2.5 s | per uplink | SF12. At 1800 s spacing that is 0.14 % against the EU868 1 % duty cycle. 48 uplinks a day is ~115 s — fine on an own gateway, over TTN's 30 s/day fair-use policy. |
