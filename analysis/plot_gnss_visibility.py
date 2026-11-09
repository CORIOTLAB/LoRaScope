#!/usr/bin/env python3
"""Plot constellation visibility observed in the six LoRaScope campaigns."""

from pathlib import Path
import math

import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
import numpy as np
import pandas as pd

from mdpi_figure_style import apply_mdpi_style
import plot_campaign_figures as common  # shared basemap, extent, gateway symbol and axis format


ANALYSIS_DIR = Path(__file__).resolve().parent
REPO_DIR = ANALYSIS_DIR.parent
DATA_DIR = REPO_DIR / "data" / "campaigns"
OUTPUT = ANALYSIS_DIR / "output" / "fig_gnss.png"
CELL_M = 100.0


def wgs84_to_utm18n(latitude_deg: float, longitude_deg: float) -> tuple[float, float]:
    """Convert WGS84 latitude/longitude to UTM zone 18N."""
    a = 6378137.0
    ecc_sq = 0.00669437999014
    k0 = 0.9996
    lat = math.radians(latitude_deg)
    lon = math.radians(longitude_deg)
    lon0 = math.radians(-75.0)
    ecc_prime_sq = ecc_sq / (1.0 - ecc_sq)
    n = a / math.sqrt(1.0 - ecc_sq * math.sin(lat) ** 2)
    t = math.tan(lat) ** 2
    c = ecc_prime_sq * math.cos(lat) ** 2
    aa = math.cos(lat) * (lon - lon0)
    m = a * (
        (1 - ecc_sq / 4 - 3 * ecc_sq**2 / 64 - 5 * ecc_sq**3 / 256) * lat
        - (3 * ecc_sq / 8 + 3 * ecc_sq**2 / 32 + 45 * ecc_sq**3 / 1024)
        * math.sin(2 * lat)
        + (15 * ecc_sq**2 / 256 + 45 * ecc_sq**3 / 1024) * math.sin(4 * lat)
        - (35 * ecc_sq**3 / 3072) * math.sin(6 * lat)
    )
    easting = k0 * n * (
        aa
        + (1 - t + c) * aa**3 / 6
        + (5 - 18 * t + t**2 + 72 * c - 58 * ecc_prime_sq) * aa**5 / 120
    ) + 500000.0
    northing = k0 * (
        m
        + n
        * math.tan(lat)
        * (
            aa**2 / 2
            + (5 - t + 9 * c + 4 * c**2) * aa**4 / 24
            + (61 - 58 * t + t**2 + 600 * c - 330 * ecc_prime_sq) * aa**6 / 720
        )
    )
    return easting, northing


def main() -> None:
    # At \textwidth, 20 pt in the 13.2-in source becomes about 8.3 pt.
    apply_mdpi_style(22)
    files = sorted(DATA_DIR.glob("dataset_*.csv"))
    if len(files) != 6:
        raise RuntimeError(f"Expected six campaign files, found {len(files)}")
    data = pd.concat((pd.read_csv(path) for path in files), ignore_index=True)

    data["x_cell"] = np.floor(data["x_utm_m"] / CELL_M) * CELL_M
    data["y_cell"] = np.floor(data["y_utm_m"] / CELL_M) * CELL_M
    grouped = data.groupby(["x_cell", "y_cell"], as_index=False)[
        ["GPSV", "GLOV", "BDSV"]
    ].median()

    x_edges = np.arange(
        math.floor(data["x_utm_m"].min() / CELL_M) * CELL_M,
        math.ceil(data["x_utm_m"].max() / CELL_M) * CELL_M + CELL_M,
        CELL_M,
    )
    y_edges = np.arange(
        math.floor(data["y_utm_m"].min() / CELL_M) * CELL_M,
        math.ceil(data["y_utm_m"].max() / CELL_M) * CELL_M + CELL_M,
        CELL_M,
    )

    gateways = data[
        ["gateway_id", "gateway_latitude", "gateway_longitude"]
    ].drop_duplicates()
    gateway_points = []
    for row in gateways.itertuples(index=False):
        gx, gy = wgs84_to_utm18n(row.gateway_latitude, row.gateway_longitude)
        label = "GW-CR" if str(row.gateway_id) == "2cf7f11162900005" else "GW-FR"
        gateway_points.append((gx, gy, label))

    fields = [
        ("GPSV", "(a) GPS"),
        ("GLOV", "(b) GLONASS"),
        ("BDSV", "(c) BeiDou"),
    ]
    fig, axes = plt.subplots(1, 3, figsize=(13.2, 5.6), sharex=True, sharey=True)
    common.SMALL_FS = 22.0  # gateway labels and tile attribution: >= 8 pt after scaling to \textwidth

    for ax, (field, title) in zip(axes, fields):
        grid = np.full((len(y_edges) - 1, len(x_edges) - 1), np.nan)
        for row in grouped.itertuples(index=False):
            ix = int(round((row.x_cell - x_edges[0]) / CELL_M))
            iy = int(round((row.y_cell - y_edges[0]) / CELL_M))
            grid[iy, ix] = getattr(row, field)

        common.set_extent(ax, data, 250)  # same geographic extent as the combined RSSI/PDOP maps
        common.add_basemap(ax)            # same Esri World Street Map and brightness as the other maps
        mesh = ax.pcolormesh(
            x_edges,
            y_edges,
            grid,
            cmap="viridis",
            vmin=float(data[field].min()),
            vmax=float(data[field].max()),
            shading="flat",
            alpha=0.92,
            rasterized=True,
            zorder=5,
        )
        for (gx, gy, label) in gateway_points:
            common.draw_gateway(ax, (gx, gy), label)
        ax.set_title(title, fontsize=22)
        common.format_map_axis(ax, show_ylabel=ax is axes[0])
        ax.xaxis.label.set_size(22)
        ax.yaxis.label.set_size(22)
        ax.tick_params(axis="both", labelsize=22)
        colorbar = fig.colorbar(mesh, ax=ax, fraction=0.044, pad=0.025)
        colorbar.set_label("Visible satellites", fontsize=22)
        colorbar.ax.tick_params(labelsize=22)

    fig.subplots_adjust(left=0.065, right=0.985, bottom=0.14, top=0.93, wspace=0.22)
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(OUTPUT, dpi=600, bbox_inches="tight", facecolor="white")
    plt.close(fig)
    print(f"Wrote {OUTPUT}")
    print(f"Records: {len(data)}; occupied cells: {len(grouped)}")


if __name__ == "__main__":
    main()
