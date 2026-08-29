import pandas as pd
import matplotlib.pyplot as plt

s = pd.read_csv("results/steal_policy_summary.csv")

x = s[
    (s["workload"] == "balanced") &
    (s["iterations"] == 1000)
]

fig, ax = plt.subplots(figsize=(7, 4.5))

for policy in ["random", "linear"]:
    p = x[x["policy"] == policy].sort_values("workers")

    yerr = [
        p["median_throughput"] - p["q1_throughput"],
        p["q3_throughput"] - p["median_throughput"]
    ]

    ax.errorbar(
        p["workers"],
        p["median_throughput"],
        yerr=yerr,
        marker="o",
        capsize=4,
        linewidth=2,
        label=policy.replace("_", " ").title()
    )

ax.set_xlabel("Worker Threads")
ax.set_ylabel("Throughput (tasks/s)")
ax.set_title("Work-Stealing Policy vs Throughput\nBalanced, 1,000-Iteration Tasks")
ax.set_xticks([1, 2, 4, 8, 16])
ax.legend()
ax.grid(alpha=0.25)

fig.tight_layout()
fig.savefig("results/steal_throughput_balanced_1000.png", dpi=300)

print("Saved results/steal_throughput_balanced_1000.png")
