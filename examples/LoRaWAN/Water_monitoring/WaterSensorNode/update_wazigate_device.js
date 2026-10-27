/*
 * ============================================================================
 *  KijaniSpace water node - update an EXISTING WaziGate (v3) device
 * ============================================================================
 *  Paste into the browser console of the WaziGate dashboard while logged in
 *  (F12 -> Console, paste the whole file, Enter). Safe to run any number of
 *  times: it renames and completes, and never creates a duplicate.
 *  To make the device from nothing instead, use create_wazigate_device.js.
 *
 *  What it does, on the device with the LoRaWAN address DEV_ADDR below (if
 *  several share it, the most recently changed one):
 *    1. switches it to WaziGate's built-in XLPP codec;
 *    2. re-registers it with the LoRa service, so uplinks and downlinks are
 *       routed to this device - the service routes an address to the device
 *       whose settings were written last;
 *    3. creates or renames the sensors the XLPP codec fills, one per channel,
 *       with names and units - the codec matches them by meta.xlppChan;
 *    4. creates the actuators pH calibration, EC calibration, Interval, or
 *       gives existing ones their XLPP channel (20-22) and type. An existing
 *       actuator that had no channel yet is reset to 0 first, because its old
 *       value meant something else.
 *
 *  WHY XLPP. WaziGate can only build downlinks through the device's codec,
 *  and a JavaScript codec cannot encode (in v3 its encoder is a stub), so
 *  with a custom decoder no actuator value ever reaches the node.
 *
 *  THE ACTUATORS. A push sends every actuator with its channel and current
 *  value; the node acts only on a value that changed since the last downlink:
 *    pH calibration   7 (always first), then 4, then 10; -1 = clear
 *    EC calibration   1 = dry (probe in air), then 84 or 1413 uS/cm; 2 = clear
 *    Interval         seconds 60..7200; 0 = the firmware default
 *  To repeat a step, push 0 first. Results arrive on "pH / EC calibration
 *  result": 0..3 = points stored, 90 = implausible, 91 = no answer,
 *  92 = never settled, 93 = battery low. Full procedure: docs/calibration.md.
 *
 *  The SENSORS and ACTUATORS lists are identical in create_wazigate_device.js.
 *  Change one, change the other.
 * ========================================================================= */

var DEV_ADDR = "00000000";   // the node's DevAddr, 8 hex digits, as in WaterSensorNode.ino (devAddr)
var DEVICE_ID = "6ada0ba468f31908de7a03c3";          // optional: a WaziGate device id, overrides the search by DEV_ADDR

var SENSORS = [
  { chan: 1,  name: "Water temperature",     kind: "Thermometer",             quantity: "Temperature",  unit: "DegreeCelsius",     unitSymbol: "°C" },
  { chan: 2,  name: "Battery",               kind: "VoltageSensor",           quantity: "Voltage",      unit: "Volt",              unitSymbol: "V" },
  { chan: 3,  name: "pH",                    kind: "WaterPHSensor",           quantity: "PH",           unit: "",                  unitSymbol: "pH" },
  { chan: 4,  name: "Dissolved oxygen",      kind: "OxygenSensor",            quantity: "",             unit: "MilligramPerLitre", unitSymbol: "mg/L" },
  { chan: 5,  name: "Conductivity",          kind: "WaterConductivitySensor", quantity: "Conductivity", unit: "MicroSiemens",      unitSymbol: "µS/cm" },
  { chan: 6,  name: "Turbidity",             kind: "",                        quantity: "",             unit: "",                  unitSymbol: "NTU" },
  { chan: 7,  name: "DO probe temperature",  kind: "Thermometer",             quantity: "Temperature",  unit: "DegreeCelsius",     unitSymbol: "°C" },
  { chan: 8,  name: "Restart cause",         kind: "",                        quantity: "",             unit: "",                  unitSymbol: "" },
  { chan: 9,  name: "pH calibration result", kind: "",                        quantity: "",             unit: "",                  unitSymbol: "" },
  { chan: 10, name: "EC calibration result", kind: "",                        quantity: "",             unit: "",                  unitSymbol: "" },
];

