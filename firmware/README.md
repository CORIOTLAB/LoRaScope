# Firmware

<p align="justify">
Arduino firmware for the M5Stack AtomS3 that reads a GNSS receiver and an ADXL345
accelerometer, packs the data into the 20-byte LoRaScope frame and transmits it
through the RAK3172 (A152) LoRaWAN module.
</p>

```
firmware/
├── CORIOT_LoRaScope/        Arduino sketch (.ino) and configuration
│   ├── CORIOT_LoRaScope.ino
│   ├── CORIOT_LoRaScope_AUX.h
│   └── A152_CONFIG.example.h   → copy to A152_CONFIG.h (not versioned)
├── libraries/               CORIOT_* libraries used by the sketch
└── decoder/decoder.js       ChirpStack payload codec (frame version 2)
```

## Requirements

- Arduino IDE 2.x or `arduino-cli`, with the ESP32 core for M5Stack boards and the **M5Stack AtomS3** board selected.
- Libraries from the Library Manager: **M5Unified** and **M5-LoRaWAN-RAK**.
- The `CORIOT_*` libraries in [`libraries/`](libraries): copy each folder to your Arduino sketchbook `libraries/` directory (or point your build to this folder).
- A LoRaWAN network server with an OTAA device per LoRaScope. The article used ChirpStack with the AU915 plan restricted to sub-band 2.

## Build and flash

1. Copy `CORIOT_LoRaScope/A152_CONFIG.example.h` to `CORIOT_LoRaScope/A152_CONFIG.h`.
2. Enter the DevEUI, AppEUI and AppKey of your device(s) in `A152_CONFIG.h`. **Never commit this file.**
3. Select the device with `A152_KEYS` (`A152_01` … `A152_04`; default `A152_02`).
4. Open `CORIOT_LoRaScope.ino`, select the AtomS3 board and upload.
5. Open the serial monitor at 115200 baud. The start-up trace reports `ATGM.IN`, `NMEA.TK`, `ADXL.IN` and the radio join sequence (`A152.JR`, then `A152.JN | OK` when the network accepts the device).

## Configuration

<p align="justify">
Defined in <code>A152_CONFIG.h</code> unless stated otherwise.
</p>

| Setting | Default | Meaning |
| --- | --- | --- |
| `LW_UPLINK_SEC` | `5` | Uplink period in seconds (the sketch falls back to 60 if undefined) |
| `LW_CONFIRMED` | `true` | Confirmed uplinks. **The mobile campaigns of the article used unconfirmed uplinks: set `false` to reproduce them.** |
| `LW_ADR` | `false` | Adaptive data rate disabled |
| `LW_DATA_RATE` | `5` | DR5 = SF7/125 kHz in AU915 |
| `LW_CHANNEL_MASK` | `"0002"` | AU915 sub-band 2 |
| `LW_UPLINK_PORT` | `1` | LoRaWAN application port |
| `LW_TX_POWER` (sketch) | `0` | Regional transmit-power index 0 (MaxEIRP 30 dBm in AU915; not a measured EIRP) |
| `GNSS_SOURCE` | `GNSS_SRC_ATGM` | `GNSS_SRC_SIM` (simulated NMEA), `GNSS_SRC_M10` (u-blox M10) or `GNSS_SRC_ATGM` (ATGM336H) |

<p align="justify">
Pin assignment for the AtomS3: GNSS UART RX 1 / TX 2 at 115200 baud; RAK3172 UART RX 6 / TX 5; ADXL345 I²C SDA 38 / SCL 39 at 400 kHz.
</p>

<p align="justify">
The sketch transmits only when a valid 2D or 3D GNSS epoch is available. The button on
the AtomS3 forces an immediate uplink.
</p>

## Payload decoder

<p align="justify">
<a href="decoder/decoder.js"><code>decoder/decoder.js</code></a> implements <code>decodeUplink()</code> for ChirpStack
(and any server that uses the same codec interface). It accepts exactly 20 bytes with
frame version 2 and returns <code>GPSV, BDSV, GLOV, GALV, SATU, SATX, FIXT, PDOP, HDOP,
VDOP, LATI, LONG, ALTI, PITC, ROLL, VERS, TXPW</code>. The bit layout is documented in
<a href="../docs/payload-format.md"><code>docs/payload-format.md</code></a> and tested against every record of the
campaign datasets (<code>node tests/decoder.test.js</code>).
</p>

## Notes

- Galileo satellites were never reported in the campaigns (`GALV = 0`), although the receiver is specified for Galileo; the GNSS analyses use GPS, GLONASS and BeiDou.
- The libraries carry version `1.0.0` and author `CORIOT`; they have no separate releases.

## License

<p align="justify">
MIT, see <a href="../LICENSES/MIT.txt"><code>LICENSES/MIT.txt</code></a>. Third-party libraries (M5Unified, M5-LoRaWAN-RAK) keep their own licenses.
</p>
