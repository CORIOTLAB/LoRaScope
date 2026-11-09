/*
 * CORIOT GNSS uplink decoder
 *
 * Payload version history:
 * - Version 1: original 20-byte coverage payload. Byte 19 already reserved
 *   its low nibble for the verified LoRaWAN TX-power index.
 * - Version 2: current 20-byte payload. The high nibble of byte 19 explicitly
 *   identifies the payload version. Unix time is omitted because the network
 *   server timestamps each uplink. Satellite diagnostics, integer pitch, and
 *   full-range roll remain encoded in the same compact payload.
 *
 * Version 2 changes:
 * - Added an explicit payload version discriminator.
 * - Added SATX to report saturation of any 6-bit satellite counter.
 * - Kept the verified TX-power index in the low nibble of byte 19.
 * - Rejects payload lengths other than 20 bytes and unsupported versions.
 */
function decodeUplink(input) {
  // BYTS: raw unsigned byte array received from the LoRaWAN application.
  var BYTS = input.bytes;

  if (!BYTS || BYTS.length !== 20) {
    return {
      errors: [
        "Invalid payload length: expected 20 bytes, got " +
        (BYTS ? BYTS.length : 0)
      ]
    };
  }

  // Bytes 0..3: five 6-bit satellite counters and a 2-bit fix type.
  // GPSV: number of visible GPS satellites, saturated at 63.
  var GPSV = (BYTS[0] >> 2) & 0x3F;
  // BDSV: number of visible BeiDou satellites, saturated at 63.
  var BDSV = ((BYTS[0] & 0x03) << 4) | ((BYTS[1] >> 4) & 0x0F);
  // GLOV: number of visible GLONASS satellites, saturated at 63.
  var GLOV = ((BYTS[1] & 0x0F) << 2) | ((BYTS[2] >> 6) & 0x03);
  // GALV: number of visible Galileo satellites, saturated at 63.
  var GALV = BYTS[2] & 0x3F;
  // SATU: number of satellites used by the navigation solution, saturated at 63.
  var SATU = (BYTS[3] >> 2) & 0x3F;
  // FIXT: GNSS fix type encoded in two bits (1=no fix, 2=2D, 3=3D).
  var FIXT = BYTS[3] & 0x03;

  // Byte 4 bit 7 reports satellite-counter saturation.
  // SATX: 1 when at least one packed satellite counter was greater than 63.
  var SATX = (BYTS[4] >> 7) & 0x01;
  // PDOP: position dilution of precision with 0.1 resolution.
  var PDOP = (BYTS[4] & 0x7F) / 10.0;
  // HDOP: horizontal dilution of precision with 0.1 resolution.
  var HDOP = BYTS[5] / 10.0;
  // VDOP: vertical dilution of precision with 0.1 resolution.
  var VDOP = BYTS[6] / 10.0;

  // Bytes 7..10: signed latitude multiplied by 1,000,000.
  // LATI: latitude in decimal degrees.
  var LATI = (
    (BYTS[7] << 24) |
    (BYTS[8] << 16) |
    (BYTS[9] << 8) |
    BYTS[10]
  );
  LATI = LATI / 1000000.0;

  // Bytes 11..14: signed longitude multiplied by 1,000,000.
  // LONG: longitude in decimal degrees.
  var LONG = (
    (BYTS[11] << 24) |
    (BYTS[12] << 16) |
    (BYTS[13] << 8) |
    BYTS[14]
  );
  LONG = LONG / 1000000.0;

  // Bytes 15..16: signed altitude multiplied by 10.
  // ALTI: altitude above mean sea level in metres.
  var ALTI = (BYTS[15] << 8) | BYTS[16];
  if (ALTI & 0x8000) ALTI -= 0x10000;
  ALTI = ALTI / 10.0;

  // Bytes 17..18: integer pitch (1 degree) and roll (2 degrees).
  // PIT8: signed raw pitch byte in one-degree steps.
  var PIT8 = BYTS[17];
  // ROL8: signed raw roll byte in two-degree steps.
  var ROL8 = BYTS[18];
  if (PIT8 & 0x80) PIT8 -= 0x100;
  if (ROL8 & 0x80) ROL8 -= 0x100;
  // PITC: decoded pitch angle in degrees.
  var PITC = PIT8;
  // ROLL: decoded roll angle in degrees.
  var ROLL = ROL8 * 2;

  // Byte 19: payload version in the high nibble and TX-power in the low nibble.
  // VERS: binary payload-format version used to select compatible decoding.
  var VERS = (BYTS[19] >> 4) & 0x0F;
  // TXPW: RAK3172 LoRaWAN TX-power index verified immediately before uplink.
  var TXPW = BYTS[19] & 0x0F;
  if (VERS !== 2) {
    return { errors: ["Unsupported payload version: " + VERS] };
  }

  return {
    data: {
      VERS: VERS,
      GPSV: GPSV,
      BDSV: BDSV,
      GLOV: GLOV,
      GALV: GALV,
      SATU: SATU,
      SATX: SATX,
      FIXT: FIXT,
      PDOP: PDOP,
      HDOP: HDOP,
      VDOP: VDOP,
      LATI: LATI,
      LONG: LONG,
      ALTI: ALTI,
      PITC: PITC,
      ROLL: ROLL,
      TXPW: TXPW
    }
  };
}
