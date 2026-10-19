# Carrier Board in EasyEDA Pro (superseded)

From an empty project to ten assembled boards at JLCPCB — written for someone who has never opened an EDA tool, and only for this board.

- 6 October 2026 · companion to the carrier schematic

> [!CAUTION]
> **Historical document: do not build from it**
>
> This walkthrough was written for drawing the carrier in EasyEDA Pro. The board was drawn in KiCad
> instead and is finished; see [Sensor Carrier](sensor-carrier.md) and [`pcb/README.md`](../pcb/README.md).
> It is kept for its research. Two of its figures are **wrong** and would produce scrap:
>
> - **Every module footprint's pad row below is mirrored.** The order as the module sits on the
>   board, component side up, read left to right:
>
>   | Module | Row | Left → right |
>   | --- | --- | --- |
>   | BE-IVI isolator | MCU side | `VCC OFF GND A B` |
>   | | isolated side | `3.9V N/C GND A B` |
>   | EZO circuit | bus side | `GND TX RX` |
>   | | probe side | `VCC PRB PGND` |
>   | Pololu U3V9F12 | single row | `VIN GND VOUT` |
>   | MAX3485 module | single row | `VCC TXD RXD GND` |
>
> - **The Pololu's pin labels are printed on its back**, so reading them gives the mirror image.
>
> Where this walkthrough and [`pcb/out/FACTS.md`](../pcb/out/FACTS.md) disagree, FACTS.md is right.

Every menu path below was checked against EasyEDA's own documentation and JLCPCB's own capability and price pages, then checked a second time by someone whose job was to prove it wrong. Where the second pass found an error, the corrected version is what you are reading. Where nothing could be confirmed, it says so.

> [!WARNING]
> **Two things must happen before you start**
>
> **Measure the isolator and the MAX3485 module.** Outline, pad positions, pitch, and for the MAX3485 the A/B side as well. You cannot place a part whose size nobody knows, and no amount of software skill works around it.
>
> **Set the interface language to English.** Every name in this guide comes from the English documentation. On a German or Chinese UI the labels differ and you will spend the evening translating back.

## Stage 0 · Settings, and one library

EasyEDA Pro's hierarchy is **Project → Board → (Schematic + PCB)**. A schematic and a PCB only talk to each other if they sit under the *same* Board. Creating a project creates all three for you, so if you never touch the Board level you cannot get this wrong — and you should not touch it.

1. **Sign in and work online.** Offline mode costs you the LCSC search, the one-click order and any backup — all three are things the rest of this guide leans on. There is no reason to run this project offline.
2. Settings → **language: English**. Every path below comes from the English documentation; on another language none of them match.
3. Settings → set units to **mm**. The in-canvas shortcut `Q` toggles units later.
4. **Create a library first.** On the start page click **New Library**, or File > New > Library. Call it `KijaniSpace`. In offline mode there is no cloud account to own your parts, so without this step you have nowhere to save them.
5. File > New > Project. Board, schematic and PCB appear together.

> [!WARNING]
> **Pick the parts before you draw, not at export**
>
> Go through the BOM now and find each SMD part in the LCSC library, filtering for **Basic** or **Preferred Extended**. Note the LCSC number against each line.
>
> Do it the other way round — draw first, shop later — and you will be swapping footprints into a finished schematic, which is exactly how pin numbering and rotation errors get in. At ten boards the $3.07-per-unique-Extended-part fee is also the item most likely to dominate the bill, so the substitutions have to happen now, while they are free.

> [!NOTE]
> **Where your files actually live**
>
> The data directory is `Documents/LCEDA-Pro` — note *LCEDA*, not EasyEDA. Rather than trusting that, read the configured library and project paths in Settings, or right-click in the bottom Library panel and choose **Open the component Library directory**. Back that folder up; it is not in your project file.

## Stage 1 · Three parts nobody else has

The EZO circuits, the isolators, the Pololu regulator and the MAX3485 module are not in LCSC's library. You draw them once. This is the stage where a mistake becomes scrap, so it comes first and it gets checked before anything else is built on it.

For each part: File > New > Symbol, then File > New > Footprint, then File > New > Device to bind them together. In EasyEDA Pro a **Device = symbol + footprint + 3D model + properties**, and *only a Device can be placed* — a loose symbol or footprint cannot.

### The EZO circuit — Atlas gives you the recipe

The pH datasheet has a page called **Circuit footprint** that spells the construction out: place an 8-position header, then put a 3-position header at each end of it. The 8-position header is a measuring jig — eight positions at 0.1″ span exactly 0.7″, which is the row spacing. Delete it afterwards.

