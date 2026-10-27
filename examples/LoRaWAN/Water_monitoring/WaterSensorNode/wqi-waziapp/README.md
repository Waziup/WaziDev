# Water Quality Index — WaziApp

Shows the water node's **CCME Water Quality Index** on the WaziGate as one number and a traffic
light. The index covers the last 24 hours of temperature, pH, dissolved oxygen, conductivity and
turbidity, and refreshes every 5 minutes.

| Light | WQI | CCME category |
|---|---|---|
| green | 80–100 | Excellent (95–100), Good (80–94) |
| amber | 65–79 | Fair |
| red | 0–64 | Marginal (45–64), Poor (0–44) |

The light thresholds are set in `objectives.json` (`traffic_light`).

## How the index is calculated

`wqi.py` implements the CCME Water Quality Index. Every measurement in the window is compared with
an **objective** for its variable:

- **F1 (scope):** the share of variables that missed their objective at least once.
- **F2 (frequency):** the share of all measurements that missed.
- **F3 (amplitude):** how far the failed measurements missed.

WQI = 100 − √(F1² + F2² + F3²) / 1.732

**The objectives define the index.** The same data scores differently against other limits. The values in
[`objectives.json`](objectives.json) are starting values for Nile tilapia culture. **Replace them
with the applicable local guideline.** Each variable takes a `min`, a `max` or both, and both must be
greater than 0.

| Variable | Default objective |
|---|---|
| Water temperature | 22–32 °C |
| pH | 6.5–8.5 |
| Dissolved oxygen | ≥ 5 mg/L |
| Conductivity | ≤ 2000 µS/cm |
| Turbidity | ≤ 30 NTU |

**Limits of a sensor-based index:**
- It covers five variables only. It cannot see ammonia, nitrite, nutrients or bacteria, which
  matter most in aquaculture, so a good index does not rule those out.
- It is not comparable with lab-based indices such as the NSF-WQI.
- CCME asks for at least four variables with at least four measurements each. The page says so when
  the window holds fewer.
- Values recorded while the probes were out of the water, for example during calibration, enter the
  index like any other.

### From the command line

`wqi.py` also runs on its own, on a CSV export. The first column is the time; the other columns are
named like the variables in `objectives.json`. Other columns are ignored.

```bash
python wqi.py data.csv
```

## Install on the WaziGate (v3)

The app runs as a Docker container next to the gateway's own apps. The gateway builds it from this
folder.

1. **Copy this folder to the gateway** as `/var/lib/wazigate/apps/kijanispace.wqi`. The folder name
   is the app ID and must match exactly.
   ```bash
   scp -r wqi-waziapp <user>@<gateway-ip>:/tmp/
   ```
   ```bash
   ssh <user>@<gateway-ip> "sudo cp -r /tmp/wqi-waziapp /var/lib/wazigate/apps/kijanispace.wqi"
   ```
2. **Build and start it** on the gateway:
   ```bash
   cd /var/lib/wazigate/apps/kijanispace.wqi && sudo docker compose up -d --build
   ```
   On older systems the command is `docker-compose` instead of `docker compose`.
3. **Open it.** In the dashboard it appears under **Apps** as *Water Quality Index*. The page is also
   available directly at `http://<gateway-ip>/apps/kijanispace.wqi/ui/`.

**Which device.** The app picks the device that carries the node's sensor channels. With several such
devices it takes the most recently changed one. To fix the device, set `DEVICE_ID` in
`docker-compose.yml`, then run `docker compose up -d` again.

**Changing the limits.** Edit `objectives.json` in the app's folder on the gateway. It is read on
every refresh, so no restart is needed.

## How it fits into the gateway

| File | Role |
|---|---|
| `main.py` | Serves the page and `GET /api/wqi` on the unix socket `proxy.sock`. The gateway forwards `/apps/kijanispace.wqi/…` to that socket. Reads the sensor values from the gateway API on `127.0.0.1`. |
| `wqi.py` | The calculation, no dependencies. |
| `objectives.json` | Objectives, time window, light thresholds. |
| `ui/index.html` | The page: value, category, traffic light. |
| `docker-compose.yml`, `package.json` | What the gateway needs to run the app and list it in the dashboard. |
| `Dockerfile` | Builds the image, including the `/index.zip` the dashboard installer expects. |

The container uses the host network. The gateway API admits requests from `127.0.0.1` without a
login, so the app needs no credentials.

### Development without a gateway

```bash
python main.py --tcp 8080 --api http://<gateway-ip>
```

This serves the page at `http://127.0.0.1:8080/ui/`. A gateway reached over the network asks for a
login token, which you pass as the `WAZIGATE_TOKEN` environment variable.
