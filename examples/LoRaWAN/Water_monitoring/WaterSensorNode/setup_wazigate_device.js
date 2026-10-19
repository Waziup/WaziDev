/*
 * ============================================================================
 *  KijaniSpace water node - one-off WaziGate (v3) device setup
 * ============================================================================
 *  Run ONCE, in the browser console of the WaziGate dashboard while logged
 *  in (F12 -> Console, paste the whole file, Enter). Safe to run again: it
 *  renames and completes what is there and never creates a duplicate.
 *
 *  What it does:
 *    1. finds the water node - the device that still uses the custom codec
 *       "WaziUp-Water_Surveillance_Device" (or set DEVICE_ID below);
 *    2. switches that device to WaziGate's built-in XLPP codec;
 *    3. creates the sensors the XLPP codec will fill, one per channel, with
 *       real names and units - the codec matches them by meta.xlppChan, so
 *       new values land in these instead of in generic "analoginput"s;
 *    4. creates three named actuators: pH calibration, EC calibration,
 *       Interval.
 *
 *  WHY THE CODEC CHANGES. WaziGate can only build downlinks through the
 *  device's codec, and a JavaScript ("script") codec cannot encode - in v3
 *  its encoder is still a stub that returns an error, and wazigate-lora then
 *  drops the downlink. So with the custom decoder no actuator value ever
 *  reached the node. With XLPP it does. The firmware no longer scales EC or
 *  turbidity, so nothing needs a script decoder to undo it.
 *
 *  WHY THE ACTUATORS ARE PLAIN NUMBERS. In v3 the downlink is the number you
 *  push, as text, with no channel. The node tells the commands apart by the
 *  value itself - the names here are for the person pushing it:
 *
 *    pH calibration   7 (always first!), then 4, then 10 - probe in that buffer
 *                     -1 = clear the pH calibration
 *    EC calibration   -3 = dry (probe in AIR, dry), then 84 (or 1413) in that
 *                     standard, in uS/cm;  -2 = clear the EC calibration
 *    Interval         seconds, whole minutes only: 300, 600, 1800, 3600
 *
 *  The value is sent after the node's NEXT uplink, so put the probe into the
 *  buffer first, push the value, and leave it there until the next uplink
 *  shows the result on "pH calibration result" / "EC calibration result":
 *    0..3 = calibration points now stored     90 = refused, implausible
 *    91   = circuit did not answer            92 = reading never settled
 *    93   = refused, battery too low (hibernating)
 *
 *  Old sensors from the custom decoder (temperature_C, pH, oxygen_mgL ...)
 *  are left alone with their history. Delete them by hand once the new ones
 *  are filling.
 * ========================================================================= */

var DEVICE_ID = "";   // leave empty to find the device by its old codec
var OLD_CODEC_NAME = "WaziUp-Water_Surveillance_Device";

var SENSORS = [
  { chan: 1,  name: "Water temperature",     kind: "Thermometer",            quantity: "Temperature",  unit: "DegreeCelsius",     unitSymbol: "°C" },
  { chan: 2,  name: "Battery",               kind: "VoltageSensor",          quantity: "Voltage",      unit: "Volt",              unitSymbol: "V" },
  { chan: 3,  name: "pH",                    kind: "WaterPHSensor",          quantity: "PH",           unit: "",                  unitSymbol: "pH" },
  { chan: 4,  name: "Dissolved oxygen",      kind: "OxygenSensor",           quantity: "",             unit: "MilligramPerLitre", unitSymbol: "mg/L" },
  { chan: 5,  name: "Conductivity",          kind: "WaterConductivitySensor", quantity: "Conductivity", unit: "MicroSiemens",     unitSymbol: "µS/cm" },
  { chan: 6,  name: "Turbidity",             kind: "",                       quantity: "",             unit: "",                  unitSymbol: "NTU" },
  { chan: 7,  name: "DO probe temperature",  kind: "Thermometer",            quantity: "Temperature",  unit: "DegreeCelsius",     unitSymbol: "°C" },
  { chan: 9,  name: "pH calibration result", kind: "",                       quantity: "",             unit: "",                  unitSymbol: "" },
  { chan: 10, name: "EC calibration result", kind: "",                       quantity: "",             unit: "",                  unitSymbol: "" },
];

