#!/usr/bin/env python3
"""Regenerate LoRaScope campaign RSSI, PDR, and PDOP figures."""

from __future__ import annotations

from pathlib import Path
import math

import contextily as cx
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection
from matplotlib.colors import Normalize
from matplotlib.lines import Line2D
from matplotlib.ticker import FuncFormatter
import numpy as np
import pandas as pd

from mdpi_figure_style import apply_mdpi_style


ANALYSIS_DIR = Path(__file__).resolve().parent
REPO_DIR = ANALYSIS_DIR.parent
DATA_DIR = REPO_DIR / "data" / "campaigns"
FIGURE_DIR = ANALYSIS_DIR / "output"
MAX_GAP_FRAMES = 12
LOCAL_HALF_WINDOW = 12
SMALL_FS = 18.0   # source size (pt) for gateway label and tile attribution; set per figure

FILES = (
    ("CR", 1, "dataset_ciudad_del_rio_1.csv"),
    ("CR", 2, "dataset_ciudad_del_rio_2.csv"),
    ("CR", 3, "dataset_ciudad_del_rio_3.csv"),
    ("FR", 1, "dataset_fraternidad_1.csv"),
    ("FR", 2, "dataset_fraternidad_2.csv"),
    ("FR", 3, "dataset_fraternidad_3.csv"),
)


def load_data() -> pd.DataFrame:
    frames = []
    for zone, campaign, filename in FILES:
        frame = pd.read_csv(DATA_DIR / filename)
        frame["zone"] = zone
        frame["campaign"] = campaign
        frame["campaign_file"] = filename
        frames.append(frame)
    return pd.concat(frames, ignore_index=True)


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
        - (3 * ecc_sq / 8 + 3 * ecc_sq**2 / 32 + 45 * ecc_sq**3 / 1024) * math.sin(2 * lat)
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


def gateway_points(data: pd.DataFrame) -> dict[str, tuple[float, float]]:
    points: dict[str, tuple[float, float]] = {}
    for zone, group in data.groupby("zone"):
        row = group.iloc[0]
        points[zone] = wgs84_to_utm18n(row["gateway_latitude"], row["gateway_longitude"])
    return points


def set_extent(ax: plt.Axes, frame: pd.DataFrame, margin_m: float = 220.0) -> None:
    ax.set_xlim(frame["x_utm_m"].min() - margin_m, frame["x_utm_m"].max() + margin_m)
    ax.set_ylim(frame["y_utm_m"].min() - margin_m, frame["y_utm_m"].max() + margin_m)


def add_basemap(ax: plt.Axes) -> None:
    try:
        cx.add_basemap(
            ax,
            crs="EPSG:32618",
            source=cx.providers.Esri.WorldStreetMap,
            zoom=14,
            alpha=0.68,
            attribution=False,
            reset_extent=True,
        )
    except Exception as error:
        print(f"Basemap unavailable: {error}")
    ax.text(
        0.99,
        0.012,
        "Tiles © Esri",
        transform=ax.transAxes,
        ha="right",
        va="bottom",
        fontsize=SMALL_FS,
        color="0.25",
        bbox={"facecolor": "white", "alpha": 0.72, "edgecolor": "none", "pad": 1.5},
        zorder=20,
    )


def format_map_axis(ax: plt.Axes, show_ylabel: bool = True) -> None:
    # One decimal prevents distinct UTM ticks from collapsing to repeated
    # integer-kilometre labels (for example 691, 692, 692).
    km = FuncFormatter(lambda value, _: f"{value / 1000:.1f}")
    ax.xaxis.set_major_formatter(km)
    ax.yaxis.set_major_formatter(km)
    ax.set_xlabel("UTM easting (km)")
    if show_ylabel:
        ax.set_ylabel("UTM northing (km)")
    ax.set_aspect("equal", adjustable="box")
    ax.grid(False)


def draw_gateway(ax: plt.Axes, point: tuple[float, float], label: str, color: str = "#159d72") -> None:
    ax.scatter(
        [point[0]],
        [point[1]],
        s=330,
        marker="o",
        facecolor=color,
        edgecolor="white",
        linewidth=2.0,
        zorder=12,
    )
    ax.annotate(
        label,
        point,
        xytext=(0, -17),
        textcoords="offset points",
        ha="center",
        va="top",
        fontsize=SMALL_FS,
        weight="bold",
        color="0.1",
        bbox={"facecolor": "white", "alpha": 0.82, "edgecolor": "none", "pad": 1.5},
        zorder=13,
    )


