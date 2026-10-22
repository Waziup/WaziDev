# Legacy

Superseded files, kept for reference. Nothing here is needed to build, deploy or calibrate the
node, and none of it should be used on a current build.

| File | What it was | Replaced by |
|---|---|---|
| [`decoder.js`](decoder.js) | Custom WaziGate JavaScript decoder: named the values and undid the old payload scaling (conductivity ÷ 100, turbidity ÷ 10). | WaziGate's built-in XLPP codec. The firmware no longer scales. Names and units come from [`create_wazigate_device.js`](../create_wazigate_device.js) / [`update_wazigate_device.js`](../update_wazigate_device.js). |
| [`add_decoder_to_wg.js`](add_decoder_to_wg.js) | The same decoder wrapped in a `POST /codecs`, to install it on the gateway. | as above |
| [`EZOBridge/`](EZOBridge/) | UART console to one EZO circuit over the SDA/SCL wires. It performed the UART → I²C switch. | [`EZOSwitch/`](../EZOSwitch/) for the switch, [`EZOCal/`](../EZOCal/) to talk to circuits that are in I²C mode. EZOBridge cannot reach those. |
| [`TurbBrush/`](TurbBrush/) | Experiment that made the Y511-A wiper sweep on command and read its interval. | Nothing: it settled that the probe wipes by itself on power-up, so the firmware no longer commands the wiper (`TURB_BRUSH_BEFORE_READ 0`). |
| `firmware-snapshots/` | Old copies of `WaterSensorNode.ino` (`.bak` … `.bak5`). Local only, never published. | the current `WaterSensorNode.ino` |

## Why the custom decoder cannot come back

WaziGate builds every downlink through the device's codec, and a JavaScript codec can only decode:
its encoder is not implemented in WaziGate v3. With the custom decoder assigned, no actuator value
ever reaches the node, so remote calibration and the interval setting would stop working. The
decoder also does not understand the current payload: it stops at the conductivity channel.
