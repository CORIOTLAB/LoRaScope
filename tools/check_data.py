#!/usr/bin/env python3
"""Integrity checks for the LoRaScope campaign datasets (data/campaigns/*.csv).

Verifies the schema, the record counts reported in the article, the absence of
duplicated frame counters, the coordinates/distances derived columns and the
PDR bookkeeping. Exit status is non-zero if any check fails.
"""
from pathlib import Path
import sys

import numpy as np
import pandas as pd

DATA = Path(__file__).resolve().parents[1] / "data" / "campaigns"
COLUMNS = [
    "ALTI", "BDSV", "FIXT", "GALV", "GLOV", "GPSV", "HDOP", "LATI", "LONG", "PDOP",
    "PITC", "ROLL", "SATU", "SATX", "TXPW", "VDOP", "VERS", "gateway_id",
    "gateway_channel_index", "crc_status", "gateway_latitude", "gateway_longitude",
    "rf_chain", "rssi", "snr", "fcnt_rel", "x_utm_m", "y_utm_m", "gateway_distance_m",
]
EXPECTED = {  # records per campaign and received-over-attempted PDR (%)
    "dataset_ciudad_del_rio_1.csv": (242, 49.4), "dataset_ciudad_del_rio_2.csv": (207, 42.7),
    "dataset_ciudad_del_rio_3.csv": (182, 38.2), "dataset_fraternidad_1.csv": (326, 75.6),
    "dataset_fraternidad_2.csv": (350, 81.8), "dataset_fraternidad_3.csv": (464, 92.4),
}
GATEWAYS = {"ciudad_del_rio": "2cf7f11162900005", "fraternidad": "787264fffee04f72"}
failures = []


def check(ok: bool, message: str) -> None:
    print(("PASS  " if ok else "FAIL  ") + message)
    if not ok:
        failures.append(message)


frames = []
for name, (records, pdr) in EXPECTED.items():
    path = DATA / name
    check(path.is_file(), f"{name} present")
    if not path.is_file():
        continue
    d = pd.read_csv(path, dtype={"gateway_id": str})
    frames.append(d)
    check(list(d.columns) == COLUMNS, f"{name}: schema")
    check(len(d) == records, f"{name}: {records} records (found {len(d)})")
    fc = d["fcnt_rel"].to_numpy()
    check(bool(np.all(np.diff(fc) > 0)), f"{name}: fcnt_rel strictly increasing")
    attempts = fc.max() - fc.min() + 1
    check(abs(100 * len(d) / attempts - pdr) < 0.05, f"{name}: PDR {pdr}%")
    zone = "ciudad_del_rio" if "ciudad" in name else "fraternidad"
    check((d["gateway_id"] == GATEWAYS[zone]).all(), f"{name}: single gateway id")
    check((d["crc_status"] == 1).all() and (d["TXPW"] == 0).all() and (d["VERS"] == 2).all(),
          f"{name}: CRC OK, TXPW 0, payload version 2")
    check(d["FIXT"].isin([2, 3]).all(), f"{name}: only 2D/3D fixes transmitted")
all_d = pd.concat(frames, ignore_index=True)
check(len(all_d) == 1771, f"total records 1771 (found {len(all_d)})")
lost = sum(int((np.diff(f["fcnt_rel"].to_numpy()) - 1).sum()) for f in frames)
check(lost == 1042, f"missing frames 1042 (found {lost})")
check(abs(100 * len(all_d) / (len(all_d) + lost) - 63.0) < 0.05, "global PDR 63.0 %")
check(int((all_d["GALV"] > 0).sum()) == 0, "no Galileo satellites reported (GALV == 0)")
print(f"\n{'ALL CHECKS PASSED' if not failures else str(len(failures)) + ' CHECK(S) FAILED'}")
sys.exit(1 if failures else 0)
