/*
How to install decoder:
=======================
1. Copy contents of this script
2. Open console in browser (F12)
3. Paste content and hit ENTER
4. Select this codec, named "WaziUp-Water_Surveillance_Device" for the respective device.
*/

var new_codec_script = `
/**
 * Entry, decoder.js
 */
function Decoder(bytes, port) {
  var out = {};
  var i = 0;

  while (i < bytes.length) {
    var chan = bytes[i++];
    var type = bytes[i++];

    switch (type) {

      case 0x67: {                       // LPP_TEMPERATURE, 2 bytes, 0.1 degC
        var t = (bytes[i] << 8) | bytes[i + 1];
        if (t & 0x8000) t -= 0x10000;    // signed
        i += 2;
        var degC = t / 10.0;
        if (chan === 1)      out.temperature_C = degC;
        else if (chan === 7) out.temperature_do_probe_C = degC;
        else                 out['temperature_ch' + chan + '_C'] = degC;
        break;
      }

      case 0x02: {                       // LPP_ANALOG_INPUT, 2 bytes, 0.01
        var a = (bytes[i] << 8) | bytes[i + 1];
        if (a & 0x8000) a -= 0x10000;    // signed
        i += 2;
        var v = a / 100.0;

        if (chan === 3) {
          out.pH = v;
        } else if (chan === 4) {
          out.oxygen_mgL = v;
        } else if (chan === 5) {
          // Round after scaling. Binary floats make 0.56 * 10 come out as
          // 5.6000000000000005, and a dashboard showing that looks broken.
          // The divisor is 100, not 1000: that puts the step at 1 uS/cm
          // instead of 10, which matters because Lake Victoria sits near
          // 100 uS/cm and the EZO-EC is accurate to about 2 uS/cm there.
          out.conductivity_uScm = Math.round(v * 100);
        } else if (chan === 6) {
          out.turbidity_NTU = Math.round(v * 10 * 10) / 10;
        } else {
          out['analog_ch' + chan] = v;
        }
        break;
      }

      case 0x74: {                       // LPP_VOLTAGE, 2 bytes, 0.01 V
        var mv = (bytes[i] << 8) | bytes[i + 1];
        i += 2;
        out.battery_V = mv / 100.0;
        break;
      }

      default:
        // An unknown type means the channel map and this decoder have drifted
        // apart. Stop rather than mis-read the rest of the frame as data.
        out.decode_error = 'unknown LPP type 0x' + type.toString(16) +
                           ' on channel ' + chan + ' at byte ' + (i - 1);
        return out;
    }
  }

  return out;
}

`;

var jsonBody = {
  name: "WaziUp-Water_Surveillance_Device",
  mime: "application/javascript",
  script: new_codec_script
};

async function POST_custom_WaziGate_CODEC() {

  try {
    var POSTRequestResponse = await fetch("codecs", {
      method: "POST",
      body: JSON.stringify(jsonBody),
      headers: {
        "Content-type" : "application/json"
      }

    });
    var POSTRequestResponseContent = await POSTRequestResponse.text();
    console.log("New codec id:", POSTRequestResponseContent);
  }
  catch (err) {
		console.error(`Error at POST_custom_WaziGate_CODEC : ${err}`);
		throw err;
	}
}

POST_custom_WaziGate_CODEC()