In coordinates, with the origin at the body centre and the long axis along X:

```
Body outline   20.16 × 13.97 mm   (0.79" × 0.55")
Drill          1.05 mm        Pad ∅ 1.8 mm, round, layer "Multi"

Row A   X = -8.89    Y = +2.54 / 0 / -2.54      GND  TX/SDA  RX/SCL
Row B   X = +8.89    Y = +2.54 / 0 / -2.54      VCC  PRB     PGND
```

> [!WARNING]
> **Count the pins on your own boards first**
>
> Three pads per row is what the datasheet's own footprint recipe produces. **Confirm it on the two circuits in front of you** — pH and EC — and note which end of each row is pin 1. A footprint with holes where the module has no pins is not a cosmetic error: the board is scrap.

### The Pololu U3V9F12 — the row is on the short edge

Body **13.6 × 8.6 mm**. The three pins sit on the **8.6 mm edge**, not the long one — Pololu's dimensioned photo puts the 0.34″ measurement across the pin row. Lay the footprint out with the pins spanning 5.08 mm (3 × 2.54 mm) across the short dimension, and **measure the pin-to-edge inset** on the part itself; that one number is not published.

Pin order from the board's own silkscreen: **1 = VOUT, 2 = GND, 3 = VIN**.

### Drawing the footprints

- Pad tool: `P`. Set the drill and the pad diameter in the right-hand property panel, and the layer to the through-hole option (the Pad Manager calls it **M**, the Layer Manager calls it **Multi** — read your own dropdown).
- Place pads by coordinate rather than by eye: press `C` for the command input and type the offset, or set the grid to 2.54 mm and snap.
- Silkscreen rectangle: `R`. **Not `Alt+R`**.
- Keep the silkscreen on the body edge so it never crosses a pad. Routing > Auto Trim Silkscreen exists but *only works on polylines* — it ignores rectangles entirely.
- Set the origin deliberately. The body centre is the easiest reference for the placement coordinates in the carrier plan.

> [!TIP]
> **The check that costs nothing and catches everything**
>
> When each footprint is drawn, **print it at 1:1 and lay the real module on the paper.** Pins through holes, body inside the silkscreen. Two of the footprint figures in this project were already wrong once — a five-per-row EZO and a Pololu rotated 90° — and both were caught by reading a datasheet twice. A sheet of paper catches what reading does not.

> [!WARNING]
> **Do not take a footprint from the Public Repository**
>
> EasyEDA's library panel includes a **Public Repository** and a **Std Edition Public Repository**. Those are user-contributed and **nobody vets the footprints**. For a board you are paying to have assembled, draw the part yourself or use the System Library, which is LCSC's own.

## Stage 2 · The schematic

1. Place parts: Place > Device, or `Shift+F`, or `S` for the bottom library panel. The search box has two engines — **LCSC Mall** shows only parts LCSC actually sells, **EasyEDA Pro** shows more. Use LCSC Mall for anything you want JLCPCB to place.
2. Wire: `W` from the menu Place > Wire.
3. Name a net by double-clicking the wire. In Pro a net label is *not* a separate object — it is the wire's name attribute shown on screen. Place > Net Label or `N`.
4. Second sheet: File > New > Page, or right-click the project.

Work through the three sheets of the carrier schematic in order: power and the two switches, then the 3.3 V domain, then the isolators. Name every net exactly as the netlist does — `SWGND`, `ISO1_GND` and `ISO2_GND` especially, because those names are what keep the four copper pours apart later.

> [!WARNING]
> **Cross-page nets are a project setting in v3**
>
> Two wires with the same name on different pages are *not* automatically one net. Open Design > Project Setting and check the connection scope for net labels is project-wide. Then verify on the PCB side that a ratline really exists between the two pins — the Nets list in the left panel, or the unrouted count. Do not trust the schematic for this.

### Four grounds, or you have built an ordinary board

This is the one that undoes everything if you miss it, and it happens silently.

**The standard ground symbol carries the net name `GND`.** Place it four times and you have *one* net, not four. The schematic looks right — four ground symbols, one per domain — and at transfer to the PCB the four copper regions you so carefully drew become a single plane. No warning, no DRC error: electrically it *is* one net, so nothing is violated.

So give each domain its own explicitly named net:

| Domain | Net name | What returns to it |
| --- | --- | --- |
| permanent ground | `GND` | cell, converter, probe terminals, capacitors |
| switched return | `SWGND` | MAX3485, DS18B20, bias pull-down, both isolators’ MCU side |
| isolated, pH | `ISO1_GND` | EZO-pH only |
| isolated, EC | `ISO2_GND` | EZO-EC only |