def plot_route_lines(ax: plt.Axes, frame: pd.DataFrame, color: str = "#1f67d2", alpha: float = 0.55) -> None:
    for _, campaign in frame.groupby("campaign_file", sort=False):
        ax.plot(campaign["x_utm_m"], campaign["y_utm_m"], color=color, linewidth=1.1, alpha=alpha, zorder=4)


def plot_rssi_all(data: pd.DataFrame, gateways: dict[str, tuple[float, float]]) -> None:
    global SMALL_FS
    SMALL_FS = 18.5
    apply_mdpi_style(20.5)
    fig, ax = plt.subplots(figsize=(7.8, 7.0))
    set_extent(ax, data, 250)
    add_basemap(ax)
    plot_route_lines(ax, data, alpha=0.35)
    norm = Normalize(-125, -30)
    scatter = ax.scatter(
        data["x_utm_m"], data["y_utm_m"], c=data["rssi"], cmap="turbo", norm=norm,
        s=13, linewidth=0.18, edgecolor="white", zorder=7,
    )
    for zone_key, point in gateways.items():
        draw_gateway(ax, point, f"GW-{zone_key}")
    format_map_axis(ax)
    colorbar = fig.colorbar(scatter, ax=ax, fraction=0.043, pad=0.025)
    colorbar.set_label("RSSI (dBm)")
    fig.tight_layout()
    fig.savefig(FIGURE_DIR / "fig_rssi.png", dpi=600, bbox_inches="tight", facecolor="white")
    plt.close(fig)


def plot_rssi_zones(data: pd.DataFrame, gateways: dict[str, tuple[float, float]]) -> None:
    global SMALL_FS
    SMALL_FS = 18.0
    apply_mdpi_style(20.0)
    plt.rcParams["axes.titlesize"] = 20.0
    fig, axes = plt.subplots(1, 2, figsize=(12.2, 6.4))
    norm = Normalize(-125, -30)
    scatter = None
    for ax, zone, title in zip(axes, ("CR", "FR"), ("(a) Ciudad del Río", "(b) Fraternidad")):
        frame = data[data["zone"] == zone]
        set_extent(ax, frame, 210)
        add_basemap(ax)
        plot_route_lines(ax, frame)
        scatter = ax.scatter(
            frame["x_utm_m"], frame["y_utm_m"], c=frame["rssi"], cmap="turbo", norm=norm,
            s=15, linewidth=0.22, edgecolor="white", zorder=7,
        )
        draw_gateway(ax, gateways[zone], f"GW-{zone}")
        format_map_axis(ax, show_ylabel=ax is axes[0])
        ax.set_title(f"{title} ($n={len(frame)}$)")
    colorbar_axis = fig.add_axes([0.24, 0.035, 0.52, 0.028])
    colorbar = fig.colorbar(scatter, cax=colorbar_axis, orientation="horizontal")
    colorbar.set_label("RSSI (dBm)")
    fig.subplots_adjust(left=0.07, right=0.985, bottom=0.23, top=0.92, wspace=0.15)
    fig.savefig(FIGURE_DIR / "fig_rssi_zones.png", dpi=600, bbox_inches="tight", facecolor="white")
    plt.close(fig)


def plot_pdop(data: pd.DataFrame, gateways: dict[str, tuple[float, float]]) -> None:
    global SMALL_FS
    SMALL_FS = 17.6
    apply_mdpi_style(19.6)
    fig, ax = plt.subplots(figsize=(7.8, 7.0))
    set_extent(ax, data, 250)
    add_basemap(ax)
    plot_route_lines(ax, data, color="0.35", alpha=0.28)
    scatter = ax.scatter(
        data["x_utm_m"], data["y_utm_m"], c=data["PDOP"], cmap="magma_r", vmin=1.0, vmax=6.0,
        s=14, linewidth=0.18, edgecolor="white", zorder=7,
    )
    for zone_key, point in gateways.items():
        draw_gateway(ax, point, f"GW-{zone_key}")
    format_map_axis(ax)
    colorbar = fig.colorbar(scatter, ax=ax, fraction=0.043, pad=0.025, extend="max")
    colorbar.set_label("PDOP")
    fig.tight_layout()
    fig.savefig(FIGURE_DIR / "fig_pdop.png", dpi=600, bbox_inches="tight", facecolor="white")
    plt.close(fig)


def local_pdr(frame: pd.DataFrame) -> np.ndarray:
    counter = frame["fcnt_rel"].to_numpy(dtype=int)
    values = np.empty(len(counter), dtype=float)
    for index, value in enumerate(counter):
        lower = value - LOCAL_HALF_WINDOW
        upper = value + LOCAL_HALF_WINDOW
        inside = (counter >= lower) & (counter <= upper)
        first = max(lower, int(counter[0]))
        last = min(upper, int(counter[-1]))
        values[index] = 100.0 * inside.sum() / (last - first + 1)
    return values


