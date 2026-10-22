# Water Node Bring-Up

The order in which this node was actually made to work, and the observable result that had to be true before each step could be trusted. Companion to [Water Sensor Node — Wiring](prototype-wiring.md): that one says what connects to what, this one says what to run and what you should see. The [parts list](parts-list.md) is the third piece.

- SITE  **Lake Victoria / fish farm**
- MCU   **WaziSense v2 (ATmega328P)**
- RADIO **SX1276 · EU868 · SF12**
- CYCLE **30 min · all sensors every cycle**

> [!NOTE]
> **Status · 7 October 2026**
>
> Bring-up is finished; the node ran on jumper wires through every stage below. **The carrier board now exists** — placed, routed, checked, with Gerbers, drill file, BOM and placement files released. Where this guide and [`pcb/out/FACTS.md`](../pcb/out/FACTS.md) disagree, **FACTS.md is right**: it is generated from the board file itself on every build. *(This paragraph named `pcb/README.md` until 13 October 2026; that file is prose, is not generated, and had drifted.)*
>
> Three things on this guide were wrong. They are corrected in place rather than deleted, because the wrong version is the more instructive one: the `Wire` timeout (Stage 1), the reason for the `H1` jumper (Stage 1 preconditions), and the isolators' current draw (Stage 6).

> [!WARNING]
> **When the assembled carrier boards arrive — before anything is plugged in**
>
> **Look at `U1`, the 3-pin socket for the Pololu boost module, first.** It must sit on the three holes marked `1:VOUT`. An early Rev C placement file had it 5.5 mm away from them. If it is missing or crooked, the 12 V chain has no converter — fit a 1×3 female header (2.54 mm) there by hand before going further.
>
> Then the rest of the placement: every screw terminal and every socket row present, terminal openings toward the nearest board edge (`X1` facing down, `J8` facing up toward the RS485 socket), `C1`/`C4`/`C8` with the stripe away from the “+” marking.

## Why the order matters

Every stage below is a precondition for the next, and each has one measurable criterion rather than an impression. That is not tidiness: the two longest delays in this build both came from skipping a gate. An I²C scan ran for two days against a bus that had no pull-up resistors, and a dissolved-oxygen sensor was declared faulty and nearly returned under warranty while the actual cause — a wrongly set echo flag — sat in the firmware.

**The rule that would have caught both: after fixing any suspected cause, re-run the basic test before writing a new diagnostic.** Three tools got written in the gap where one repeated scan would have shown the sensor answering.

## The scripts

All in the repository root. Every diagnostic is read-only on the serial monitor — nothing needs typing except the EZO bridge.

| Sketch | Answers | Stage |
| --- | --- | --- |
| `I2CScan/` | Can this I²C bus work at all? Measures residual voltage in mV and rise time in µs per line, bit-banged so it cannot hang. | 1, 3 |
| `EZOSwitch/` | Switches one EZO circuit from UART to I²C, picking the address from the circuit's own reply. | 3 |
| `legacy/EZOBridge/` | A **UART** console to one EZO circuit over the existing SDA/SCL wires. This is the tool that *performs* the switch to I²C — and the moment the switch takes effect, it can no longer reach the circuit. | 3 |
| `EZOCal/` | An **I²C** console to `0x63` / `0x64`. The calibration tool, and the one to use on the circuits as they are now. | 8 |
| `VccCal/` | Prints the untrimmed bandgap reading and the raw A0 count, so both battery constants can be fitted from one run. | 8 |
| `RS485Drive/` | Does the node's own RS485 driver work? Two multimeter readings, no probe needed. | 4 |
| `RS485Pins/` | Which board pin sits on the transceiver's output? Safe — no contention risk. | 4 |
| `RS485Turnaround/` | How long does the module keep the bus after transmitting? | 4 |
| `RS485Scan/` | Finds a Modbus device across baud rates, addresses and registers, and dumps raw bytes in all four byte orders. | 5 |
| `RS485Config/` | Writes a device's address or baud rate. Refuses unless exactly one device is present. | 6 |
| `WaterSensorNode.ino` | The firmware. Staged by `ENABLE_*` flags. | 2, 6, 7 |
| `create_wazigate_device.js` | Gateway: creates the device from scratch (XLPP codec, named sensors, calibration actuators). Paste into the dashboard's browser console. | 7 |
| `update_wazigate_device.js` | Gateway: brings an existing device up to date, same result. | 7 |

---

## Before power

> [!WARNING]
> **Damage risk**
>
> **Attach the antenna before the board is ever powered.** The firmware transmits in its first cycle, at SF12 and full power. A radio driving into an open port sees a full mismatch, and the reflected power can destroy the RFM95W's output stage. This is the only item on this guide that costs hardware.

> [!NOTE]
> **Preconditions**
>
> - **H1 jumper on 3.3 V.** It sets both "Sensor Power" blocks at once. *The reason this guide used to give was wrong.* The transceiver was assumed to be a MAX3485 with a 3.0–3.6 V limit; the chip on the module is marked **MAX13487**, which Analog Devices specifies at **4.75–5.25 V**. It works at 3.3 V regardless — the whole bring-up ran that way — so 4.2 V would not have destroyed it. Keep the jumper on 3.3 V for the real reason: that rail is *regulated*. The battery position hands every sensor an unregulated 4.2 → 3.0 V slide, and the bias network, the ADC references and the DS18B20 minimum all move with it.
> - **...and the manual says the opposite, so here is why not.** The WaziSense v2 manual advises *“move the H1 jumper to BATT position when deploying with a solar panel. It appears the solar circuit’s highs and lows during operation affects transmissions.”* That advice was written for a bare board driving simple sensors. On this node it must not be followed, for one hard reason and one that removes the incentive:
> 	- **The hard one.** `H1` sets the voltage at the Sensor Power terminals, which on the carrier is the net `3V3REG`, and the **RS485 module’s `VCC` sits on it**. Its `RO` output is *push-pull* and runs straight to `D3` with no series resistor. At BATT and a full cell that drives **4.2 V into a pin whose absolute maximum is VCC+0.5 = 3.8 V**, with nothing to limit the current.
> 	- **The one that removes the point.** `H1` does not touch the ATmega or the radio — those always run from the on-board HE2031A33MPR. So BATT cannot keep the node alive one millivolt longer than 3.3 V does; it only keeps the *sensors* powered after the MCU has already browned out, which is worth nothing. The transmission interference the manual describes is real, but this node has already engineered it away, and that is what settles the question. **`railsOff()` runs at line 1521; `sendLoRaWAN()` at line 1575.** By the time the radio speaks, `D6`, `D5` and `A1` are all low, `SWGND` is floating and **nothing on the carrier draws a microamp from the WaziSense's regulator**. The manual's advice is about unloading that regulator during transmission. It is already unloaded. **Final answer: `H1` stays on 3.3 V.**
> - **Battery connected.** USB alone cannot run this node: over USB the battery domain at `X1-1` measured **2.836 V**, below the boost converter's minimum input, so it stopped regulating and the 12 V rail read **7.2 V**. Every Modbus device then looks identically dead on every baud rate.
> - **Boost set with a meter, before any probe is connected. 12 V.** Earlier revisions said 14 V; that was a workaround for a converter that was failing — the same unit later put out **30 V from a 3.6 V input**. A regulated 12 V that holds under the wiper motor is what both probes are specified for. If the voltage at the *probe* sags during a sweep, the cure is thicker cable, not a higher setting.
> - **Serial monitor at 38400 baud.** The IDE defaults to 9600 and shows only garbage.
> - A bench supply on the 12 V rail is worth it for the ammeter alone — the current reading separates "sensor dead" from "sensor alive but not talking". Limit it to about 1 A: the Y511-A specifies a source able to deliver over 500 mA for the wiper, so a 150 mA limit trips on the first sweep.

