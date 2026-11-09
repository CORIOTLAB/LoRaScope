# Campaign datasets

Six mobile LoRaWAN/GNSS campaigns recorded in Medellín, Colombia, three in the
coverage area of each gateway. Each row is **one uplink received by a gateway**:
the decoded 20-byte LoRaScope frame joined with the reception metadata of the
gateway. These are the exact files used for every number and map of the article.

| File | Zone | Gateway | Records | Attempts inferred | PDR (%) | Path (km) |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| `dataset_ciudad_del_rio_1.csv` | Ciudad del Río | GW-CR | 242 | 490 | 49.4 | 10.6 |
| `dataset_ciudad_del_rio_2.csv` | Ciudad del Río | GW-CR | 207 | 485 | 42.7 | 10.3 |
| `dataset_ciudad_del_rio_3.csv` | Ciudad del Río | GW-CR | 182 | 477 | 38.2 | 9.8 |
| `dataset_fraternidad_1.csv` | Fraternidad | GW-FR | 326 | 431 | 75.6 | 10.8 |
| `dataset_fraternidad_2.csv` | Fraternidad | GW-FR | 350 | 428 | 81.8 | 12.3 |
| `dataset_fraternidad_3.csv` | Fraternidad | GW-FR | 464 | 502 | 92.4 | 13.8 |
| **Total** | | | **1771** | **2813** | **63.0** | **67.6** |

Gateways: **GW-CR** `2cf7f11162900005` at 6.22427° N, 75.57398° W (about 30 m above
ground, south facade, 3 dBi antenna); **GW-FR** `787264fffee04f72` at 6.24482° N,
75.55130° W (about 20 m, rooftop, 5 dBi antenna). Network: AU915, sub-band 2, SF7
(DR5), transmit-power index 0, ADR disabled, 5 s reporting interval, unconfirmed uplinks.

## Data dictionary

| Column | Unit / values | Description |
| --- | --- | --- |
| `GPSV`, `BDSV`, `GLOV`, `GALV` | count | Visible GPS, BeiDou, GLONASS and Galileo satellites (`GALV` is always 0, see below) |
| `SATU` | count | Satellites used in the position solution |
| `SATX` | 0/1 | 1 if any satellite counter exceeded its 6-bit range (always 0 here) |
| `FIXT` | 2 or 3 | Fix type: 2 = 2D, 3 = 3D (no uplink is sent without a fix) |
| `PDOP`, `HDOP`, `VDOP` | – | Dilution of precision, 0.1 resolution |
| `LATI`, `LONG` | degrees | WGS-84 latitude and longitude reported by the receiver, 10⁻⁶° resolution |
| `ALTI` | m | GNSS altitude above mean sea level, 0.1 m resolution |
| `PITC`, `ROLL` | degrees | Pitch (1° steps) and roll (2° steps) from the ADXL345 |
| `TXPW` | index | LoRaWAN transmit-power index (0 in all records) |
| `VERS` | – | Frame version (2 in all records) |
| `gateway_id` | EUI-64 | Receiving gateway |
| `gateway_channel_index` | 0–7 | Uplink channel within sub-band 2 |
| `crc_status` | 1 | CRC status reported by the gateway (1 = correct; all records) |
| `gateway_latitude`, `gateway_longitude` | degrees | Gateway location |
| `rf_chain` | 0/1 | Gateway radio-frequency chain |
| `rssi` | dBm | Received signal strength reported by the gateway |
| `snr` | dB | Signal-to-noise ratio reported by the gateway |
| `fcnt_rel` | – | LoRaWAN uplink frame counter relative to the first received uplink of the campaign: FCntUp − FCntUp<sub>first</sub> + 1 |
| `x_utm_m`, `y_utm_m` | m | Easting and northing of (`LATI`, `LONG`) in UTM zone 18N (EPSG:32618) |
| `gateway_distance_m` | m | Planar distance between the record and the gateway in UTM coordinates |

`x_utm_m`, `y_utm_m` and `gateway_distance_m` were verified against an independent
WGS-84 → EPSG:32618 transformation (maximum difference 0.5 mm, i.e. rounding).

## How packet delivery ratio is obtained

Rows are in increasing `fcnt_rel` order. Missing counter values between two received
rows are lost uplinks, so PDR = received / (received + lost) computed from bounded gaps.
Attempts before the first or after the last received uplink cannot be inferred, which
makes PDR an upper bound. Of the 1042 missing frames, 764 (73.3 %) lie in gaps longer
than 12 frames; the largest gap is 123 frames (about 10.25 min at 5 s) in campaign
`ciudad_del_rio_3`. The article draws interpolated positions only for gaps of up to 12 frames.

## Limitations to keep in mind

- Only **received** uplinks are present; RSSI and SNR distributions are censored where packets were not decoded.
- The files carry no timestamps; campaign dates, duration and mode of travel are described in the article.
- No Galileo satellites were reported, although the receiver is specified for Galileo.
- The antenna-comparison and energy-consumption measurements of the article are **not** included in this release.
- SNR values are compressed near +15 dB.

## Check the files

```bash
python tools/check_data.py
```

## License

[CC BY 4.0](../LICENSES/CC-BY-4.0.txt). Please cite the article when you reuse the data.
