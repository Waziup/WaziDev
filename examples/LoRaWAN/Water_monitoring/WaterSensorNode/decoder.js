/* ============================================================================
 *  KijaniSpace water node - uplink payload decoder
 * ============================================================================
 *  Decodes the Cayenne-LPP / XLPP payload the node sends and undoes the two
 *  scalings the wire format forced on us.
 *
 *  WHY ANYTHING NEEDS UNDOING
 *  LPP_ANALOG_INPUT (type 0x02) is a SIGNED 2-byte value with 0.01
 *  resolution, so its whole range is -327.68 to +327.67. Two measurements do
 *  not fit:
 *
 *    conductivity  0 - 32767 uS/cm    -> sent as uS/cm/100, multiply by 100
 *    turbidity     0 - 1000 NTU       -> sent as NTU/10, so multiply by 10
 *
 *  Without this decoder the portal shows the scaled numbers: a real 5.69 NTU
 *  appears as 0.56. That 0.56 is not an error, it is 5.69/10 truncated to the
 *  format's 0.01 step - which also costs a little resolution, though 0.1 NTU
 *  is well below the Y511-A's own +/-5 % or 0.3 NTU accuracy.
 *
 *  CHANNEL MAP (must stay in step with WaterSensorNode.ino)
 *    1  temperature   degC   water temperature: DS18B20, or the PT1000 if fitted
 *    2  battery       V      needs VccCorrection calibrated to be meaningful
 *    3  pH                   omitted entirely when the reading is rejected
 *    4  oxygen        mg/L
 *    5  conductivity  uS/cm / 100  -> uS/cm
 *    6  turbidity     NTU/10 -> NTU
 *    7  temperature   degC   the DO probe's own sensor, a cross-check on ch 1
 *    8  restart cause        raw MCUSR of the reset BEFORE this uplink
 *                            NOT CURRENTLY SENT - the sketch was rolled
 *                            back to its last confirmed build. The case
 *                            below is harmless until it returns.
 *
 *  A channel whose sensor failed, or that was skipped that cycle, is left out
 *  of the payload rather than sent as zero. So a missing field means "no
 *  reading", never "reading of 0" - do not fill gaps with zeros downstream.
 *  Turbidity is only read every 4th cycle by design.
 *
 *  Channel 8 is the exception: it is sent on EVERY uplink, because "nothing
 *  restarted" is itself the information you want on a node you cannot reach.
 *
 *  NOTE ON WIRING THIS IN: the function name and signature below follow the
 *  common `Decoder(bytes, port)` convention. Check what entry point your
 *  WaziCloud / Aqua instance expects and rename accordingly - the body is
 *  what matters.
 * ========================================================================= */

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

      case 0x00: {                       // LPP_DIGITAL_INPUT, 1 byte
        var b = bytes[i];
        i += 1;
        if (chan === 8) {
          // Why the node last restarted. Sent over the air because a node at
          // the pond has no serial port, and a restart is otherwise invisible
          // downstream: the data just arrives less often, with no reason.
          //
          // These describe the reset that PRECEDED this uplink, so a normal
          // battery change shows power_on once and then nothing.
          var why = [];
          if (b & 0x01) why.push('power_on');
          if (b & 0x02) why.push('external');   // RESET pin, or a USB monitor
          if (b & 0x04) why.push('brown_out');  // supply sagged under load
          if (b & 0x08) why.push('watchdog');   // a cycle hung, dog restarted
          out.restart_flags = b;
          out.restart_cause = why.length ? why.join('+') : 'none_recorded';

          // Worth alerting on. Either one repeating is a real fault that the
          // measurements themselves will not reveal: brown_out means the
          // supply cannot hold the rail switch-on, watchdog means a cycle is
          // hanging - most likely a blocking I2C read, since this AVR core's
          // Wire has no timeout.
          out.restart_alarm = !!(b & 0x0C);
        } else {
          out['digital_ch' + chan] = b;
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

/* ----------------------------------------------------------------------------
 *  Sanity ranges, for alarms or dashboard bounds rather than for the decoder
 *  to enforce - the node already rejects impossible readings and omits them.
 *
 *    temperature   Lake Victoria surface sits around 24-28 degC
 *    pH            6.5 - 9.0 typical; the node drops anything outside 0.5-14
 *    oxygen        0 - 20 mg/L; below ~4 is stressful for fish
 *    conductivity  Lake Victoria is soft water, order 100 uS/cm
 *    turbidity     rises sharply near a fish farm and after rain
 *
 *  Two temperatures are transmitted on purpose. Channel 1 is the DS18B20 and
 *  channel 7 the DO probe's own sensor, in a different spot. A growing gap
 *  between them is a fault worth alerting on - one of the two has drifted or
 *  come loose - and it is the kind of thing that is invisible if you only ever
 *  plot one of them.
 * ------------------------------------------------------------------------- */