// chan: the XLPP channel the gateway packs the value under (meta.xlppChan);
// without it the actuator is left out of every downlink. quantity + unit pick
// the XLPP type: "AnalogeValue" (WaziGate's spelling) is analog, 0.01 steps
// up to 327.67; concentration in ppm is a whole number up to 65535 (the
// interval uses it for that range, not for its meaning).
var ACTUATORS = [
  { chan: 20, name: "pH calibration", quantity: "AnalogeValue",               unit: "",                unitSymbol: "pH",
    hint: "7 first, then 4, then 10 (probe in that buffer); -1 = clear; 0 = nothing" },
  { chan: 21, name: "EC calibration", quantity: "ChemicalAgentConcentration", unit: "PartsPerMillion", unitSymbol: "µS/cm",
    hint: "1 = dry (probe in air), then 84 or 1413 (probe in that standard); 2 = clear; 0 = nothing" },
  { chan: 22, name: "Interval",       quantity: "ChemicalAgentConcentration", unit: "PartsPerMillion", unitSymbol: "s",
    hint: "seconds, 60..7200 (300 or more outside the bench, duty cycle); 0 = firmware default" },
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

function devAddrOf(d) {
  return ((d.meta && d.meta.lorawan && d.meta.lorawan.devAddr) || "").toUpperCase();
}

async function configure(device) {
  var base = "devices/" + device.id;
  for (var s of SENSORS) {
    var meta = { xlppChan: s.chan, kind: s.kind, quantity: s.quantity, unit: s.unit,
                 unitSymbol: s.unitSymbol, createdBy: "kijanispace-device-script" };
    var have = (device.sensors || []).find(function (x) { return x.meta && Number(x.meta.xlppChan) === s.chan; });
    if (have) {
      await api("POST", base + "/sensors/" + have.id + "/name", s.name);
      await api("POST", base + "/sensors/" + have.id + "/meta", meta);
      console.log("  sensor ch " + s.chan + " named: " + s.name);
    } else {
      await api("POST", base + "/sensors", { name: s.name, meta: meta });
      console.log("  sensor ch " + s.chan + " created: " + s.name);
    }
  }
  for (var a of ACTUATORS) {
    var meta = { xlppChan: a.chan, kind: "", quantity: a.quantity, unit: a.unit,
                 unitSymbol: a.unitSymbol, hint: a.hint, createdBy: "kijanispace-device-script" };
    var have = (device.actuators || []).find(function (x) { return x.name === a.name; });
    if (!have) {
      await api("POST", base + "/actuators", { name: a.name, value: 0, meta: meta });
      console.log("  actuator ch " + a.chan + " created: " + a.name + "  - " + a.hint);
      continue;
    }
    // An actuator from before the channels holds a value in the old meaning
    // (seconds, -3 ...). Zero it while it has no channel yet - that push is
    // packed into no downlink - and only then give it its channel.
    if (!(have.meta && have.meta.xlppChan !== undefined && have.meta.xlppChan !== null)) {
      try {
        await api("POST", base + "/actuators/" + have.id + "/value", 0);
        console.log("  actuator " + a.name + ": old value " + JSON.stringify(have.value) + " reset to 0");
      } catch (e) {
        console.warn("  actuator " + a.name + ": could not reset the value (" + e.message + "); push 0 by hand");
      }
    }
    await api("POST", base + "/actuators/" + have.id + "/meta", meta);
    console.log("  actuator ch " + a.chan + " set up: " + a.name + "  - " + a.hint);
  }
}

async function updateWaterNode() {
  var devices = await api("GET", "devices");
  var device;
  if (DEVICE_ID) {
    device = devices.find(function (d) { return d.id === DEVICE_ID; });
    if (!device) { console.log("No device with id " + DEVICE_ID); return; }
  } else {
    if (!/^[0-9A-Fa-f]{8}$/.test(DEV_ADDR) || DEV_ADDR === "00000000") {
      console.log("Set DEV_ADDR at the top of the script (8 hex digits, the node's devAddr).");
      return;
    }
    var hits = devices.filter(function (d) { return devAddrOf(d) === DEV_ADDR.toUpperCase(); });
    if (hits.length === 0) {
      console.log("No device has the address " + DEV_ADDR + ". Create it with create_wazigate_device.js.");
      return;
    }
    hits.sort(function (a, b) { return new Date(b.modified) - new Date(a.modified); });
    device = hits[0];
    if (hits.length > 1) {
      console.log(hits.length + " devices share the address " + DEV_ADDR + "; updating the most recently changed.");
      console.log("The others receive no data and can be deleted in the dashboard:");
      hits.slice(1).forEach(function (d) { console.log("  " + d.id + "  " + d.name); });
    }
  }
  console.log("Device: " + device.name + " (" + device.id + ")");

  // The codec, and the LoRaWAN block written again: the LoRa service only
  // (re)learns which device a DevEUI belongs to when the device meta changes.
  var meta = { codec: "application/x-xlpp" };
  if (device.meta && device.meta.lorawan) meta.lorawan = device.meta.lorawan;
  await api("POST", "devices/" + device.id + "/meta", meta);
  console.log("  codec -> application/x-xlpp, LoRaWAN registration refreshed");

  await configure(device);
  console.log("Done. Values fill from the node's next uplink.");
}

updateWaterNode().catch(function (e) { console.error("Update failed: " + e.message); });
