#!/bin/bash

set -e

mkdir -p results/priority_backlog_raw

BACKLOGS=(1000 5000 10000 50000 100000)
MODES=(priority fifo)

for backlog in "${BACKLOGS[@]}"; do
    for mode in "${MODES[@]}"; do
        for trial in $(seq 1 10); do

            echo "backlog=$backlog mode=$mode trial=$trial"

            ./priority_latency "$mode" "$backlog" \
                > "results/priority_backlog_raw/${mode}_${backlog}_${trial}.txt"

        done
    done
done

echo "Priority backlog sweep complete."
