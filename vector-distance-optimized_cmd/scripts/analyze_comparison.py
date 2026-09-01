#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = ROOT / "data" / "part2" / "raw_measurements_comparison.csv"
DEFAULT_SUMMARY = ROOT / "data" / "part2" / "summary_comparison.csv"
DEFAULT_TABLE = ROOT / "data" / "part2" / "comparison_table.csv"
EXPECTED_VARIANTS = {"baseline_v3", "optimized_v4"}
EXPECTED_T = {32, 64, 128, 256, 512, 1024, 2048, 4096}


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", type=Path, default=DEFAULT_INPUT)
    ap.add_argument("--summary", type=Path, default=DEFAULT_SUMMARY)
    ap.add_argument("--table", type=Path, default=DEFAULT_TABLE)
    args = ap.parse_args()

    df = pd.read_csv(args.input)
    required = {"variant", "N", "T", "batch", "order_position", "repetition", "elapsed_ns", "seed", "checksum"}
    missing = required.difference(df.columns)
    if missing:
        raise SystemExit(f"missing columns: {sorted(missing)}")
    if set(df["variant"].unique()) != EXPECTED_VARIANTS:
        raise SystemExit(f"unexpected variants: {sorted(df['variant'].unique())}")
    if set(df["T"].unique()) != EXPECTED_T:
        raise SystemExit(f"unexpected T set: {sorted(df['T'].unique())}")
    if df["N"].nunique() != 1:
        raise SystemExit("comparison violates constant-N requirement")
    if df["seed"].nunique() != 1:
        raise SystemExit("comparison uses more than one data seed")
    if (df["elapsed_ns"] <= 0).any():
        raise SystemExit("non-positive elapsed time found")

    counts = df.groupby(["variant", "T"]).size()
    if counts.nunique() != 1:
        raise SystemExit(f"unequal measurements: {counts.to_dict()}")

    df = df.copy()
    df["time_s"] = df["elapsed_ns"] * 1e-9
    df["vectors_per_s"] = df["N"] / df["time_s"]
    df["elements_per_s"] = (df["N"] * df["T"]) / df["time_s"]
    df["ns_per_element"] = df["elapsed_ns"] / (df["N"] * df["T"])

    rows = []
    for (variant, n, t), g in df.groupby(["variant", "N", "T"], sort=True):
        time = g["time_s"]
        mean = float(time.mean())
        std = float(time.std(ddof=1)) if len(time) > 1 else 0.0
        rows.append({
            "variant": variant,
            "N": int(n),
            "T": int(t),
            "measurements": int(len(g)),
            "mean_time_s": mean,
            "median_time_s": float(time.median()),
            "std_time_s": std,
            "cv_time_pct": 100.0 * std / mean if mean else np.nan,
            "min_time_s": float(time.min()),
            "q1_time_s": float(time.quantile(0.25)),
            "q3_time_s": float(time.quantile(0.75)),
            "max_time_s": float(time.max()),
            "median_vectors_per_s": float(g["vectors_per_s"].median()),
            "median_elements_per_s": float(g["elements_per_s"].median()),
            "median_ns_per_element": float(g["ns_per_element"].median()),
        })

    summary = pd.DataFrame(rows).sort_values(["T", "variant"])
    args.summary.parent.mkdir(parents=True, exist_ok=True)
    summary.to_csv(args.summary, index=False, float_format="%.12g")

    b = summary[summary["variant"] == "baseline_v3"].set_index("T")
    o = summary[summary["variant"] == "optimized_v4"].set_index("T")
    table = pd.DataFrame(index=sorted(EXPECTED_T))
    table.index.name = "T"
    table["baseline_median_ms"] = b["median_time_s"] * 1e3
    table["optimized_median_ms"] = o["median_time_s"] * 1e3
    table["speedup"] = b["median_time_s"] / o["median_time_s"]
    table["time_reduction_pct"] = 100.0 * (1.0 - o["median_time_s"] / b["median_time_s"])
    table["baseline_elements_per_s"] = b["median_elements_per_s"]
    table["optimized_elements_per_s"] = o["median_elements_per_s"]
    table["baseline_ns_per_element"] = b["median_ns_per_element"]
    table["optimized_ns_per_element"] = o["median_ns_per_element"]
    table["baseline_cv_pct"] = b["cv_time_pct"]
    table["optimized_cv_pct"] = o["cv_time_pct"]

    # Paired block-level speedups: each block contains both variants under the
    # same campaign period, reducing sensitivity to slow temporal drift.
    block = (df.groupby(["batch", "variant", "T"], as_index=False)["time_s"].median())
    bp = block.pivot(index=["batch", "T"], columns="variant", values="time_s").reset_index()
    bp["block_speedup"] = bp["baseline_v3"] / bp["optimized_v4"]
    block_stats = bp.groupby("T")["block_speedup"].agg(
        block_speedup_median="median",
        block_speedup_min="min",
        block_speedup_max="max",
    )
    table = table.join(block_stats)
    table.reset_index().to_csv(args.table, index=False, float_format="%.12g")

    derived = args.summary.with_name("measurements_comparison_with_metrics.csv")
    df.to_csv(derived, index=False, float_format="%.12g")
    print(f"Wrote {args.summary}")
    print(f"Wrote {args.table}")
    print(f"Wrote {derived}")


if __name__ == "__main__":
    main()
