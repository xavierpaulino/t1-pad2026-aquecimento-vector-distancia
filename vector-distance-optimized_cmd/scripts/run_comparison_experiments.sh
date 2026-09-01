#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CONFIG_FILE="${CONFIG_FILE:-$ROOT/config/experiment.conf}"
RAW="$ROOT/data/part2/raw_measurements_comparison.csv"
T_VALUES=(32 64 128 256 512 1024 2048 4096)
VARIANTS=(baseline optimized)

if [[ ! -x "$ROOT/build/vector_distance" ]]; then
  echo "Building release executable..."
  make release
fi
make test

# Part 2 is a comparison against the frozen Part-1 baseline. Recalibrating N or
# changing CPU here would introduce a confounder, so the comparison script
# requires the existing Part-1 configuration instead of creating a new one.
if [[ ! -f "$CONFIG_FILE" ]]; then
  echo "ERROR: missing frozen Part-1 configuration: $CONFIG_FILE" >&2
  echo "Restore config/experiment.conf from the Part-1 campaign before running Part 2." >&2
  exit 2
fi
# shellcheck disable=SC1090
source "$CONFIG_FILE"
: "${N:?N missing from config}"
: "${SEED:?SEED missing from config}"
: "${REPETITIONS:?REPETITIONS missing from config}"
: "${WARMUP:?WARMUP missing from config}"
: "${CPU:?CPU missing from config}"
BATCHES="${BATCHES:-5}"

if (( REPETITIONS % BATCHES != 0 )); then
  echo "ERROR: REPETITIONS=$REPETITIONS must be divisible by BATCHES=$BATCHES" >&2
  exit 2
fi
PER_BATCH=$((REPETITIONS / BATCHES))

if ! taskset -c "$CPU" true 2>/dev/null; then
  echo "ERROR: frozen CPU=$CPU is no longer allowed/online." >&2
  exit 2
fi

mkdir -p "$ROOT/data/part2" "$ROOT/results/part2" "$ROOT/system"
scripts/collect_system_info.sh
{
  echo "EXPERIMENT=part2_baseline_vs_optimized"
  echo "N=$N"
  echo "SEED=$SEED"
  echo "REPETITIONS_PER_VARIANT_T=$REPETITIONS"
  echo "BATCHES=$BATCHES"
  echo "REPETITIONS_PER_BATCH=$PER_BATCH"
  echo "WARMUP_PER_VARIANT_T_PER_BATCH=$WARMUP"
  echo "CPU=$CPU"
  echo "T_VALUES=${T_VALUES[*]}"
  echo "VARIANTS=baseline_v3 optimized_v4"
  echo "ORDER=randomized_variant_T_blocks_deterministic"
  echo "ORDER_SEED_BASE=$SEED"
  echo "CXX=${CXX:-g++}"
  echo "RELEASE_FLAGS=${RELEASE_FLAGS:--O3 -march=native}"
} > system/part2_experiment_parameters.txt

rm -f "$RAW"
for (( batch=1; batch<=BATCHES; batch++ )); do
  mapfile -t ORDER < <(python3 - "$SEED" "$batch" <<'PY'
import random, sys
seed = int(sys.argv[1])
batch = int(sys.argv[2])
ts = [32, 64, 128, 256, 512, 1024, 2048, 4096]
variants = ["baseline", "optimized"]
items = [f"{v}:{t}" for v in variants for t in ts]
rng = random.Random(seed + 10000 + batch)
rng.shuffle(items)
print(*items, sep="\n")
PY
  )
  echo "Batch $batch/$BATCHES order: ${ORDER[*]}"
  offset=$(((batch - 1) * PER_BATCH))
  pos=0
  for item in "${ORDER[@]}"; do
    pos=$((pos + 1))
    variant="${item%%:*}"
    T="${item##*:}"
    echo "  variant=$variant T=$T N=$N CPU=$CPU reps=$PER_BATCH"
    taskset -c "$CPU" "$ROOT/build/vector_distance" \
      --variant "$variant" --n "$N" --t "$T" \
      --repetitions "$PER_BATCH" --warmup "$WARMUP" \
      --seed "$SEED" --batch "$batch" --order-position "$pos" \
      --repetition-offset "$offset" \
      --csv "$RAW" --append --quiet
  done
done

echo "Raw comparison measurements: $RAW"
echo "Next: python3 scripts/analyze_comparison.py && python3 scripts/plot_comparison.py"
