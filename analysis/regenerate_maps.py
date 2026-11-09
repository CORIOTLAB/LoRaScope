#!/usr/bin/env python3
"""Rebuild the LoRaScope campaign and GNSS maps from the campaign datasets in ../data/campaigns."""

from __future__ import annotations

from pathlib import Path
import sys

import pandas as pd


HERE = Path(__file__).resolve().parent
SCRIPT_DIR = HERE
DATA_DIR = HERE.parent / "data" / "campaigns"
OUTPUT_DIR = HERE / "output"
MAX_GAP_FRAMES = 12

sys.path.insert(0, str(SCRIPT_DIR))

import plot_campaign_figures as campaign_maps  # noqa: E402
import plot_gnss_visibility as gnss_map  # noqa: E402


def report_hidden_pdr_gaps() -> None:
    """Print the missing-frame fraction that Figure 4 deliberately omits."""
    totals = {"CR": [0, 0, 0], "FR": [0, 0, 0], "ALL": [0, 0, 0]}
    for path in sorted(DATA_DIR.glob("dataset_*.csv")):
        zone = "CR" if "ciudad_del_rio" in path.name else "FR"
        counter = pd.read_csv(path, usecols=["fcnt_rel"])["fcnt_rel"].astype(int)
        gaps = counter.diff().dropna().astype(int) - 1
        lost = int(gaps[gaps > 0].sum())
        hidden = int(gaps[gaps > MAX_GAP_FRAMES].sum())
        maximum = int(gaps.max())
        for key in (zone, "ALL"):
            totals[key][0] += lost
            totals[key][1] += hidden
            totals[key][2] = max(totals[key][2], maximum)

    for key in ("CR", "FR", "ALL"):
        lost, hidden, maximum = totals[key]
        fraction = 100.0 * hidden / lost
        print(
            f"{key}: missing={lost}, hidden in gaps >{MAX_GAP_FRAMES}="
            f"{hidden} ({fraction:.1f}%), maximum gap={maximum} frames"
        )


def main() -> None:
    files = sorted(DATA_DIR.glob("dataset_*.csv"))
    if len(files) != 6:
        raise RuntimeError(f"Expected six campaign CSVs in {DATA_DIR}, found {len(files)}")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    campaign_maps.DATA_DIR = DATA_DIR
    campaign_maps.FIGURE_DIR = OUTPUT_DIR
    campaign_maps.MAX_GAP_FRAMES = MAX_GAP_FRAMES
    gnss_map.DATA_DIR = DATA_DIR
    gnss_map.OUTPUT = OUTPUT_DIR / "fig_gnss.png"

    report_hidden_pdr_gaps()
    campaign_maps.main()
    gnss_map.main()
    print(f"Rebuilt maps in {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
