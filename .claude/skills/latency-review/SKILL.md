---
name: latency-review
description: Review the hot path for latency hazards and tell the author what to measure, without optimizing for them. Usage: /latency-review
---

Review the tick-to-quote hot path for latency hazards. Do NOT optimize or
rewrite the code.

- Flag: heap allocation on the critical path; locks where a lock-free structure
  belongs; false sharing (shared atomics not cache-line padded); unnecessary
  copies, indirection, or virtual calls; anything that inflates tail latency.
- For each, explain why it costs latency and roughly where it shows up (p50 vs
  p99.9).
- Recommend what to measure to confirm it matters (which microbenchmark, which
  percentile) before changing anything. Measurement first.
- If you can run a benchmark via Bash, do, and cite the numbers.