**Then count them.** Open the Nets list in the PCB editor’s left panel after transfer and confirm all four exist as separate nets with the pins you expect. This is a manual gate because no automatic check will ever flag it.

When the schematic is done: Design > Schematic to PCB. That is the **first** conversion only. Every later change goes across with **Update PCB**, or from the PCB side Design > Import Changes from Schematic — and in that dialog tick **Also update wire nets**, or traces you have already routed keep their old net names.

## Stage 3 · Outline and placement

1. Place > Board Outline, rectangle. Set the grid to 1 mm and draw 100 steps by 100 steps, or press `C` while drawing and enter the increments numerically. Verify with Tools > Measure Distance before you trust it.
2. Design > Design Rules — set these before routing, so the defaults are right and real-time DRC flags violations as you draw. It does *not* stop you drawing them.
3. Place the modules first, in the regions from the placement plan, then the passives around them.

### Decide the board itself first

| Setting | Choose | Why |
| --- | --- | --- |
| Layers / thickness | 2 / 1.6 mm | Economic PCBA only accepts 0.8–1.6 mm. A 2 mm board forces Standard PCBA and about $50 more |
| Copper | 1 oz | standard stack-up only on Economic |
| Finish / colour | lead-free HASL, green | other colours are quantity-banded |
| Order number | “specify a location” | the free option — you choose an unimportant spot instead of being surprised by one |
| Assembled side | **top** | see below — this is not arbitrary |

> [!WARNING]
> **SMT on top, and the reason is your soldering iron**
>
> Economic PCBA places on one side only. Put it on the **top**, with the through-hole module bodies also on top and their joints soldered from below. Then your iron works on a bare surface.
>
> Choose the bottom for SMT and you will be hand-soldering at 350 °C in among already-placed 0402s.

> [!NOTE]
> **100 × 100 mm is exactly on a price boundary**
>
> A sloppy frame of 100.2 mm jumps JLCPCB into the next size class. Draw it on the grid, measure it, and after uploading the Gerbers **check the board size JLCPCB reports back**: it must say 100.00 × 100.00 mm and 2 layers. That echo is the only dimensional check that actually binds.

| Rule | Value | Why |
| --- | --- | --- |
| Track / clearance | 0.2 mm | JLCPCB allows 0.1 mm; 0.2 gives you margin for nothing |
| Via drill / diameter | 0.3 / 0.6 mm | capability is 0.15 / 0.25 — this is a safe margin |
| Copper to board edge | ≥ 0.3 mm | capability 0.2 mm |
| Pad to silkscreen | 0.25 mm | capability 0.15 mm; the default 6 mil already passes, but legibility is worth the extra |
| Silkscreen text | ≥ 1.0 mm | below this the fab may drop it |
| PTH annular ring | ≥ 0.25 mm | minimum is 0.20 mm |
| Copper to board outline | ≥ 0.3 mm | **this rule also governs the slot**, because the slot lives on the outline layer |

## Stage 4 · Four grounds and one slot

This is the stage the whole board exists for, and it is the one a beginner cannot guess. Four copper regions that must never touch: `GND`, `SWGND`, `ISO1_GND`, `ISO2_GND`.

### The pours

1. Place > Copper Region, polygon. Draw each region around the area it owns. You may draw past the board edge — EasyEDA trims it at the frame.
2. **Set the Net property immediately after drawing each one**, in the right-hand panel. Also give each a Name like `SWGND_TOP`; the Copper Manager lists them by name.
3. Repeat per layer. Top and bottom are separate regions.
4. Tools > Copper Manager — check all four net names are distinct. **Two regions that accidentally share a net silently merge into one plane, and DRC will not complain**, because electrically it is one net. This is the error that would undo the entire design.
5. Press `Shift+B` after every change. Copper does not reflow on its own; the screen can be right while the Gerbers are wrong.

> [!WARNING]
> **Use Copper Region, never Fill Region**
>
> EasyEDA's own page says a Fill Region *“cannot create gaps with elements of different nets”*. It would short straight across any foreign trace crossing it.

### Keeping an isolated pour alive

A copper region whose net has no pad or via inside it on that layer is classed as dead copper and silently deleted on rebuild. Your two isolated grounds are exactly that case.

**The fix is not to switch Keep islands on.** Make sure at least one pad of that net sits inside the region on that layer — which it will, because the EZO's ground pin is there. Leave **Keep islands = No** so stray fragments near the barrier are still removed, press `Shift+B`, and look at what survived.

