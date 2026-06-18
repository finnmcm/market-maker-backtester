---
name: critique-mm
description: Stress-test the market-maker logic by poking holes in it, without rewriting it. Usage: /critique-mm
---

Review the current market-maker logic and try to break it, as a skeptical
trader would. Do NOT rewrite it.

- Walk the quoting path and ask: what happens under sustained one-sided flow?
  When inventory hits the limit? When the spread is crossed? When a quote rests
  and fair value moves away (adverse selection)? On a burst of cancels/replaces?
- Identify the scenarios where this strategy loses money or misbehaves, and
  explain the mechanism in each case.
- Where useful, suggest which metric or scenario test would expose the weakness
  -- but let me write the fix.
- Prioritise: lead with the failure that would hurt P&L the most.
