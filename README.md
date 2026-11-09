<div align="center">

# LoRaScope

**An open, modular hardware platform for joint LoRaWAN coverage mapping and GNSS positioning-quality assessment**

[![CI](https://github.com/OWNER/LoRaScope/actions/workflows/ci.yml/badge.svg)](https://github.com/OWNER/LoRaScope/actions/workflows/ci.yml)
[![Code: MIT](https://img.shields.io/badge/code-MIT-blue.svg)](LICENSES/MIT.txt)
[![Hardware: CERN-OHL-P-2.0](https://img.shields.io/badge/hardware-CERN--OHL--P--2.0-green.svg)](LICENSES/CERN-OHL-P-2.0.txt)
[![Data: CC BY 4.0](https://img.shields.io/badge/data-CC%20BY%204.0-lightgrey.svg)](LICENSES/CC-BY-4.0.txt)

</div>

<p align="justify">
This repository is the open-science companion of the article
</p>

> J. Botero-Valencia, J. Morales-Guerra, S. Cano-Molina, L. Hernández-Palacio, R. Hernández-García, S. Gutiérrez-Calderon and C. Del-Valle-Soto, "LoRaScope: An Open Modular Hardware Platform for Joint LoRaWAN Coverage Mapping and GNSS Positioning Quality Assessment", *Applied Sciences* (MDPI), 2026. *Manuscript in preparation; the DOI will be added on publication.*

<p align="justify">
It contains everything needed to inspect, reproduce and extend the work: the firmware and network-server payload decoder, the 3D-printable parts, the six field-campaign datasets, and the scripts that generate the geographic figures of the article.
</p>

![Exploded view of the LoRaScope assembly](docs/images/lorascope_exploded_view.png)

## What is LoRaScope?

<p align="justify">
LoRaScope is a portable instrument that sends, in <b>every LoRaWAN uplink</b>, a compact 20-byte frame with the GNSS solution, its quality indicators (fix type, satellites used, visible satellites per constellation, PDOP/HDOP/VDOP) and the device orientation (pitch and roll). The network server adds the RSSI and SNR measured by the receiving gateway, so radio quality and positioning conditions are recorded together at each location, under fixed radio settings (SF7, transmit-power index 0, ADR disabled).
</p>

| Subsystem | Component |
| --- | --- |
| Processing and display | M5Stack AtomS3 (ESP32-S3) |
| Orientation | M5Stack Unit Accel (ADXL345, SKU U056), I²C |
| LoRaWAN | M5Stack Atom DTU LoRaWAN base, RAK3172 (AU915, sub-band 2) |
| GNSS | M5Stack Unit GPS SMA (SKU U190, ATGM336H) with external SK530 antenna |
| Power | PM1S-16340 charging/step-up module, 16340 Li-ion cell |
| Structure | LEGO Technic elements and two 3D-printed adapters |

## Headline results (as reported in the manuscript)

<p align="justify">
Six mobile campaigns on a private ChirpStack network in Medellín, Colombia, with two gateways:
</p>

| | Ciudad del Río (GW-CR) | Fraternidad (GW-FR) |
| --- | ---: | ---: |
| Received uplinks | 631 | 1140 |
| Packet delivery ratio | 43.5 % | 83.8 % |
| Log-distance slope *n* | 4.25 | 1.54 |

<p align="justify">
In total, 1771 georeferenced uplinks along a 67.6 km lower-bound path. Distance-adjusted RSSI residuals showed negligible correlation with PDOP (Spearman ρ = −0.085 and −0.012). Whole-device power measured at 5 V was 0.592 W (60 s confirmed-uplink configuration). Please read the article for the limitations of these estimates; the numbers here may change in the published version.
</p>

| | |
| --- | --- |
| ![RSSI maps](docs/images/rssi_map_by_zone.png) | ![PDR map](docs/images/pdr_map.png) |

## Repository layout

```
LoRaScope/
├── hardware/           3D-printed parts (STEP and STL) and bill of materials
├── firmware/           Arduino sketch, CORIOT_* libraries and ChirpStack payload decoder
├── data/campaigns/     Six field-campaign datasets (CSV) and their data dictionary
├── analysis/           Scripts that regenerate the maps and GNSS figures
├── docs/               20-byte frame specification and figures used in this README
├── tests/              Decoder round-trip test against the datasets
├── tools/              Dataset integrity check and publication helper
└── LICENSES/           Full license texts (MIT, CERN-OHL-P-2.0, CC-BY-4.0)
```

## Quick start

<p align="justify">
<b>1. Build the hardware.</b> Print the two adapters in <a href="hardware/README.md"><code>hardware/cad/</code></a> and assemble the parts listed in <a href="hardware/BOM.csv"><code>hardware/BOM.csv</code></a>.
</p>

<p align="justify">
<b>2. Flash the firmware.</b> See <a href="firmware/README.md"><code>firmware/README.md</code></a>. In short: install the <i>M5Unified</i> and <i>M5-LoRaWAN-RAK</i> libraries, copy the <code>CORIOT_*</code> libraries from <code>firmware/libraries/</code> to your Arduino libraries folder, copy <code>A152_CONFIG.example.h</code> to <code>A152_CONFIG.h</code>, enter your own ChirpStack keys, and upload <code>CORIOT_LoRaScope.ino</code> to an AtomS3.
</p>

<p align="justify">
<b>3. Decode uplinks.</b> Paste <a href="firmware/decoder/decoder.js"><code>firmware/decoder/decoder.js</code></a> as a JavaScript codec in your ChirpStack device profile. The frame is described in <a href="docs/payload-format.md"><code>docs/payload-format.md</code></a>.
</p>

<p align="justify">
<b>4. Explore the data.</b> See <a href="data/README.md"><code>data/README.md</code></a>.
</p>

<p align="justify">
<b>5. Regenerate the figures.</b>
</p>

```bash
pip install -r analysis/requirements.txt
python analysis/regenerate_maps.py
```

<p align="justify">
Outputs are written to <code>analysis/output/</code>. Esri basemap tiles are downloaded on first use; without internet access the maps are drawn without the basemap. See <a href="analysis/README.md"><code>analysis/README.md</code></a>.
</p>

## Verify the repository

```bash
python tools/check_data.py        # dataset integrity and numbers quoted in the article
node tests/decoder.test.js        # encodes all 1771 records and decodes them back
```

<p align="justify">
Both checks run automatically on every push through GitHub Actions.
</p>

## Security note

<p align="justify">
Never commit LoRaWAN credentials. <code>A152_CONFIG.h</code> is listed in <code>.gitignore</code>; only the placeholder <code>A152_CONFIG.example.h</code> is versioned.
</p>

## How to cite

<p align="justify">
Please cite the article and, where relevant, the dataset and firmware release. Machine-readable metadata is provided in <a href="CITATION.cff"><code>CITATION.cff</code></a>; GitHub shows a <i>Cite this repository</i> button.
</p>

## Licenses

| Content | License |
| --- | --- |
| Firmware, decoder, scripts and tests | [MIT](LICENSES/MIT.txt) |
| Hardware design files (STEP, STL, bill of materials) | [CERN-OHL-P-2.0](LICENSES/CERN-OHL-P-2.0.txt) |
| Datasets and documentation | [CC BY 4.0](LICENSES/CC-BY-4.0.txt) |

<p align="justify">
Third-party components keep their own licenses: LEGO® Technic elements, M5Stack and RAKwireless products, the <i>M5Unified</i> and <i>M5-LoRaWAN-RAK</i> libraries, and Esri basemap tiles (© Esri) shown in the figures. LEGO® is a trademark of the LEGO Group, which does not sponsor or endorse this project.
</p>

## Funding and acknowledgments

<p align="justify">
This research received funding from the Sistema General de Regalías (SGR) – Asignación para la Ciencia, Tecnología e Innovación (ACTeI), through the SosAgro4.C project (BPIN code 2024000100094). The authors acknowledge the Grupo de Sistemas de Control y Robótica (COL0123701) of the Institución Universitaria ITM.
</p>

## Contact

<p align="justify">
Corresponding author: Juan Botero-Valencia, Grupo Sistemas de Control y Robótica, Institución Universitaria ITM, Medellín, Colombia. Please use <a href="../../issues">GitHub Issues</a> for questions about the repository.
</p>
