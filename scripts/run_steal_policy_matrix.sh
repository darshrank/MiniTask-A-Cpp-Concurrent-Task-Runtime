#!/usr/bin/env bash
set -euo pipefail

POLICIES=("random" "linear")
WORKLOADS=("balanced" "imbalanced")
ITERS=(100 1000 10000)
TRIALS=10

mkdir -p results/raw

for policy in "${POLICIES[@]}"; do
  for workload in "${WORKLOADS[@]}"; do
    for iter in "${ITERS[@]}"; do
      for trial in $(seq 1 "$TRIALS"); do
        echo "Running: policy=$policy workload=$workload iterations=$iter trial=$trial"

        ./steal_policy_benchmark "$policy" "$iter" "$workload" \
          > "results/raw/${policy}_${workload}_${iter}_trial${trial}.txt"
      done
    done
  done
done