def missing_positions(frame: pd.DataFrame) -> tuple[np.ndarray, np.ndarray]:
    counter = frame["fcnt_rel"].to_numpy(dtype=int)
    x = frame["x_utm_m"].to_numpy(dtype=float)
    y = frame["y_utm_m"].to_numpy(dtype=float)
    missing_x: list[float] = []
    missing_y: list[float] = []
    for index in range(len(counter) - 1):
        gap = int(counter[index + 1] - counter[index] - 1)
        if gap <= 0 or gap > MAX_GAP_FRAMES:
            continue
        for offset in range(1, gap + 1):
            fraction = offset / (gap + 1)
            missing_x.append(x[index] + fraction * (x[index + 1] - x[index]))
            missing_y.append(y[index] + fraction * (y[index + 1] - y[index]))
    return np.asarray(missing_x), np.asarray(missing_y)


def plot_pdr(data: pd.DataFrame, gateways: dict[str, tuple[float, float]]) -> None:
    global SMALL_FS
    SMALL_FS = 19.0
    apply_mdpi_style(21.0)
    plt.rcParams["axes.titlesize"] = 20.0
    fig, axes = plt.subplots(1, 2, figsize=(12.2, 6.4))
    norm = Normalize(0, 100)
    mappable = plt.cm.ScalarMappable(norm=norm, cmap="viridis")
    attempts = {"CR": 1452, "FR": 1361}

    for ax, zone, title in zip(axes, ("CR", "FR"), ("(a) Ciudad del Río", "(b) Fraternidad")):
        frame = data[data["zone"] == zone]
        set_extent(ax, frame, 210)
        add_basemap(ax)
        for _, campaign in frame.groupby("campaign_file", sort=False):
            campaign = campaign.reset_index(drop=True)
            pdr = local_pdr(campaign)
            points = np.column_stack((campaign["x_utm_m"], campaign["y_utm_m"]))
            segments = np.stack((points[:-1], points[1:]), axis=1)
            colors = 0.5 * (pdr[:-1] + pdr[1:])
            counter_gap = np.diff(campaign["fcnt_rel"].to_numpy(dtype=int)) - 1
            visible = counter_gap <= MAX_GAP_FRAMES
            segments = segments[visible]
            colors = colors[visible]
            collection = LineCollection(segments, cmap="viridis", norm=norm, linewidth=5.0, alpha=0.86, zorder=6)
            collection.set_array(colors)
            ax.add_collection(collection)
            miss_x, miss_y = missing_positions(campaign)
            ax.scatter(miss_x, miss_y, marker="x", s=19, linewidth=0.9, color="#ef6c00", zorder=9)
        draw_gateway(ax, gateways[zone], f"GW-{zone}", color="0.22")
        format_map_axis(ax, show_ylabel=ax is axes[0])
        ax.set_title(f"{title} ({attempts[zone]} inferred attempts)")

    legend = [Line2D([0], [0], marker="x", linestyle="none", color="#ef6c00", label="Interpolated\nmissing frame")]
    fig.legend(handles=legend, loc="lower right", bbox_to_anchor=(0.995, -0.03), frameon=False, handletextpad=0.2, borderaxespad=0.2)
    colorbar_axis = fig.add_axes([0.24, 0.035, 0.52, 0.028])
    colorbar = fig.colorbar(mappable, cax=colorbar_axis, orientation="horizontal")
    colorbar.set_label("Local packet delivery ratio (PDR, %)")
    fig.subplots_adjust(left=0.07, right=0.985, bottom=0.23, top=0.92, wspace=0.15)
    fig.savefig(FIGURE_DIR / "fig_pdr.png", dpi=600, bbox_inches="tight", facecolor="white")
    plt.close(fig)


def main() -> None:
    FIGURE_DIR.mkdir(parents=True, exist_ok=True)
    data = load_data()
    gateways = gateway_points(data)
    plot_rssi_all(data, gateways)
    plot_rssi_zones(data, gateways)
    plot_pdr(data, gateways)
    plot_pdop(data, gateways)
    print("Wrote fig_rssi.png, fig_rssi_zones.png, fig_pdr.png, and fig_pdop.png")
    print(f"PDR interpolation MAX_GAP_FRAMES={MAX_GAP_FRAMES}")


if __name__ == "__main__":
    main()