---

### 1 · I²C bus integrity

Runs: **I2CScan/** · nothing else connected

Before any device is on the bus, prove the bus can carry a signal. The scan is bit-banged with a bounded wait on every edge, so it cannot block, and it measures two numbers per line rather than sampling high or low once.

> [!TIP]
> **Gate**
>
> ```
> PHASE 1  line integrity
> SCL (A5): idle=H  driven-low=12 mV  rise=3 us   ok
> SDA (A4): idle=H  driven-low=12 mV  rise=3 us   ok
> SDA/SCL: independent  ok
> PHASE 2  address scan 0x08..0x77
> 0 device(s) found.
> ```
> Zero devices is correct here — nothing is connected yet. What must be true is **both lines ok**, with `driven-low` near 12 mV and `rise` in single-digit microseconds.

> [!WARNING]
> **If it fails**
>
> `rise=never` means the line floats: there is no pull-up. Measure `A4`–3V3 and `A5`–3V3 — they must read about **4.7 kΩ**. On this board the `I2C_J` jumper was **open** despite the manual stating it ships closed, and both lines measured **5.9 MΩ**. Two 4.7 kΩ resistors were fitted from `A4` and `A5` to `D6(+)` — the switched rail, so the pull-ups power up and down with the devices they serve.
>
> **The manual, now that it has been read.** It states the pull-ups go to **3.3 V** — the *regulated* rail, not a switched one — and that `I2C_J` is “closed by default”. On this board it was open. Fitting your own to `D6(+)` is still the right answer, but it is not free: `D6(+)` is permanently live, so an unpowered device on the bus clamps the line and the pull-ups deliver current all night. **Measured in sleep: SDA and SCL sit at 2.25 V, not 3.3 V** — that is (3.3 − 2.25) / 4.7 kΩ = **223 µA per line, 447 µA for the pair**. On the carrier board this is gone by construction: the only pull-ups are the isolators’ own, and their supply `ISO_VIN` is switched by `A1`, so pull-ups and bus die together. **Leave `R11`/`R12` unfitted and `I2C_J` open.**
>
> A missing I²C pull-up does not degrade gracefully. The TWI hardware releases SCL and waits for it to rise before it can issue a START; with no pull-up it waits forever. The symptom is a log that simply stops mid-cycle with no message.
>
> **Correction.** This guide used to blame the AVR core for having no timeout. It has had one since core 1.8.1. What defeated it was this sketch: the call was wrapped in `#if defined(WIRE_HAS_TIMEOUT)`, and **that macro does not exist** — the core defines only `WIRE_HAS_END`. The guard was false on every core, so the timeout was never armed anywhere. It is now set unconditionally, `Wire.setWireTimeout(25000, true)`, at all three places that touch the bus. **Never put that guard back.**

### 2 · Temperature

Runs: **WaterSensorNode.ino** · `ENABLE_DS18B20 1`, everything else `0`

The simplest sensor first, and the only one with no protocol to configure. Four wires: `VDD` and `GND` from `X3`, `DQ` to `A2`, and a 4.7 kΩ pull-up from `DQ` to `D6(+)`.

> [!TIP]
> **Gate**
>
> ```
> Temperature (DS18B20): 24.25 degC
> ```
> A plausible room temperature, moving in its decimals between cycles.

> [!WARNING]
> **If it fails**
>
> **The pull-up goes to `+3V3`, not to `GND`.** 1-Wire is open-drain: the sensor only ever pulls the line down, and the resistor is the only thing that restores the idle-high state. Wired to ground it is a pull-*down*, the line never rises, and the sensor is electrically invisible. That mistake was made here and cost an evening.
>
> **−127 °C** is `DEVICE_DISCONNECTED`. Exactly **85.0 °C** is the DS18B20's power-on reset value, which is what you read if the conversion has not finished or the supply is marginal — relevant here because the rail is gated. The firmware's sanity band of −20 to +60 °C rejects both, so a bad reading shows as `FAIL` rather than as plausible data.

### 3 · EZO circuits — one at a time

Runs: **EZOSwitch/** then **I2CScan/** · one circuit's SDA+SCL at a time

Atlas EZO circuits ship in **UART mode**, not I²C. In that state they are not merely silent on an I²C bus — their `TX` pin actively drives one of the two bus lines, which jams it for everything else. So they must be switched before they can be scanned.

> [!WARNING]
> **One at a time, and this one is hard to undo**
>
> Both circuits share the same two wires, and a mode-switch command is obeyed by **every** circuit listening. Sending `I2C,99` with both connected puts both at address `0x63`, after which neither can be addressed individually and the only way back is the datasheet's hardware procedure on both boards. `EZOSwitch` therefore refuses to act unless exactly one circuit answers, and it reads the device type out of that reply so a swapped cable cannot hand the pH circuit the EC address.

> [!TIP]
> **Gate**
>
> ```
> SCL (A5): idle=H  driven-low=12 mV  rise=0 us   ok
> SDA (A4): idle=H  driven-low=12 mV  rise=0 us   ok
> 0x63  ACK   <-- EZO-pH
> 0x64  ACK   <-- EZO-EC
> 2 device(s) found.
> ```
> **The number that proves it is `SDA driven-low`, not the LED colour.** Before the switch it read **2177 mV**: two UART transmitters holding the line high against the AVR pulling it down. An I²C pin is open-drain and can never drive high, so **12 mV** — the same as SCL — is the objective confirmation. LED colours were misread twice during this build; the millivolts were not.

> [!NOTE]
> **Wiring detail**
>
> **I²C goes straight through: SDA to SDA, SCL to SCL.** Only UART is crossed. The EZO pins are dual-function, so the same physical wires carry both — which is also what lets `EZOBridge` talk UART to a circuit with no rewiring at all.

### 4 · RS485 — the node side, before any probe

Runs: **RS485Drive/** · **RS485Pins/** · **RS485Turnaround/** · no probe connected

A silent Modbus device tells you nothing about which side is at fault, and "timeout" is the same message for a wiring error, a wrong address and a dead sensor. These three prove the node's own half first, so that everything found later is genuinely on the probe's side.

> [!TIP]
> **Gate · bias network, measured with a meter**
>
> | Point | Calculated | Measured here |
> | --- | --- | --- |
> | `A` to GND, idle | 1.784 V | 1.845 V |
> | `B` to GND, idle | 1.516 V | 1.520 V |
> | `A` − `B`, idle | 268 mV | 271 mV |
>
> Those follow from the divider — 3.3 V across 680 Ω + 120 Ω + 680 Ω is 2.23 mA — so matching them proves four things at once: the transceiver has its 3.3 V, both bias resistors are on the correct rails, the 120 Ω terminator is across A–B, and A/B actually reach the terminal block.
>
> **Those figures are the bare divider.** With the transceiver module in circuit, `A` − `B` measures **0.36 V** rather than 271 mV — the MAX13487 carries its own fail-safe bias, which adds to the external network. Termination was then settled by two further measurements: the module's own `120_S` jumper is **open** (10.8 kΩ across A–B, unplugged and unpowered), and the probe cable reads **116.5 Ω** with the external 120 Ω fitted and no sensors attached — so the probes do not terminate either. One terminator is correct, and the external 120 Ω is the only one there is. On the carrier board it is `RT1`, fitted.

> [!TIP]
> **Gate · driver, receiver, turnaround**
>
> **Driver:** while transmitting, `A`−`B` must go *negative*. The passive bias network can only ever make A higher than B, so a sign change is proof the driver is active — here it swung to **−0.675 V**.
>
> **Receiver:** `RS485Pins` must report `D3` driven and `D4` floating. With a valid idle bus the transceiver's `RO` output holds a logic HIGH, so the pin that recovers after being discharged is the module's output. Reversed, the MCU's transmit pin would be fighting that output at roughly 66 mA — above the ATmega328P's 40 mA per-pin maximum, which is why this is tested in software rather than by swapping wires to see.
>
> **Turnaround:** under 3.5 character times — 7.3 ms at 4800 baud, 3.65 ms at 9600. Measured **2.06 ms**, very consistently. A slave only recognises end-of-frame after 3.5 characters of silence and so cannot answer sooner, which means anything below that has released the bus in time.

### 5 · Each Modbus probe, alone

Runs: **RS485Scan/** · one probe, the other's A/B *and* power disconnected

Both probes ship on address `0x01`, so they cannot be identified together. Aim the scan with three constants at the top of the sketch and let it dump raw bytes; the register sweep reads two registers from every address in a window, so the windows overlap and the raw values can be reconstructed.

> [!TIP]
> **Gate · what each probe turned out to be**
>
> |  | DFRobot SEN0680 · oxygen | Yosemitech Y511-A · turbidity |
> | --- | --- | --- |
> | Baud | 4800 8N1 | 9600 8N1 — silent at 4800 |
> | Address | 0x01 | 0x01 |
> | Value register | 0x0002 · mg/L | 0x2602 · NTU |
> | Temperature | 0x0004 · °C | 0x2600 · °C |
> | Byte order | ABCD · big endian | DCBA · fully reversed |
> | Answers after rail-on | 0.4 s | 0.4 s |
> | **Ready for a command** | 3 s | **3 s** |
> | Supply | 10–30 V · 10.6 mA | 12–24 V · 17 mA, 50 mA wiping |
>
> A CRC-valid reply where **exactly one** byte order gives a plausible number identifies both the register and the format at once. One register is not enough: at `0x2600` two permutations both looked sensible, and only `0x2602` — nonsense in one of them — settled it.

> [!NOTE]
> **The internal consistency check that confirmed the decode**
>
> The SEN0680 reported 6.64 mg/L at 25.6 °C, and its saturation register read 0.8165. Oxygen saturation at 25.6 °C is about 8.2 mg/L, and 6.64 / 8.2 = 0.81 — the same number. Two registers agreeing through physics is much stronger evidence than one plausible-looking value.

> [!WARNING]
> **What a silent probe was, twice**
>
> **Neither time was it the sensor.** First the 12 V rail was starved because the node ran on USB. Then, with power fixed, the firmware's `RS485_ECHOES_OWN_TX` was set to `1` while this module does not echo — the discard window swallowed the reply and every read reported a timeout. A unit was declared dead and a warranty claim drafted; it answered normally when swapped back in later.

### 6 · Both probes on one bus

Runs: **WaterSensorNode.ino** · all `ENABLE_*` on · optionally **RS485Config/**

The two probes' factory settings collide on address and differ on baud rate, which normally means reconfiguring one of them. It does not here: `SoftwareSerial` can change rate at runtime and the two are never read in the same instant, so the firmware switches the bus rate before addressing each one. Both keep their factory settings and **nothing is written to either device**.

The rate difference also separates them despite the shared address: a frame sent at the wrong rate arrives as framing garbage with a broken CRC, and Modbus requires a valid CRC before a slave acts. Moving the Y511-A to `0x03` via register `0x3000` is still worth doing as hygiene — `RS485Config` does it, and refuses unless exactly one device is present — but nothing needs it to work.

> [!TIP]
> **Gate**
>
> ```
> 3.3 V sensor rail ON  (D6)
> 12 V sensor rail ON   (D5)
> Turbidity: start measurement (acknowledged)
> Temperature (DS18B20): 22.19 degC
> DO: warming up optics...
> Dissolved oxygen: 8.77 mg/L
> Temperature (DO probe): 24.91 degC   (DS18B20 differs by 2.73 K)
> Isolators ON          (A1)
> pH: 5.65                          (probe still in storage solution)
> Conductivity: 0.0 uS/cm
> Isolators OFF
> Turbidity: waiting 1 s for wiper/optics
> Turbidity: 2.36 NTU
> Both sensor rails OFF
> Battery: 3.94 V
> LoRaWAN send ... OK
> ```
> **This is the `VERBOSE_LOG 1` log** — the bench setting. The firmware is *meant* to ship with it at `0`, and **the working copy has been at `0` since 15 October 2026** — set back after the October bench runs, and not yet compiled since, so build it once and check the flash use before the next upload. At `0`, where the rail transitions, the isolator lines, the warm-up notice and the downlink chatter are all compiled out and only measured values, `FAIL`s, the battery, the send result, the hibernation notice and a terse `sleep 1800` remain. **Set it to 1 before using this guide as a checklist**, or the lines below will not be there to check.
>
> No `FAIL` line, and **no `re-arming` line** — that one only appears when the first turbidity read times out and the sketch makes its one real second attempt. If it appears followed by a value, the probe dropped off briefly and the reading was saved; followed by `FAIL`, the fault is supply or wiring.
>
> **There is no brush line, and that is correct.** The wiper still sweeps — listen for it right after the 12 V rail comes up. It is the probe doing that by itself.
>
> **`Isolators ON` appears late, and that is also correct.** The two pairs draw **130 mA measured** together, and pH plus EC take under two seconds, so `A1` is raised only for that window instead of for the whole cycle. Over 48 cycles a day that saves about **78 mAh** — near enough to everything else on the node put together, which comes to roughly 89 mAh a day. (This guide carried 165 mA and 100 mAh; both were estimates made before the pair was metered.) If the two isolator lines bracket anything other than the pH and EC reads, something has been reordered.

> [!NOTE]
> **Why the brush is no longer commanded**
>
> **The Y511-A sweeps on its own, every time the 12 V rail comes up.** This was found by ear, not in the log, because the command is acknowledged either way. Two consecutive cycles were counted:
>
> | cycle *with* `activateBrush` | **2** strokes |
> | --- | --- |
> | cycle *without* it | **3** strokes |
>
> *Fewer* strokes when commanded. The command is therefore not additive: it arrives 3 s after the rail comes up (`RAIL12_SETTLE_MS`), lands in the middle of a motor that is already running, and **truncates the automatic sweep**. It was buying nothing and cutting the cleaning short, so `TURB_BRUSH_BEFORE_READ` is now `0`.
>
> Anti-fouling is unaffected — improved, if anything. The probe cleans itself once per power-up, which is once per measurement cycle, where the command was only scheduled every second cycle.
>
> The internal brush-*interval* register remains useless here for a separate reason: it only counts while the sensor is *powered*, and here it is powered for under a minute per cycle. An interval of 30 minutes would never once fire.
>
> **Do not assume the next probe behaves this way.** The command frame below still works and the code is still there behind the flag. Set it back to `1` and count strokes by ear if you fit a different unit.
>
> **And the frame is not ordinary Modbus.** Yosemitech sends its register-*count* field as `0x0000`. The sweep is `01 10 31 00 00 00 00 74 94`, verified on the bench as the only command that actually moves the wiper; a normal read of `0x3100`, or a `0x06` write to it, are different frames and the sensor ignores them.
>
> **Give the rail 3 s before the first command.** At 500 ms the sweep command returned `NO REPLY` — the probe had not finished booting, which reads as a protocol fault and is not one. `RAIL12_SETTLE_MS` is 3000.

### 7 · Gateway setup

Runs: **create\_wazigate\_device.js** or **update\_wazigate\_device.js** · WaziGate dashboard, browser console

The device uses WaziGate's built-in **XLPP codec**. A custom JavaScript codec is not an option: WaziGate
can only decode uplinks with one, never encode a downlink, and calibration over LoRa needs downlinks.

Two scripts, one result. Open the WaziGate dashboard, open the browser console (`F12`), paste the
whole file and press Enter.

- **`create_wazigate_device.js`** makes the device from scratch. Fill in the node's `DEV_ADDR` and
  `SESSION_KEY` (the `devAddr` and `LoRaWANKeys` arrays in `WaterSensorNode.ino`, as hex) first.
- **`update_wazigate_device.js`** brings an existing device up to date. Fill in `DEV_ADDR`.

Either one:

1. puts the device on the XLPP codec and registers it with WaziGate's LoRa service, which routes
   an address to the device whose settings were written last;
2. creates one named sensor per channel, with its unit. The XLPP codec fills sensors by their
   `xlppChan`, so the values land in these instead of in generic entries;
3. creates the actuators *pH calibration*, *EC calibration* and *Interval* on XLPP channels 20, 21
   and 22. The gateway packs an actuator into a downlink only if it has a channel
   (`meta.xlppChan`), and picks the XLPP type from its quantity and unit, so these must not be
   created by hand. The update script also gives existing actuators their channel, after resetting
   their old value to 0.

Running the update script twice is safe: it renames and completes, and never creates duplicates.
Older devices with the same address receive no data once a newer one is registered; both scripts
list them so they can be deleted in the dashboard.

| Ch | Quantity | Sent as | Resolution |
| --- | --- | --- | --- |
| 1 | Water temperature — DS18B20 | °C | 0.1 °C |
| 2 | Battery | V | 0.01 V |
| 3 | pH | direct | 0.01 |
| 4 | Dissolved oxygen | mg/L | 0.01 mg/L |
| 5 | Conductivity | whole µS/cm, concentration type (0–65535) | 1 µS/cm |
| 6 | Turbidity | NTU, capped at 327 | 0.01 NTU |
| 7 | Water temperature — DO probe | °C | 0.1 °C |
| 8 | Restart cause | once per start; see [Restart cause and the watchdog](#restart-cause-and-the-watchdog-channel-8) | — |
| 9 / 10 | pH / EC calibration result | sent once, after a calibration | — |

1 µS/cm sits just under the EZO-EC's ±2 µS/cm near 100 µS/cm, where Lake Victoria sits. Lake
turbidity stays far below the 327 NTU cap; a reading of exactly 327 means off the scale.

*History: an earlier version sent conductivity as µS/cm ÷ 100 and turbidity as NTU ÷ 10 and undid the
scaling in a custom gateway decoder (`decoder.js`, `add_decoder_to_wg.js`, kept in
[`legacy/`](../legacy/) for reference). That decoder is also why no downlink ever reached the node. After the switch to XLPP, pushes
still did not arrive until 22 October 2026: the actuators had been created without a channel, and
the gateway leaves those out of the downlink.*

> [!TIP]
> **Gate**
>
> The dashboard shows the named sensors with conductivity in µS/cm and turbidity in NTU. A channel whose sensor failed is **absent** from the payload rather than zero, so do not fill gaps with zeros downstream: an oxygen sensor that dropped out would then look like a fish kill.

### 8 · Calibration — still open

> [!TIP]
> **New 19 October 2026 — calibration over LoRa, no cable needed**
>
> Only **pH and EC** need calibrating in the field; DO and turbidity are factory-calibrated and the DS18B20 needs none. Both EZO circuits keep their calibration in their own EEPROM, so the node only forwards the command.
>
> **How:** put the probe in the buffer, then push a number to an actuator in the dashboard. It reaches the node right after its next uplink. The node waits until the reading has been steady for 10 s (up to ~2.5 min), refuses anything implausible for that buffer, and only then calibrates. **Only a changed value is acted on**: every downlink carries all three actuators, so to repeat a value push `0`, wait one uplink, then push it again.
>
> | Actuator | Push | Does |
> | --- | --- | --- |
> | *pH calibration* | `7`, then `4`, then `10` | mid (always first, wipes the rest), low, high |
> | | `-1` | clear |
> | *EC calibration* | `1`, then `84` (or `1413`) | dry (probe in air), then single point in µS/cm |
> | | `2` | clear |
> | *Interval* | `1` … `60` | sampling interval in minutes; `0` = firmware default |
>
> In the serial log each item shows as `Downlink ch 20 = 7.00`, or `Downlink ch 21 unchanged`.
>
> **Answer** on the next uplink, sensor *pH / EC calibration result*: **0–3** points now stored, **90** implausible (wrong buffer, or still in lake water), **91** no answer, **92** never settled (leave it in longer, push `0`, then the value again), **93** battery too low. After a calibration the node cycles every ~5 min for half an hour, so the next step does not wait 30 min.
>
> **Firmware:** `ENABLE_LORA_CAL 1`. Since 21 October 2026 it fits together with `VERBOSE_LOG 1`, so the bench log and remote calibration can run at the same time. *(Until then the two did not fit in flash together.)* **Correction:** the single-point EC command is `Cal,<value>`, e.g. `Cal,84`. `EZOCal` documented `Cal,one,<value>`, which is not an Atlas command; check `Cal,?` if an earlier single point is in doubt.

Runs: **EZOCal/** · **VccCal/** · buffer solutions · a multimeter

Everything above proves the node reads and transmits. None of it makes the numbers *true*. Until this stage is done the pH and EC figures are readings, not measurements.

- **Use `EZOCal`, not `EZOBridge`.** The circuits are in I²C mode now. A UART console gets silence from them, and the silence says nothing about why.
- **pH** — `Cal,mid,7.00`, then `Cal,low,4.00`, then `Cal,high,10.00`. Mid *first*: it clears the other two, so the reverse order throws away the work. Submerge both the bulb and the reference junction, about 2–3 cm; a narrow vessel reaches that on 20–30 mL. Rinse with distilled water between buffers and shake off rather than wipe — rubbing charges the glass and the drift takes minutes to settle. **Never dilute a buffer**, and never store the probe in distilled water.
- **EC — and the standards on hand do not cover this water.** The three bottles are 1413 µS/cm, 12.88 mS/cm and 111.3 mS/cm — a decade series of KCl standards. **Read the units off the label**; two of the three are milli, and an earlier version of this guide and of the parts list had 111.3 as µS/cm, which would have put the calibration out by a factor of a thousand. Lake Victoria sits near **100 µS/cm**, so even the lowest bottle is 14× above the range this node measures in. **Order an 84 µS/cm standard before deployment.** Until then: single point on 1413, then read 12.88 mS/cm without recalibrating to see the linearity. Set the probe's `K` value first, then `Cal,dry` with the probe in air, then a standard. Without both of those the circuit answers **0.0 µS/cm** in any liquid, which looks like a dead probe and is not. Lake Victoria sits near 100 µS/cm, so an 84 µS/cm standard suits it better than the common 1413 µS/cm one.
- **Battery voltage — done, and here is the trap.** `VccCorrection` is **1.0** (this chip's bandgap reads 3.3129 V untrimmed, inside half a percent) and `BATT_DIVIDER` is **2.00**, the on-board 470k/470k ratio. A first attempt produced 2.696 because the ADC count came from a run whose battery domain sat near 3.0 V while the meter reading of 4.07 V was taken separately. **Both halves of a calibration must come from the same instant** — fitting one state's counts to another state's volts gives a plausible number that is simply wrong, and it wrongly discredited the board's own documentation.
- **Atmospheric pressure on the SEN0680**, register `0x1022`. Oxygen saturation follows the partial pressure of oxygen, and Lake Victoria sits at about 1135 m where pressure is near 88 kPa instead of 101 — roughly 13 % lower. Left at a sea-level default, every oxygen reading carries a systematic bias far larger than the sensor's own ±3 % FS. The value's unit and scaling are not on DFRobot's specification page, so look them up in the protocol document rather than guessing. Salinity, `0x1020`, stays 0 for fresh water.
- **Calibrate in the configuration you will measure in.** Calibrating without the pH isolator and then fitting one folds the offset into the calibration, where it is far harder to see.

---

## Restart cause and the watchdog (channel 8)

After every start the node sends **why it started**, once, on uplink channel 8. On the gateway it
appears as the sensor *Restart cause*. No new value means no restart, which is the normal state.

**The watchdog.** If the firmware stops reporting in for 8 s while the node is awake (a hung radio
call, a crash), it restarts itself. It runs on Timer2, not on the AVR's hardware watchdog, for two
reasons:
- **This board's bootloader cannot survive a watchdog reset.** It would loop forever.
- **The LowPower library takes the watchdog interrupt.**

Tested on 22 October 2026: with `WDT_TEST_HANG 1` the node restarted after 8 s and reported 101.
With the switch back at 0 it then ran without a single unexpected restart.

### Reading the value

The low values are reset flags and add up. 6, for example, is 2 + 4.

| Value | Meaning |
| --- | --- |
| 1 | power-on: the supply was connected or fully collapsed |
| 2 | external reset: the reset pin, i.e. the reset button, or the FTDI when flashing or opening the serial monitor |
| 4 | brown-out: the supply dipped below the reset threshold |
| 101–109 | watchdog restart; the last digit is the checkpoint passed last before the hang (table below) |
| 0 | restart without a reset and without the watchdog: a crash |

| Checkpoint | Last passed before the hang |
| --- | --- |
| 1 | start of a measurement cycle (`loop`) |
| 2 | radio initialisation |
| 3 | DS18B20 read |
| 4 | Modbus transmission (oxygen or turbidity probe) |
| 5 | EZO command (pH or conductivity) |
| 6 | a long wait (warm-up, wiper, rail settling) |
| 7 | radio send |
| 8 | radio receive |
| 9 | sleep |

### What is normal and what needs a look

| Seen | Assessment |
| --- | --- |
| 2 or 6 right after flashing | normal |
| 1 after connecting the cell | normal |
| **4 or 6 while nobody touched the node** | supply dips: a weak cell or a loose contact. Investigate. |
| **101–109** | a real hang. One is a hint; repeated ones are a fault to find, starting at the checkpoint named |
| **0** | a crash. Should never appear; investigate |

### In the serial log

- **At start-up:** `Restart cause: MCUSR=0x… WATCHDOG, hung after checkpoint 7 (radio send)`, or the
  reset flags in words.
- **Before every sleep:** `sleep 1800 s  (watchdog max gap 2987 ms)`. The second number is the
  longest time the watchdog went unfed in that cycle. It must stay well below 8000. Measured on
  the bench on 22 October 2026: **2,402 ms**, a margin of 5.6 s.

### Testing the watchdog

1. **Set `WDT_TEST_HANG 1` and flash.**
   - The second cycle stops with `WDT_TEST_HANG: stopping here`.
   - Within about 8 s the node restarts and reports `WATCHDOG, hung after checkpoint 1 (loop)`.
   - The gateway shows 101.
2. **Set it back to 0 and flash.** It would otherwise hang every second cycle. **Never deploy with 1.**

## A booting oxygen probe reports 0.00 mg/L, and nothing rejects it

> [!WARNING]
> **Found while measuring the warm-up**
>
> For the first **8 seconds** after the rail comes up, the SEN0680 answers Modbus *perfectly* — correct address, correct function code, valid CRC — and returns **0.00 mg/L**. It is not a timeout, not a fault, and nothing in the frame says the probe is not ready.
>
> **There is no sanity band on `v_do`.** Temperature has one (−20 to +60 °C, so `−127` and the DS18B20's 85 °C reset value are both caught). Oxygen has none. A cycle that reads one second early transmits **0 mg/L of dissolved oxygen from a fish pond** — which is not a missing reading, it is a fish-kill alarm. The gateway section of this guide (stage 7) already states the rule this breaks: a failed channel must be *absent*, never zero.
>
> **And the obvious fix is wrong.** Rejecting zero outright throws away the one reading the whole station exists to send: a genuinely anoxic pond really does read near zero. The discriminator has to be something else. The cheapest reliable one is the probe's own temperature register — a booting probe returns 0.00 there too, and 0.00 °C in Lake Victoria is impossible. **Both registers exactly zero = not ready. A real zero comes with a real temperature.**
>
> Open, because which way to fail is a decision about the station, not about the code.

## The real floor is 3.5 V, not 3.0 V

> [!WARNING]
> **Measured on the bench supply, 2026-10-09**
>
> | Cell | What happens |
> | --- | --- |
> | 3.6 V | all values transmitted |
> | 3.5 V | all values transmitted — **this is the working floor** |
> | 3.4 V | boots and transmits, but **pH, EC and turbidity are missing** |
> | < 3.4 V | will not start — brown-out bootloop |
>
> **What fails at 3.4 V is not the regulator, it is sag under load.** The node still boots and still completes a LoRa uplink there. What drops out are exactly the three measurements with a heavy supply behind them: the isolators at 130 mA and the 12 V converter, which draws on the order of **400 mA at the input** to make 12 V from 3.4 V. That current runs through the series **SS34**, which drops roughly **0.45–0.5 V** at 400 mA. The cell is fine; the path to it is not.
>
> **So `D1`, not the TVS and not `H1`, is what costs the bottom of the cell.** An ideal-diode P-MOSFET in its place — an AO3401A at 50 mΩ drops 20 mV where the Schottky drops 500 — moves the floor from about 3.5 V to about 3.2 V. **This was done on 9 October 2026** — on the carrier that position is `Q5` with `R13` holding its gate to `GND`, and there is no Schottky on the board at all. The paragraph above describes the jumper-wire node, which still has one. On the flat part of a lithium discharge curve that is a large share of the remaining charge. The energy saving alone is modest, 6.1 mAh a day of 98, but the headroom is the point.

> [!WARNING]
> **Your meter is changing the result**
>
> At 3.4, 3.5 and 3.6 V the node went into a bootloop *with the multimeter in series* and ran correctly without it. That is **burden voltage**: in a milliamp range the meter's shunt is ohms, and the 160 mA peak puts hundreds of millivolts across it — straight out of the node's headroom. The 10 A range has almost no burden, which is why that one worked, but it resolves only 1 mA.
>
> So **every sleep-current figure taken near the floor is suspect**, and the 3.0 V point in the TVS test above should be repeated at 4.2 V and 3.8 V, where there is headroom to spare. To measure properly: put the meter in *voltage* mode across a 1 Ω shunt instead of putting it in the supply line, or keep the 10 A range and accept 1 mA resolution.

## Below the cut-off — what still drains the cell

> [!WARNING]
> **A brown-out is not a disconnect**
>
> The regulator is an **HE2031A33MPR**, specified **3.5 V to 25 V in**, 0.18 V dropout at 100 mA, 2.0 µA quiescent. In practice the node keeps going to about 3.0 V, because at a 1.5 mA load the dropout is far smaller than the specified figure. The series SS34 moves whatever that threshold is *up* by its forward drop, so the cell-side cut-off lands near **3.3 V**.
>
> **Nothing in this design then disconnects the cell.** The MCU stops; the drain does not. What is still connected below the cut-off:
>
> | Still drawing | Figure | Source |
> | --- | --- | --- |
> | TVS leakage across the battery rail | falls with voltage, never zero | datasheet, not yet metered here |
> | CN3791 solar charger, permanently on the cell | 22.5 µA | 29 − 6.5 µA, from the manual |
> | WaziSense remainder | 6.5 µA | manual |
> | A0 divider, 2 × 470 kΩ | 3.2 µA at 3 V | cuttable at the `BATT%` jumper |
> | Regulator quiescent | 2.0 µA | manual |
>
> There is **no jumper on the v2 to disconnect the solar circuit** — the manual lists adding one under “further improvement”. Without the TVS the floor is about **35 µA**, and a cell sitting at 3.0 V will outlast any plausible dark spell. With a TVS leaking a few hundred microamps the last ~125 mAh go in **weeks**, and then the cell is *deep-discharged*, not merely empty.
>
> **The cell is unprotected — confirmed.** There is no PCM, so nothing in this node stops a flat cell being destroyed, and that is the one remaining hole in the energy path. The cheap fix is firmware, not hardware — `A0` is already read, so a hibernate threshold near 3.5 V that stops measuring and transmitting and wakes hourly only to check the voltage removes the 65 mAh/day active term and lets the panel climb back out.

## What cost the most time

Seven things, in the order they would have saved the most.

### Re-run the basic test after every fix, before writing a new tool

The oxygen sensor was silent for two reasons in sequence — a starved 12 V rail, then a wrongly aimed scan. Both were corrected, but nobody re-ran the scan afterwards: three further diagnostic sketches got written in that gap, and the sensor had been answering the whole time. A repeated basic test is cheaper than any new tool.

### Measure the defaults a manual promises

The WaziSense manual states the `I2C_J` jumper ships closed and the I²C pull-ups are therefore fitted. On this board it was open and both lines read **5.9 MΩ**. Re-reading the manual would have reproduced the wrong conclusion indefinitely.

### Judge every voltage under load

No-load readings misled three separate times: a 12 V rail that read 12 V unloaded and sagged to 7.2 V with a sensor on it; an I²C line that read idle-high but could not be pulled low; documented 4.7 kΩ pull-ups that measured megohms.

### Configure devices one at a time — some mistakes are one-way

Two EZO circuits share two wires, and a mode-switch command reaches both. Two Modbus probes ship on the same address. Every configuration tool here refuses to act unless exactly one device answers, because the recovery from getting it wrong is a hardware procedure on every affected board.

### "Little-endian" is ambiguous — print all four byte orders

A Yosemitech reading was invisible while the scanner showed only two permutations: its floats arrive fully reversed, so `10 5C CA 41` is the float `41 CA 5C 10` = 25.29. Both columns showed nonsense while the device was answering perfectly. Two devices on the same bus here disagree — one ABCD, one DCBA.

### A blocking library call looks exactly like a dead sensor

A log that ends mid-cycle with no message is not the sensor it was about to read; it is a call that never returned. A missing I²C pull-up does that. Suspect the call before the sensor.

It happened a second time from the other direction: plugging an isolator in backwards stopped the sketch dead. The core *had* a timeout — the sketch had hidden it behind a macro that does not exist (Stage 1). A guard that is never true is worse than no guard at all, because in the source it reads like protection.

### A logging macro that discards its argument discards the work inside it

Trimming the log for deployment meant wrapping lines in `LOGLN()`, which expands to `do {} while (0)` when `VERBOSE_LOG` is 0. Two of those lines did real work in the middle of the print:

Serial.print(turbStartMeasurement() ? F("ack") : F("NO REPLY"));

Gated as written, **the turbidity measurement would never have been started**, and the only symptom would have been a sensor quietly reporting stale or failed readings. The call is now hoisted into a local and only the printing is conditional. **Nothing with a side effect goes inside that macro.**

The same change had a second, visible bug: a print is usually three statements, and gating only the first leaves the tail printing on its own. The log filled with orphans — `15 s for wiper/optics`, `3600 seconds.`, `acknowledged)` — which is how it was caught. Gate the whole group or none of it.

### A library that transmits does not necessarily sleep afterwards

Neither `sendLoRaWAN()` nor `receiveLoRaWAN()` returns the SX1276 to sleep — it sits in STANDBY, **measured at 2.3 mA**, for the whole 30-minute interval. That was more than twice everything else on the node put together, and it survived every wiring fix because nothing in the log mentions the radio after the send. One call to `sx1272.setSleepMode()` before power-down removed it. **Sleep current is measured, not inferred**, and the FTDI must be off when you measure it, because it feeds the battery domain and hides exactly this.

### A power-gated node breaks anything with an internal timer

The turbidity sensor's brush interval, the EZO circuits' sleep command, the MAX31865's configuration register — all assume continuous power. On a node that powers its sensors for under a minute an hour, each has to be commanded explicitly on every cycle instead.

---

## Open before deployment

| Item | Why it matters | Effect |
| --- | --- | --- |
| **Gate the boost converter — decided** | A **Pololu #2810** now sits in the converter's *input*, driven from the `D5` pin, and the opto module on the output is gone. (`D7` is not brought out as a logic pin on this board — only as the Sensor Power 2 terminal pair.) The converter is genuinely unpowered between measurements rather than idling. The replacement converter is a **U3V9F12**, fixed 12 V — the old one failed by putting out 30 V at a 3.6 V input. **Sleep current, in order:** 20.8 mA before any gating, 1.08 mA with the WaziSense wired straight to the cell, **1.46 mA** on the present bench build. Disconnecting the isolator's `VOUT` drops that to 1.36 mA and the module LED goes out — 0.1 mA back-feeding through the LED's ESD diodes. The manual’s own figure for the bare board is **29 µA** asleep with the solar circuit active and **6.5 µA** with it inactive — so the board is 2 % of what is measured and the search belongs everywhere else. Current budget: | 20.8 → 1.46 mA |
| **Hibernation below 3.6 V — built** | The node now stops measuring below **3.60 V** at the cell and resumes above **3.80 V**, reporting only its own battery voltage once an hour. The thresholds come straight off the bench: everything transmits at 3.5 V, pH/EC/turbidity drop out at 3.4, below 3.4 it bootloops. Stopping at 3.6 keeps a margin and, more to the point, keeps the station *talking* while it is in trouble rather than bootlooping silently. Every reading stays `NAN` and is therefore **absent** from the uplink, never zero. It had to be paid for: the sketch was at 99 % with 102 bytes free. Twenty narrative log lines are now wrapped in `LOGLN()` behind `#define VERBOSE_LOG 0`, which frees about 790 bytes. **No line was deleted** — set the flag to 1 and the old log comes back verbatim. The one message that is *not* gated is the hibernation notice itself: why a station stopped measuring is a report, not narration. | < 3.60 V |
| **Where the sleep current actually goes** | Everything accounted for so far, against **1460 µA measured**: \| WaziSense v2 itself, per its manual \| 29 µA \| \| --- \| --- \| \| I²C pull-ups, from the 2.25 V line measurement \| 447 µA \| \| *of which traced to the isolator branch* \| *100 µA* \| \| **still unexplained** \| **984 µA** \| **The TVS was the prime suspect and has been cleared.** Measured from a bench supply: **4.2 V → 1.46 mA, 3.0 V → 1.17 mA**. A purely resistive load would have given 1.04 mA at 3.0 V; avalanche leakage collapses far faster than linearly. The reading came in *above* the resistive line, so the SMBJ5.0A can be at most about 0.4 mA of the total and is probably far less. **`D2` stays on the carrier board.** Fitting I(V) = a + b·V to the two points gives a constant **445 µA** plus a voltage-dependent **242 µA/V**. The constant term matches the 447 µA computed from the 2.25 V line measurement to within 2 µA — two points cannot prove a model, but the pull-ups are the one load that must sit in the constant term, because they hang on the regulated 3.3 V. | TVS cleared |
| **Measure the DO settling time** | **Measured — and the answer is to wait LESS, not more.** `SettleTime/` logged the probe every 2 s for four minutes from rail-on: silence (valid Modbus replies of `0.00`) for 8 s, first real value **8.36 mg/L** at 10 s, then a slow monotonic sag to **8.13** by 200 s, where it sat to ±0.01. \| Warm-up \| Reading \| Error vs final \| of the sensor's ±0.6 mg/L \| \| --- \| --- \| --- \| --- \| \| 10 s \| 8.36 \| 0.23 \| 38 % \| \| 15 s \| 8.33 \| 0.20 \| 33 % \| \| 30 s ← today \| 8.29 \| 0.16 \| 27 % \| \| 72 s \| 8.23 \| 0.10 \| 17 % \| \| 150 s \| 8.15 \| 0.02 \| 3 % \| **30 s of the node's 48 s active window goes on this one delay** — 62 % of everything the node does awake. The whole drift it is waiting out, 0.23 mg/L from the first valid reading to the final one, is a third of what the sensor can resolve. Cutting to **15 s costs 0.04 mg/L and saves 20 mAh a day**, a fifth of the budget, taking autonomy from ~25 days to ~32. **The tool's own headline was wrong, which is worth recording.** It reported “settled after 72 s, set `DO_WARMUP_MS` to 77000”, because it was asked when the reading stops moving. The question that sets a warm-up is when the reading gets closer to the truth than the sensor can resolve — and that happens at 10 s. The tool now reports both and recommends the second. | 30 s → 15 s |
| **Turbidity vs temperature — settled** | No milk needed in the end: submerging the probe was already a large enough optical change. `0x2602` went from **0.08 NTU in air to 8.88 NTU under water** — a factor of 111 — while both temperature readings moved by 0.06 K. No temperature register behaves that way, so `0x2602` is turbidity and the pairing with `0x2600` holds. | 0.08 → 8.88 NTU |
| **Isolators — two fitted, windowed** | A loop needs two grounded ends, so the pH one alone would do; the EC one is the first to remove if autonomy ever binds. The pH pair draws **57 mA** and the EC pair **82 mA** at 3.3 V per the datasheet, and **130 mA was measured for the two together**, and switching that onto the board's own 3.3 V regulator browns the MCU out at every rail switch-on — so both run off the cell behind **#2810 (B)** on `A1`. **Their MCU-side ground goes to `D6(−)`, not to board ground**: on board ground it cost a measured 0.4 mA of sleep current. A healthy board reads about **47 kΩ** from `D6(−)` to `GND` — that is the low-side switch standing open, not a fault. Near zero means the two have been bonded somewhere and `D6` can no longer switch anything. `A1` is now raised only for the pH and EC reads — about four seconds instead of fifty. **Settled:** the isolated sides do carry their own I²C pull-ups, which is why `R11` and `R12` are left unfitted on the carrier. Also measured: the isolated output is **regulated at 3.825 V** with either 3.0 V or 3.7 V on the input, so feeding the isolator from the raw cell is safe — the `#2810` in that path is there to switch, not to regulate. | ≈78 mAh/day saved |
| **Antenna** | Bring-up ran on a patch antenna, which is why RSSI wandered between −94 and −115 dBm. Those figures say nothing about the link from a pole at the lake. | re-measure on site |
| **LED currents** | `L,0` turns the EZO status LEDs off. Anything on the switched rail costs almost nothing — the continuously powered ones are what matter, and there are now three: the boost converter's, and **one on each Pololu #2810**. The #2810 LEDs were observed lit right through sleep with the switch off, so they sit on the input side; the datasheet's ~210 µA/V puts them at roughly 0.8 mA each at 3.9 V. Confirm against a sleep-current measurement before desoldering anything. | a few mA, continuous |
| **Ask Yosemitech for the wiper's rated life** | The probe sweeps itself once per power-up — at a 30 min cycle that is **48 sweeps a day**. Energy is negligible at 6 J per sweep, but mechanical wear is unpublished. **Note that software no longer has a lever here:** `TURB_BRUSH_EVERY_N` only ever throttled the *commanded* sweep, which is now disabled. The only way to reduce wiper cycles is to lengthen `sleep_sec`, which reduces every reading with it. | maintenance interval |

## Airtime

Measured time on air is **2.45 s** per uplink at SF12. On a 30-minute cycle that is **0.14 %** duty cycle against the EU868 limit of 1 % — a factor of seven in hand, and still legal at a four-minute interval.

One thing to carry forward: 48 uplinks a day at 2.45 s is **115 s of airtime daily**. On your own WaziGate that is your budget to spend. On a public network it would breach The Things Network's fair-use policy of 30 s per day by a wide margin.
