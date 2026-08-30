import csv
import glob
import os
import re

rows = []

for path in glob.glob("results/priority_backlog_raw/*.txt"):
    name = os.path.basename(path).replace(".txt", "")
    mode, requested_backlog, trial = name.split("_")

    with open(path) as f:
        text = f.read()

    actual_backlog = int(
        re.search(r"LOW backlog before urgent\s+:\s+(\d+)", text).group(1)
    )

    p50_ns = int(
        re.search(r"p50\s+:\s+(\d+) ns", text).group(1)
    )

    p95_ns = int(
        re.search(r"p95\s+:\s+(\d+) ns", text).group(1)
    )

    p99_ns = int(
        re.search(r"p99\s+:\s+(\d+) ns", text).group(1)
    )

    rows.append({
        "mode": mode,
        "requested_backlog": int(requested_backlog),
        "trial": int(trial),
        "actual_backlog": actual_backlog,
        "p50_ms": p50_ns / 1e6,
        "p95_ms": p95_ns / 1e6,
        "p99_ms": p99_ns / 1e6,
    })

rows.sort(
    key=lambda r: (
        r["requested_backlog"],
        r["mode"],
        r["trial"]
    )
)

with open("results/priority_backlog.csv", "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)

print(f"Wrote {len(rows)} rows")
