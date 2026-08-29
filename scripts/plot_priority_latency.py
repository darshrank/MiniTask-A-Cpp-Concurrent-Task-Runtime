import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("results/priority_latency.csv")

rows = []

for mode in ["priority", "fifo"]:
    x = df[df["mode"] == mode]

    rows.append({
        "mode": mode,
        "median_p50_ms": x["p50_ns"].median() / 1e6,
        "q1_p50_ms": x["p50_ns"].quantile(0.25) / 1e6,
        "q3_p50_ms": x["p50_ns"].quantile(0.75) / 1e6,
        "median_p95_ms": x["p95_ns"].median() / 1e6,
        "median_p99_ms": x["p99_ns"].median() / 1e6,
    })

summary = pd.DataFrame(rows)

fig, ax = plt.subplots(figsize=(6.5, 4.5))

yerr = [
    summary["median_p50_ms"] - summary["q1_p50_ms"],
    summary["q3_p50_ms"] - summary["median_p50_ms"],
]

ax.scatter(
    summary["mode"],
    summary["median_p50_ms"],
    s=180,
    zorder=3
)

for _, row in summary.iterrows():
    ax.annotate(
        f'{row["median_p50_ms"]:.2f} ms',
        (row["mode"], row["median_p50_ms"]),
        xytext=(0, 10),
        textcoords="offset points",
        ha="center",
        fontsize=11,
        fontweight="bold"
    )
ax.set_yscale("log")
ax.set_ylim(0.5, 500)
ax.errorbar(
    summary["mode"],
    summary["median_p50_ms"],
    yerr=yerr,
    fmt="none",
    capsize=6,
    linewidth=2,
)

ax.set_xlabel("Scheduling Mode")
ax.set_ylabel("Urgent Task p50 Latency (ms, log scale)")
ax.set_title("Priority Scheduling Reduces Urgent-Task Latency")
ax.grid(axis="y", alpha=0.25)

fig.tight_layout()
fig.savefig("results/priority_latency_p50.png", dpi=300)

print(summary.to_string(index=False))
print("\nSaved results/priority_latency_p50.png")