### Thermal relief, or you cannot solder the board you ordered

Each copper region has a **connect-to-pad** property: direct connection, or a thermal spoke. **Set every pour to thermal spokes.**

A through-hole pad tied directly into a 1 oz plane the size of this board is a heatsink. A normal soldering station cannot bring it to temperature, so you hold the iron there longer and longer until the pad lifts. Every module on this board is hand-soldered through-hole — this setting decides whether the boards are assemblable at all.

### The isolation gap

Different nets are held apart automatically — but only at the default clearance, around 0.25 mm, which is nothing like an isolation distance. Two named parameters govern it:

- Design > Design Rules → **Copper Zone → Network Spacing** — pour to foreign-net elements.
- **Copper Zone → Distance To Border Outline** — pour to the board edge *and to the slot*.

Raise both via a **Net-to-Net rule** between `GND` and each isolated ground, not by raising global clearance — that would wreck the routing everywhere else. Then measure the resulting gap on both layers.

### The slot

Place > Slot Region, rectangle, under each isolator’s barrier. It exports into `Gerber_BoardOutlineLayer.GKO`, which is what JLCPCB expects.

> [!WARNING]
> **Work the slot width out, do not copy a number**
>
> The milling tolerance on a non-plated slot is **±0.2 mm**. So a slot drawn at 2.0 mm can arrive at 1.8 mm. If the creepage distance you need is 2.0 mm, the slot must be drawn at **2.2 to 2.4 mm**:
>
> ```
> nominal slot = required creepage + 0.2 mm   (minimum)
> ```
> Take the required figure from the isolator’s datasheet and whatever standard applies to you — not from this guide, and not from EasyEDA or JLCPCB, neither of which knows what you are isolating.

> [!WARNING]
> **Nothing crosses the barrier — and this is the whole point of the board**
>
> Over the slot and over the gap between two ground regions there goes **no trace, no pour spur, no via, no silkscreen, not even a dimension line**.
>
> There is a consequence that is easy to miss on a 2-layer board: with four separate grounds there is *no continuous return plane*. Any signal that crosses a domain boundary needs a defined return path — which is what the isolators are for. A signal that simply runs across the gap as a trace has no return at all, and you have paid for an isolation barrier that a single track walks straight through.

> [!NOTE]
> **Size it against the real tolerance**
>
> JLCPCB's binding figure for a non-plated slot is **±0.2 mm**, not the ±0.1 mm quoted in blog posts. Minimum width is 1.0 mm, so 2 mm is comfortable. Design the copper keepout around the slot independently, so the barrier never depends on the milling tolerance.
>
> There is a second, accidental route to a slot: a second board frame drawn inside the first becomes one. Use Slot Region deliberately instead.

## Stage 5 · DRC

1. Design > Check DRC. Violations land in the bottom panel with an X on the board.
2. Your isolated grounds may raise a complaint about a net connected to nothing else. **That is the intended result.** Use Design > Check DRC (Custom) to untick that specific check rather than silencing the whole run.
3. Design > Clear Errors wipes the markers once you have read them.

> [!WARNING]
> **Do not apply the standard fix to your isolated pours**
>
> The documented remedy for an “orphaned copper region” warning is to set Keep Island to No on that region. Applied to an isolated ground, that deletes exactly the copper you need. Read each warning before acting on it.

## Stage 6 · Export and order

> [!WARNING]
> **This decides your layout, so read it before routing**
>
> **Economic PCBA assembles one side only.** JLCPCB's capability table is explicit: Economic is *single-sided placement*; only Standard PCBA does both sides, at $51.12 setup instead of $8.18, and it refuses boards under 70 × 70 mm.
>
> So: **every machine-placed part on the top layer**, bottom layer for routing and ground only. The placement plan already does this — do not drift from it.

Three separate exports, or one combined route:

- File > Export > PCB Fabrication File (Gerber)
- File > Export > Bill of Materials (BOM)
- the Pick and Place / CPL export
- or Export > PCB Order, which pushes all three into the JLCPCB cart

> [!TIP]
> **How to keep the hand-soldered parts out of the order**
>
> EasyEDA has a “do not populate” attribute, but its name is not in the official docs and differs between versions. **Do not fight it.** Export the BOM and the CPL in full, open both CSVs, and **delete the rows for every through-hole module** before uploading.
>
> JLCPCB assembles exactly what the BOM lists. A part that is not in the file does not exist for them, so there is nothing to mark and nothing to go wrong. Two minutes, version-independent, and you can see in the file that it worked.

