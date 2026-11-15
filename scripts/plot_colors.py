#!/usr/bin/env python3
"""Plot a CSV written by log_colors.py.

    python3 plot_colors.py run1.csv            # opens a window
    python3 plot_colors.py run1.csv -o run1.png  # saves instead

Left column: raw R/G/B over time for each sensor. The background is tinted
with what classifyColor() decided, and dashed lines mark FSM state changes.
Right column: raw R vs raw G per reading, colored by classification. This is
the view for checking whether red separates from every shade of black.

Requires: pip install matplotlib
"""

import argparse
import csv

import matplotlib.pyplot as plt

CHANNELS = [("r", "R", "#d62728"), ("g", "G", "#2ca02c"), ("b", "B", "#1f77b4")]
CLASS_COLORS = {
    "Dark": "#555555",
    "Red": "#d62728",
    "Yellow": "#e6b800",
    "Blue": "#1f77b4",
}


def load(path):
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        raise SystemExit(f"{path} has no data rows")
    t0 = int(rows[0]["bot_ms"])
    for row in rows:
        row["t"] = (int(row["bot_ms"]) - t0) / 1000.0
        for side in "LR":
            for ch, _, _ in CHANNELS:
                row[f"{side}_{ch}"] = int(row[f"{side}_{ch}"])
    return rows


def shade_classes(ax, rows, side):
    """Tint spans of time by the detected color (skips Dark)."""
    start = 0
    for i in range(1, len(rows) + 1):
        if i == len(rows) or rows[i][f"{side}_color"] != rows[start][f"{side}_color"]:
            cls = rows[start][f"{side}_color"]
            if cls != "Dark":
                end = rows[i]["t"] if i < len(rows) else rows[-1]["t"]
                ax.axvspan(
                    rows[start]["t"],
                    end,
                    color=CLASS_COLORS.get(cls, "#999"),
                    alpha=0.15,
                    lw=0,
                )
            start = i


def mark_states(ax, rows, label):
    prev = None
    for row in rows:
        if row["state"] != prev:
            ax.axvline(row["t"], color="#999", lw=0.8, ls="--")
            if label:
                ax.text(
                    row["t"],
                    0.98,
                    " " + row["state"],
                    transform=ax.get_xaxis_transform(),
                    rotation=90,
                    fontsize=7,
                    color="#555",
                    ha="right",
                    va="top",
                )
            prev = row["state"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv")
    ap.add_argument("-o", "--out", help="save to this image instead of showing")
    args = ap.parse_args()
    rows = load(args.csv)
    t = [r["t"] for r in rows]

    fig, axes = plt.subplots(
        2, 2, figsize=(14, 8), gridspec_kw={"width_ratios": [3, 1]}
    )
    for row_i, (side, name) in enumerate([("L", "Left sensor"), ("R", "Right sensor")]):
        ax = axes[row_i][0]
        shade_classes(ax, rows, side)
        mark_states(ax, rows, label=(row_i == 0))
        for ch, ch_name, color in CHANNELS:
            ax.plot(
                t, [r[f"{side}_{ch}"] for r in rows], color=color, lw=1.5, label=ch_name
            )
        ax.set_title(name, loc="left", fontsize=11)
        ax.set_ylabel("raw ADC counts (LED on − ambient)")
        ax.grid(alpha=0.25)
        ax.legend(loc="upper right", fontsize=8, frameon=False)
        ax.spines[["top", "right"]].set_visible(False)

        sc = axes[row_i][1]
        for cls, color in CLASS_COLORS.items():
            pts = [r for r in rows if r[f"{side}_color"] == cls]
            if pts:
                sc.scatter(
                    [p[f"{side}_g"] for p in pts],
                    [p[f"{side}_r"] for p in pts],
                    s=12,
                    color=color,
                    alpha=0.6,
                    label=f"{cls} ({len(pts)})",
                )
        sc.set_xlabel("raw G")
        sc.set_ylabel("raw R")
        sc.set_title(f"{name}: R vs G", loc="left", fontsize=11)
        sc.grid(alpha=0.25)
        sc.legend(fontsize=8, frameon=False)
        sc.spines[["top", "right"]].set_visible(False)

    axes[1][0].set_xlabel("time (s)")
    axes[0][0].sharex(axes[1][0])
    fig.tight_layout()
    if args.out:
        fig.savefig(args.out, dpi=150)
        print(f"saved {args.out}")
    else:
        plt.show()


if __name__ == "__main__":
    main()
