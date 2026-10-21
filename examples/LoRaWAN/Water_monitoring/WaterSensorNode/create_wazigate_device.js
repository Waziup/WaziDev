/*
 * ============================================================================
 *  KijaniSpace water node - create the WaziGate (v3) device from scratch
 * ============================================================================
 *  Paste into the browser console of the WaziGate dashboard while logged in
 *  (F12 -> Console, paste the whole file, Enter), after filling in CONFIG.
 *  For a device that already exists, use update_wazigate_device.js.
 *
 *  What it does:
 *    1. creates one LoRaWAN device (ABP, profile "WaziDev") with the XLPP
 *       codec, the address and the session key. The DevEUI is derived from
 *       the address ("AA555A00" + DevAddr), as the dashboard does;
 *    2. registers it with the LoRa service. The service routes an address to
 *       the device whose settings were written last, so from now on this
 *       device gets the data; older devices with the same address get none
 *       and are listed, so they can be deleted in the dashboard;
 *    3. creates the named sensors, one per XLPP channel, and the actuators
 *       pH calibration, EC calibration, Interval.
 *
 *  The key: WaziDev uses ONE 16-byte key as both network and application
 *  session key - the LoRaWANKeys array in WaterSensorNode.ino, written as 32
 *  hex digits. The node keeps its own frame counter; the gateway's WaziDev
 *  profile accepts it, so the node needs no reflash for a new device.
 *
 *  The SENSORS and ACTUATORS lists are identical in update_wazigate_device.js.
 *  Change one, change the other.
 * ========================================================================= */

// ---- CONFIG ----------------------------------------------------------------
var DEVICE_NAME = "KijaniSpace water node";
var DEV_ADDR = "00000000";                              // 8 hex digits: devAddr in WaterSensorNode.ino
var SESSION_KEY = "00000000000000000000000000000000";  // 32 hex digits: LoRaWANKeys in WaterSensorNode.ino
// ----------------------------------------------------------------------------

var SENSORS = [
  { chan: 1,  name: "Water temperature",     kind: "Thermometer",             quantity: "Temperature",  unit: "DegreeCelsius",     unitSymbol: "°C" },
  { chan: 2,  name: "Battery",               kind: "VoltageSensor",           quantity: "Voltage",      unit: "Volt",              unitSymbol: "V" },
  { chan: 3,  name: "pH",                    kind: "WaterPHSensor",           quantity: "PH",           unit: "",                  unitSymbol: "pH" },
  { chan: 4,  name: "Dissolved oxygen",      kind: "OxygenSensor",            quantity: "",             unit: "MilligramPerLitre", unitSymbol: "mg/L" },
  { chan: 5,  name: "Conductivity",          kind: "WaterConductivitySensor", quantity: "Conductivity", unit: "MicroSiemens",      unitSymbol: "µS/cm" },
  { chan: 6,  name: "Turbidity",             kind: "",                        quantity: "",             unit: "",                  unitSymbol: "NTU" },
  { chan: 7,  name: "DO probe temperature",  kind: "Thermometer",             quantity: "Temperature",  unit: "DegreeCelsius",     unitSymbol: "°C" },
  { chan: 9,  name: "pH calibration result", kind: "",                        quantity: "",             unit: "",                  unitSymbol: "" },
  { chan: 10, name: "EC calibration result", kind: "",                        quantity: "",             unit: "",                  unitSymbol: "" },
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
    var exists = (device.actuators || []).find(function (x) { return x.name === a.name; });
    if (exists) { console.log("  actuator exists: " + a.name); continue; }
    await api("POST", base + "/actuators", {
      name: a.name,
      meta: { kind: "", quantity: "", unit: "", unitSymbol: a.unitSymbol || "",
              hint: a.hint, createdBy: "kijanispace-device-script" },
    });
    console.log("  actuator created: " + a.name + "  - " + a.hint);
  }
}

async function createWaterNode() {
  var addr = DEV_ADDR.toUpperCase(), key = SESSION_KEY.toUpperCase();
  if (!/^[0-9A-F]{8}$/.test(addr) || addr === "00000000") {
    console.log("Set DEV_ADDR: 8 hex digits, the devAddr array in WaterSensorNode.ino."); return;
  }
  if (!/^[0-9A-F]{32}$/.test(key) || /^0+$/.test(key)) {
    console.log("Set SESSION_KEY: 32 hex digits, the LoRaWANKeys array in WaterSensorNode.ino."); return;
  }

  var devices = await api("GET", "devices");
  var older = devices.filter(function (d) { return devAddrOf(d) === addr; });

  // 1. the device itself
  var lorawan = { profile: "WaziDev", devAddr: addr, devEUI: "AA555A00" + addr,
                  nwkSEncKey: key, appSKey: key };
  var id = await api("POST", "devices", {
    name: DEVICE_NAME, sensors: [], actuators: [],
    meta: { codec: "application/x-xlpp", lorawan: lorawan },
  });
  console.log("Created device " + DEVICE_NAME + " (" + id + "), DevEUI " + lorawan.devEUI);

  // 2. the LoRa service learns about a device only when its meta is written,
  //    not when the device is created - so write it once more
  await api("POST", "devices/" + id + "/meta", { codec: "application/x-xlpp", lorawan: lorawan });
  console.log("  registered with the LoRa service");

  // 3. sensors and actuators
  await configure({ id: id, sensors: [], actuators: [] });
  console.log("Done. Values fill from the node's next uplink.");
  if (older.length) {
    console.log("Older devices with the same address now receive no data and can be deleted in the dashboard:");
    older.forEach(function (d) { console.log("  " + d.id + "  " + d.name); });
  }
}

createWaterNode().catch(function (e) { console.error("Create failed: " + e.message); });