var ACTUATORS = [
  { name: "pH calibration", hint: "7 first, then 4, then 10 (probe in that buffer). -1 = clear" },
  { name: "EC calibration", hint: "-3 = dry (in air), then 84 or 1413 uS/cm in that standard. -2 = clear" },
  { name: "Interval",       hint: "seconds, whole minutes: 300 / 600 / 1800 / 3600", unitSymbol: "s" },
];

async function api(method, path, body) {
  var headers = { "Content-Type": "application/json" };
  var token = window.sessionStorage.getItem("token");
  if (token) headers["Authorization"] = "Bearer " + token;
  var resp = await fetch("/" + path, {
    method: method,
    headers: headers,
    body: body === undefined ? undefined : JSON.stringify(body),
  });
  var text = await resp.text();
  if (!resp.ok) throw new Error(method + " /" + path + " -> " + resp.status + " " + text);
  try { return JSON.parse(text); } catch (e) { return text; }
}

async function setupWaterNode() {
  // 1. the device
  var devices = await api("GET", "devices");
  var device;
  if (DEVICE_ID) {
    device = devices.find(function (d) { return d.id === DEVICE_ID; });
  } else {
    var codecs = await api("GET", "codecs");
    var old = codecs.find(function (c) { return c.name === OLD_CODEC_NAME; });
    var hits = old ? devices.filter(function (d) { return d.meta && d.meta.codec === old.id; }) : [];
    if (hits.length !== 1) {
      console.log("Could not pick the device automatically (" + hits.length + " match). Devices:");
      devices.forEach(function (d) { console.log("  " + d.id + "  " + d.name + "  codec=" + (d.meta && d.meta.codec)); });
      console.log("Set DEVICE_ID at the top of the script to the right id and run it again.");
      return;
    }
    device = hits[0];
  }
  if (!device) { console.log("No device with id " + DEVICE_ID); return; }
  console.log("Device: " + device.name + " (" + device.id + ")");
  var base = "devices/" + device.id;

  // 2. the codec - device meta is merged key by key, the LoRaWAN keys stay
  await api("POST", base + "/meta", { codec: "application/x-xlpp" });
  console.log("  codec -> application/x-xlpp");

  // 3. sensors, matched by channel
  for (var s of SENSORS) {
    var meta = { xlppChan: s.chan, kind: s.kind, quantity: s.quantity, unit: s.unit,
                 unitSymbol: s.unitSymbol, createdBy: "setup_wazigate_device.js" };
    var have = (device.sensors || []).find(function (x) { return x.meta && Number(x.meta.xlppChan) === s.chan; });
    if (have) {
      await api("POST", base + "/sensors/" + have.id + "/name", s.name);
      await api("POST", base + "/sensors/" + have.id + "/meta", meta);
      console.log("  sensor ch " + s.chan + " renamed: " + s.name);
    } else {
      await api("POST", base + "/sensors", { name: s.name, meta: meta });
      console.log("  sensor ch " + s.chan + " created: " + s.name);
    }
  }

  // 4. actuators, matched by name
  for (var a of ACTUATORS) {
    var exists = (device.actuators || []).find(function (x) { return x.name === a.name; });
    if (exists) { console.log("  actuator exists: " + a.name); continue; }
    await api("POST", base + "/actuators", {
      name: a.name,
      meta: { kind: "", quantity: "", unit: "", unitSymbol: a.unitSymbol || "",
              hint: a.hint, createdBy: "setup_wazigate_device.js" },
    });
    console.log("  actuator created: " + a.name + "  - " + a.hint);
  }
  console.log("Done. The new sensors fill from the node's next uplink.");
}

setupWaterNode().catch(function (e) { console.error("Setup failed: " + e.message); });
