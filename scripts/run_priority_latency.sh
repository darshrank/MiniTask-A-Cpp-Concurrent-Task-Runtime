#!/usr/bin/env bash
set -euo pipefail

TRIALS=10

mkdir -p results/priority_raw

for mode in priority fifo; do
  for trial in $(seq 1 "$TRIALS"); do
    echo "Running: mode=$mode trial=$trial"

    ./priority_latency_benchmark "$mode" \
      > "results/priority_raw/${mode}_trial${trial}.txt"
  done
done
