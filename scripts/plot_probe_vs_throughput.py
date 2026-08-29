import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("results/steal_policy.csv")

x = df[
    (df["workload"] == "balanced") &
    (df["iterations"] == 1000) &
    (df["workers"] == 8)
].copy()

x["probes_per_task"] = x["steal_attempts"] / 1_000_000

fig, ax = plt.subplots(figsize=(6.5, 4.5))

for policy in ["random", "linear"]:
    p = x[x["policy"] == policy]

    ax.scatter(
        p["probes_per_task"],
        p["throughput"],
        s=70,
        label=policy.title()
    )

ax.set_xlabel("Steal Probes per Task")
ax.set_ylabel("Throughput (tasks/s)")
ax.set_title("Probe Overhead vs Throughput\n8 Workers, Balanced, 1,000-Iteration Tasks")
ax.legend()
ax.grid(alpha=0.25)

fig.tight_layout()
fig.savefig("results/probes_vs_throughput_8w_1000.png", dpi=300)

print("Saved results/probes_vs_throughput_8w_1000.png")
