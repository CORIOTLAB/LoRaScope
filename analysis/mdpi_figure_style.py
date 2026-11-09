"""Shared Matplotlib typography for the local MDPI Applied Sciences template."""

import os
import sys
from pathlib import Path

import matplotlib as mpl
from matplotlib import font_manager


# Definitions/mdpi.cls loads mathpazo (Palatino). P052 is the metrically
# compatible URW implementation of the Palatino family (package
# fonts-urw-base35 on Debian/Ubuntu; also shipped with TeX Live and Ghostscript).
# Set P052_DIR to the folder holding P052-*.otf if it is installed elsewhere,
# and LORASCOPE_STRICT_FONTS=1 to fail instead of falling back.
FONT_NAMES = ("P052-Roman.otf", "P052-Italic.otf", "P052-Bold.otf", "P052-BoldItalic.otf")
FONT_SEARCH_DIRS = (
    os.environ.get("P052_DIR", ""),
    "/usr/share/fonts/opentype/urw-base35",
    "/usr/share/fonts/urw-base35",
    "/usr/local/share/fonts",
    str(Path.home() / ".fonts"),
    str(Path.home() / ".local" / "share" / "fonts"),
    "C:/Windows/Fonts",
)
FALLBACK_SERIF = ["Palatino Linotype", "TeX Gyre Pagella", "Palatino", "DejaVu Serif"]


def _find_p052() -> list:
    for folder in FONT_SEARCH_DIRS:
        if not folder:
            continue
        files = [Path(folder) / name for name in FONT_NAMES]
        if all(path.is_file() for path in files):
            return files
    return []


def apply_mdpi_style(base_font_size: float) -> None:
    """Apply Palatino-compatible typography at a source-figure point size."""
    files = _find_p052()
    if not files and os.environ.get("LORASCOPE_STRICT_FONTS") == "1":
        raise FileNotFoundError("P052 font files not found; set P052_DIR")
    if files:
        for path in files:
            font_manager.fontManager.addfont(str(path))
        serif, math_font = ["P052"], "P052"
    else:
        print("WARNING: P052 not found; using a fallback serif font. Figures will not "
              "match the article typography (install fonts-urw-base35 or set P052_DIR).",
              file=sys.stderr)
        serif, math_font = FALLBACK_SERIF, "serif"

    mpl.rcParams.update(
        {
            "font.family": "serif",
            "font.serif": serif,
            "mathtext.fontset": "custom",
            "mathtext.rm": math_font,
            "mathtext.it": f"{math_font}:italic",
            "mathtext.bf": f"{math_font}:bold",
            "mathtext.cal": f"{math_font}:italic",
            "mathtext.sf": math_font,
            "mathtext.tt": math_font,
            "font.size": base_font_size,
            "axes.labelsize": base_font_size,
            "axes.titlesize": base_font_size + 1,
            "axes.titleweight": "normal",
            "xtick.labelsize": base_font_size - 2,
            "ytick.labelsize": base_font_size - 2,
            "legend.fontsize": base_font_size - 2,
            "figure.titlesize": base_font_size + 2,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
            "savefig.dpi": 600,
        }
    )
