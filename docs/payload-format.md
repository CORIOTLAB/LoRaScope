# LoRaScope 20-byte uplink frame (version 2)

Multi-byte fields are big-endian and signed fields use two's complement. The frame is
sent on LoRaWAN port 1. [`firmware/decoder/decoder.js`](../firmware/decoder/decoder.js) is the reference decoder and
`tests/decoder.test.js` checks it against all records of the datasets.

| Byte | Bits | Field | Resolution / range | Description |
| :-: | :-: | --- | --- | --- |
| 0 | 7–2 | `GPSV` | 1, 0–63 | Visible GPS satellites |
| 0–1 | 1–0 of byte 0, 7–4 of byte 1 | `BDSV` | 1, 0–63 | Visible BeiDou satellites |
| 1–2 | 3–0 of byte 1, 7–6 of byte 2 | `GLOV` | 1, 0–63 | Visible GLONASS satellites |
| 2 | 5–0 | `GALV` | 1, 0–63 | Visible Galileo satellites |
| 3 | 7–2 | `SATU` | 1, 0–63 | Satellites used in the solution |
| 3 | 1–0 | `FIXT` | 1 = no fix, 2 = 2D, 3 = 3D | Fix type (1 is reserved and never transmitted) |
| 4 | 7 | `SATX` | 0/1 | Set if any satellite counter saturated |
| 4 | 6–0 | `PDOP` | 0.1 | Position dilution of precision |
| 5 | 7–0 | `HDOP` | 0.1 | Horizontal dilution of precision |
| 6 | 7–0 | `VDOP` | 0.1 | Vertical dilution of precision |
| 7–10 | 31–0 | `LATI` | 10⁻⁶ ° (signed) | Latitude |
| 11–14 | 31–0 | `LONG` | 10⁻⁶ ° (signed) | Longitude |
| 15–16 | 15–0 | `ALTI` | 0.1 m (signed) | Altitude above mean sea level |
| 17 | 7–0 | `PITC` | 1 ° (signed) | Pitch |
| 18 | 7–0 | `ROLL` | 2 ° (signed) | Roll (raw byte × 2) |
| 19 | 7–4 | `VERS` | 2 | Frame version |
| 19 | 3–0 | `TXPW` | 0–15 | LoRaWAN transmit-power index |

The four constellation counters (`GPSV`, `BDSV`, `GLOV`, `GALV`) occupy the first 24 bits
(bytes 0–2); `SATU` (6 bits) and `FIXT` (2 bits) fill byte 3. The frame totals 20 bytes.

## Version history

- **Version 1:** original 20-byte coverage payload; the low nibble of byte 19 already reserved the transmit-power index.
- **Version 2 (current):** the high nibble of byte 19 identifies the payload version; Unix time is omitted because the network server timestamps each uplink; adds `SATX`.

The decoder rejects payloads whose length is not 20 bytes and unsupported versions.
