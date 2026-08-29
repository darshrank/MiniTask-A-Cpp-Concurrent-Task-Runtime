import csv
import re
from pathlib import Path

RAW_DIR = Path("results/raw")
OUT = Path("results/steal_policy.csv")

rows = []

pattern = re.compile(
    r"(random|linear)_(balanced|imbalanced)_(\d+)_trial(\d+)\.txt"
)

for path in sorted(RAW_DIR.glob("*.txt")):
    match = pattern.fullmatch(path.name)
    if not match:
        continue

    policy, workload, iterations, trial = match.groups()
    lines = path.read_text().splitlines()

    i = 0
    while i < len(lines):
        parts = lines[i].split()

        if parts and parts[0] in {"1", "2", "4", "8", "16"} and len(parts) >= 3:
            workers = int(parts[0])
            elapsed = float(parts[1])
            throughput = int(parts[2])

            steal_attempts = int(lines[i + 1].split(":")[1].strip())
            successful_steals = int(lines[i + 2].split(":")[1].strip())

            rows.append({
                "policy": policy,
                "workload": workload,
                "iterations": int(iterations),
                "trial": int(trial),
                "workers": workers,
                "elapsed_s": elapsed,
                "throughput": throughput,
                "steal_attempts": steal_attempts,
                "successful_steals": successful_steals,
            })

            i += 3
        else:
            i += 1

OUT.parent.mkdir(exist_ok=True)

with OUT.open("w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)

print(f"Wrote {len(rows)} measurements to {OUT}")
