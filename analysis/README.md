# Figure generation

Scripts that regenerate the geographic figures of the article from
[`../data/campaigns/`](../data/campaigns): RSSI maps (`fig_rssi.png`, `fig_rssi_zones.png`),
the local packet-delivery-ratio map (`fig_pdr.png`), the PDOP map (`fig_pdop.png`) and
the constellation-visibility maps (`fig_gnss.png`).

```bash
pip install -r requirements.txt
python regenerate_maps.py          # writes analysis/output/*.png and prints the PDR gap audit
```

| File | Purpose |
| --- | --- |
| `regenerate_maps.py` | Entry point: checks the six CSV files, prints the hidden-gap audit and runs both plotting modules |
| `plot_campaign_figures.py` | RSSI, PDR and PDOP maps (UTM zone 18N, Esri World Street Map basemap) |
| `plot_gnss_visibility.py` | Median visible GPS/GLONASS/BeiDou satellites in 100 m UTM cells |
| `mdpi_figure_style.py` | Shared Matplotlib style (Palatino-compatible font P052) |

## Rules implemented

- RSSI colour scale −125 to −30 dBm; PDOP 1 to 6 (values above 6 share the top colour); PDR 0 to 100 %.
- PDR gaps: missing frames are drawn (orange crosses, linear interpolation in counter order) only for gaps of at most `MAX_GAP_FRAMES = 12`. In total 764 of 1042 missing frames (73.3 %) are not drawn.
- GNSS maps aggregate the median of visible satellites per 100 m cell and only plot GPS, GLONASS and BeiDou (no Galileo satellites were reported).
- Axes in UTM kilometres with at least one decimal, equal aspect ratio, north up, 600 dpi, white background.

## Environment notes

- **Fonts.** The article figures use P052 (metrically compatible with Palatino). Install it (`fonts-urw-base35` on Debian/Ubuntu, or point `P052_DIR` to the folder with `P052-*.otf`). If it is missing the scripts warn and fall back to another serif font; set `LORASCOPE_STRICT_FONTS=1` to fail instead.
- **Basemap tiles.** Esri tiles are downloaded on first use. Offline, the maps are drawn without the basemap (the scripts print a message) and are otherwise identical.
- The scripts are the numerical reference for the figures of the article; do not edit measurements or connect the end of one campaign to the beginning of another.

## Not included

The scripts for the antenna comparison and energy measurements of the article are not
part of this release because their raw data are not published here yet.
