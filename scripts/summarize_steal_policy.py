import pandas as pd

df = pd.read_csv("results/steal_policy.csv")

summary = (
    df.groupby(["policy", "workload", "iterations", "workers"])
      .agg(
          median_throughput=("throughput", "median"),
          q1_throughput=("throughput", lambda x: x.quantile(0.25)),
          q3_throughput=("throughput", lambda x: x.quantile(0.75)),
          median_attempts=("steal_attempts", "median"),
          median_successes=("successful_steals", "median"),
      )
      .reset_index()
)

summary["steal_success_rate"] = (
    summary["median_successes"] / summary["median_attempts"]
).fillna(0)

summary["probes_per_task"] = summary["median_attempts"] / 1_000_000

summary.to_csv("results/steal_policy_summary.csv", index=False)

print(summary.to_string(index=False))
print("\nWrote results/steal_policy_summary.csv")
