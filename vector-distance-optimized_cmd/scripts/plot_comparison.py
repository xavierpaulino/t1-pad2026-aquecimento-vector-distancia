#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
SUMMARY = ROOT / "data" / "part2" / "summary_comparison.csv"
MEASUREMENTS = ROOT / "data" / "part2" / "measurements_comparison_with_metrics.csv"
TABLE = ROOT / "data" / "part2" / "comparison_table.csv"
OUTDIR = ROOT / "results" / "part2"
LABELS = {"baseline_v3": "Versão base", "optimized_v4": "Versão otimizada"}


def setup_x(ax, ts):
    ax.set_xscale("log", base=2)
    ax.set_xticks(ts)
    ax.set_xticklabels([str(int(t)) for t in ts])
    ax.set_xlabel("Tamanho do vetor T (elementos)")
    ax.grid(True, which="major", alpha=0.25)


def save_overlay(summary, column, ylabel, filename, scale=1.0):
    fig, ax = plt.subplots(figsize=(7.2, 4.6))
    ts = sorted(summary["T"].unique())
    for variant in ["baseline_v3", "optimized_v4"]:
        g = summary[summary["variant"] == variant].sort_values("T")
        ax.plot(g["T"], g[column] / scale, marker="o", linewidth=1.5, label=LABELS[variant])
    setup_x(ax, ts)
    ax.set_ylabel(ylabel)
    ax.legend()
    fig.tight_layout()
    fig.savefig(OUTDIR / f"{filename}.pdf", bbox_inches="tight")
    fig.savefig(OUTDIR / f"{filename}.png", dpi=220, bbox_inches="tight")
    plt.close(fig)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--summary", type=Path, default=SUMMARY)
    ap.add_argument("--measurements", type=Path, default=MEASUREMENTS)
    ap.add_argument("--table", type=Path, default=TABLE)
    args = ap.parse_args()

    summary = pd.read_csv(args.summary)
    obs = pd.read_csv(args.measurements)
    comp = pd.read_csv(args.table).sort_values("T")
    OUTDIR.mkdir(parents=True, exist_ok=True)

    save_overlay(summary, "median_time_s", "Tempo mediano de execução (s)", "execution_time_comparison")
    save_overlay(summary, "median_vectors_per_s", "Vazão mediana (milhões de vetores/s)", "vectors_per_second_comparison", 1e6)
    save_overlay(summary, "median_elements_per_s", "Vazão mediana (bilhões de elementos/s)", "elements_per_second_comparison", 1e9)
    save_overlay(summary, "median_ns_per_element", "Tempo mediano por elemento (ns/elemento)", "ns_per_element_comparison")

    # Side-by-side boxplots for direct variability comparison.
    ts = sorted(summary["T"].unique())
    data = []
    positions = []
    width = 0.28
    centers = list(range(1, len(ts) + 1))
    for idx, t in enumerate(ts, start=1):
        for variant, offset in [("baseline_v3", -0.18), ("optimized_v4", 0.18)]:
            data.append(obs.loc[(obs["T"] == t) & (obs["variant"] == variant), "time_s"].to_numpy())
            positions.append(idx + offset)
    fig, ax = plt.subplots(figsize=(7.2, 4.6))
    bp = ax.boxplot(data, positions=positions, widths=width, showfliers=True, patch_artist=False)
    ax.set_xticks(centers)
    ax.set_xticklabels([str(int(t)) for t in ts])
    ax.set_xlabel("Tamanho do vetor T (elementos)")
    ax.set_ylabel("Tempo de execução (s)")
    ax.grid(True, axis="y", alpha=0.25)
    # Legend proxies without forcing colors/styles in the data itself.
    ax.plot([], [], marker="|", linestyle="None", markersize=12, label="Versão base")
    ax.plot([], [], marker="_", linestyle="None", markersize=12, label="Versão otimizada")
    ax.legend()
    fig.tight_layout()
    fig.savefig(OUTDIR / "execution_time_variability_comparison.pdf", bbox_inches="tight")
    fig.savefig(OUTDIR / "execution_time_variability_comparison.png", dpi=220, bbox_inches="tight")
    plt.close(fig)

    # Speedup is the most direct view of optimization effect.
    fig, ax = plt.subplots(figsize=(7.2, 4.6))
    ax.plot(comp["T"], comp["speedup"], marker="o", linewidth=1.5)
    ax.axhline(1.0, linewidth=1.0, linestyle="--")
    setup_x(ax, ts)
    ax.set_ylabel("Speedup (base / otimizada)")
    fig.tight_layout()
    fig.savefig(OUTDIR / "speedup.pdf", bbox_inches="tight")
    fig.savefig(OUTDIR / "speedup.png", dpi=220, bbox_inches="tight")
    plt.close(fig)

    print(f"Gráficos comparativos gravados em {OUTDIR}")


if __name__ == "__main__":
    main()
