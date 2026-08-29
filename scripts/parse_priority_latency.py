import csv
import re
from pathlib import Path

RAW_DIR = Path("results/priority_raw")
OUT = Path("results/priority_latency.csv")

rows = []

pattern = re.compile(r"(priority|fifo)_trial(\d+)\.txt")

for path in sorted(RAW_DIR.glob("*.txt")):
    match = pattern.fullmatch(path.name)
    if not match:
        continue

    mode, trial = match.groups()
    text = path.read_text()

    backlog = int(re.search(
        r"LOW backlog before urgent\s*:\s*(\d+)", text
    ).group(1))

    average_ns = float(re.search(
        r"Average\s*:\s*([\d.]+)\s*ns", text
    ).group(1))

    p50_ns = int(re.search(
        r"p50\s*:\s*(\d+)\s*ns", text
    ).group(1))

    p95_ns = int(re.search(
        r"p95\s*:\s*(\d+)\s*ns", text
    ).group(1))

    p99_ns = int(re.search(
        r"p99\s*:\s*(\d+)\s*ns", text
    ).group(1))

    rows.append({
        "mode": mode,
        "trial": int(trial),
        "backlog": backlog,
        "average_ns": average_ns,
        "p50_ns": p50_ns,
        "p95_ns": p95_ns,
        "p99_ns": p99_ns,
    })

with OUT.open("w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)

print(f"Wrote {len(rows)} trials to {OUT}")