> [!NOTE]
> **Order Parts is not the BOM export**
>
> Export > Order Parts uploads an existing BOM to LCSC to *buy loose components*, billed separately from the assembly order. Easy to click by mistake.

### Before you pay

- **Check every part's rotation in JLCPCB's placement preview.** CPL rotation mismatches are the classic way to get ten boards with parts turned 90°. The red dots in that preview are *orientation markers, not pin 1* — JLCPCB says so themselves.
- **Filter the parts library by Basic and Preferred Extended.** Both are fee-free on Economic PCBA; plain Extended costs $3.07 per unique part number. Do not design against a remembered part count — the label on the quote page is the only authority.
- Tick **Confirm Parts Placement** ($0.45). For a first board it is worth it.
- Stay on 2 layers, 1.6 mm, green, HASL. Economic restricts thickness to 0.8–1.6 mm, standard stack-up only, and some colours are quantity-banded.
- Do not hand-panelise. Order the single design and let JLCPCB handle quantity.

### What ten boards should cost

| Item | Cost |
| --- | --- |
| PCB, 100 × 100 mm, 2 layers, 10 pcs | ~$5 |
| Economic PCBA setup | $8.18 |
| Stencil | $1.53 |
| SMT joints, $0.0016 each | ~$3 |
| Extended parts, $3.07 per unique number | aim for $0 |
| Confirm Parts Placement | $0.45 |
| **Before components and shipping** | **~$18** |

Add the parts themselves and shipping. **Choose DDP** — duty and import VAT settled up front, no customs paperwork landing on you. The $31 figure that gets quoted around is for the site’s default origin country and is not reliable for Germany; read your own quote. One coupon may be applied per order — they do not stack, whatever the coupon centre suggests.

All of these prices are from October 2026. **Check them against the live quote**, and budget without assuming a coupon.

### In the JLCDFM preview, look for three specific things

- **Is the slot there at all?** It is the one feature most likely to be lost in export.
- **Does the drill file contain every through-hole?** All four module footprints are through-hole and they are the parts you cannot re-order cheaply.
- **Are the four ground regions genuinely separate?** Last chance to catch a merge.

Use the order remarks: one sentence saying the internal slot is intentional, with a screenshot. That saves a query mail and three to five days. And answer JLCPCB’s review mail quickly — the job sits until you do.

## Stage 7 · When the boards arrive

> [!TIP]
> **Before anything is soldered and before anything is powered**
>
> **Meter the four grounds against each other.** `GND` to `SWGND`, `GND` to each isolated ground, and the two isolated grounds to one another. Every one of those must read **open**.
>
> This is a five-minute test on a bare board that tells you whether the central idea of the design survived manufacture. Do it before you spend an evening populating ten of them.

1. Hand-solder in height order: resistors and small parts first, modules last. The populated side must not rest on the bench while you solder — the placed SMD parts are on it.
2. Bring the first board up on a **current-limited supply**, not the cell.
3. Then the bring-up sequence from the runbook: bare board, cell only, WaziSense, then one module at a time.

> [!NOTE]
> **Archive before you move on**
>
> Gerbers are not a design you can edit. Keep, in one dated folder with a revision number: the project archive, your own library (`.elibz`), and *exactly* the Gerber, BOM and CPL files you uploaded. Put that same revision on the silkscreen, so a board in your hand can always be matched to the files that made it.

## Last · What is still unverified

Honest list, so you know where to look twice rather than trusting this guide:

- **The exact in-app wording** of several menu items. The documentation is machine-translated in places and is not version-stamped against the build you installed. The menu *structure* has been stable; individual words may differ.
- **The DNP / “do not populate” attribute name.** Official docs do not name it; forum sources call it a *Mounted* attribute. Find it before you rely on it for `R11`/`R12`.
- **Whether EasyEDA's DRC has a silkscreen-over-pad check at all.** Not in the documented check list. Inspect the silkscreen by eye.
- **Economic PCBA's maximum quantity** — the capability page says 2–50, other sources say 30. Not decision-critical at ten.
- **Hotkeys** differ between the quick-start and the hotkey reference, because they depend on the active hotkey theme. Read yours in Settings rather than memorising these.

> [!TIP]
> **The order to work in**
>
> Measure the two unknown parts → draw the four library parts and check them against the physical modules → schematic → outline and rules → placement → routing → the four pours and the slot → DRC → export. Do not start the schematic before the library parts exist; every later stage inherits their mistakes.
