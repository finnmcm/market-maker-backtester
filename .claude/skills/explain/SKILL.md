---
name: explain
description: Tutor a markets or systems concept with intuition and a comprehension check, without writing project code. Usage: /explain <concept>
argument-hint: [concept]
---

Explain this concept for someone building a market-making simulator and
preparing for a quant strategy-developer role: $ARGUMENTS

- Lead with intuition, then a concrete example, then the precise definition or
  formula.
- Markets concept? Ground it in this project (order book, market maker,
  inventory, adverse selection). Systems concept? Ground it in the hot path /
  lock-free / latency work.
- Do NOT write or modify any strategy, flow-behavior, or concurrency code.
  Teaching only.
- Give one primary-source pointer (book / paper / docs) for going deeper.
- End with a question that checks whether I actually understood it.
