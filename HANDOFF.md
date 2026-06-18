# HANDOFF — market-maker-simulator

> **For Claude Code:** Read `CLAUDE.md` first — it's the source of truth for the
> collaboration boundary, conventions, and full architecture, and it loads every
> session. This file is the *current state* of the project: where things stand,
> what's next, and the facts to carry across sessions.
>
> **Keep this file current.** Update the Status checklist and Next steps at the
> end of each working session — that's what makes it a handoff and not a stale
> README.

## TL;DR

A market-making research project on an existing limit order book matching engine.
It runs in two modes that share one strategy: a **deterministic single-threaded
backtest** mode for strategy research, and a **multi-threaded live-style** mode
over the wire for low-latency practice. It's a learning project: the two goals
are (1) market understanding and (2) hands-on concurrent / low-latency C++.

## Project structure

```
engine/        matching engine — the "exchange"   [EXISTS; build on, do not rewrite]
marketdata/    market-data output + (later) UDP feed
flow/          flow generator   [mechanics OK to assist; BEHAVIORAL DESIGN = author-owned]
mm/            market-maker strategy            [AUTHOR-OWNED — do not edit]
concurrency/   lock-free queues / ring buffers  [AUTHOR-OWNED — do not edit]
transport/     TCP order entry, UDP market data, serialization
sim/           simulation harness / clock (backtest + live-style modes)
analysis/      Python: metrics, plots, latency histograms
bench/         microbenchmarks, latency harnesses
tests/         unit + scenario tests
journal/       learning log (auto-appended by the SessionEnd hook)
.claude/       Claude Code tooling (agents, hooks, skills, settings)
.github/       CI
CLAUDE.md      project rules / collaboration boundary (SOURCE OF TRUTH)
CMakePresets.json   debug / tsan / asan / release
```

## Status

Update as you go: `[x]` done · `[~]` in progress · `[ ]` not started.

- [x] Matching engine — limit/market matching, price-time priority, packets over a sample TCP wire
- [x] Claude Code tooling installed (subagents, guard + journal hooks, tutor skills, CMake presets, CI)
- [ ] **1. Observability** — market-data output from the engine; print a moving BBO
- [ ] 2. Flow generator v1 — dumb random/Poisson flow so the book moves
- [ ] 3. Market maker v1 — fixed-spread quoter; *loop closes here*
- [ ] 4. Metrics — emit logs; Python report of P&L + inventory
- [ ] 5. Inventory management — skew + position limits (A/B vs v1)
- [ ] 6. Better fair value — microprice
- [ ] 7. Avellaneda–Stoikov — reservation price / optimal spread
- [ ] 8. Flow realism — directional + informed/toxic components
- [ ] 9. Concurrency — threads + hand-built lock-free SPSC queue on the hot path
- [ ] 10. Latency instrumentation — tick-to-quote distributions (p50/p99/p99.9)
- [ ] 11. Transport — UDP market data (seq-gap handling) + TCP order entry
- [ ] 12. Optimize — kill hot-path allocs, fix false sharing, re-measure

## Next steps (immediate)

The spine of the project is one growing artifact: engine → market-data → flow →
market maker → backtester → analysis. Smallest end-to-end loop first, then enrich.

1. **Observability (step 1).** Add a way for the engine to report top-of-book and
   recent trades after each event. Day-one target: print a BBO that changes.
2. **Flow v1 (step 2).** A minimal stochastic order-flow generator so the book is
   alive. *Mechanics only at this stage — the behavioral design comes at step 8
   and is author-owned.*
3. **Market maker v1 (step 3).** Simplest fixed-spread quoter; wire it in; track
   inventory and P&L. This closes the loop and is the key milestone.

Develop steps 1–8 in **backtest mode** (single-threaded, deterministic). Steps
9–12 are the concurrency / latency track in **live-style mode**.

## Open decisions

- Transport is settled: **UDP for market data, TCP for order entry** — wire it at
  step 11; develop everything in-process behind a clean interface until then.
- Build commands in `CLAUDE.md` are placeholders; replace with the real ones once
  the build is wired so the assistant stops guessing.

## Facts to remember (write to memory)

These are the load-bearing facts. Most already live in `CLAUDE.md` (which loads
in full every session) — keep that the canonical home. Listed here so a new
session can be brought up to speed fast.

- **Two goals:** market understanding + practiced concurrent/low-latency C++
  (hands-on reps, not just theory).
- **Boundary (author-owned, do not write):** the market-maker strategy logic
  (`mm/`), the lock-free / hot-path code (`concurrency/`), the *behavioral design*
  of the flow generator, and the interpretation of results. Claude helps with
  tooling, Python analysis, tests, transport/serialization plumbing, code review,
  and tutoring. Full detail in `CLAUDE.md`.
- **Two run modes, one strategy:** deterministic single-threaded backtest (seeded
  RNG, reproducible) and multi-threaded live-style over the wire. The same MM
  logic runs in both — so keep the strategy decoupled from harness and transport.
- **Determinism is required** of backtest mode; every strategy change is an A/B
  against the prior version.
- **Hot-path rules:** no heap allocation; lock-free over locks where warranted;
  cache-line-align shared atomics (false sharing); measure before optimizing;
  latency as distributions (p50/p99/p99.9 + tail), never averages.
- **P&L includes costs. No lookahead.**
- **Transport:** UDP market data (with sequence-gap recovery), TCP order entry.
- **Build/CI:** CMake presets `tsan` / `asan` / `release`; CI runs the sanitizers
  on every push. Latency numbers from shared CI runners are NOT authoritative —
  measure locally on a quiet, CPU-pinned machine.
- **Guardrails:** the PreToolUse hook hard-blocks edits to `mm/` and
  `concurrency/`; `@systems-reviewer` and `@markets-tutor` are read-only;
  `/explain` `/quiz` `/critique-mm` `/latency-review` are tutor-mode skills.

### How to persist these in Claude Code

- **Permanent rules → `CLAUDE.md`** (loaded in full every session). The bullets
  above are already there; treat `CLAUDE.md` as canonical and edit it there.
- **Evolving status / learnings → auto memory** (on by default; Claude records
  useful notes itself). Inspect or edit memory with the `/memory` command, or just
  say "remember that …". The old `#` inline-memory shortcut is deprecated.
- Keep auto-memory entries concise — only the first ~200 lines load at session
  start, so always-on rules belong in `CLAUDE.md`, not in the memory file.
- **Important:** `CLAUDE.md` and memory are *guidance, not enforcement*. The hard
  stop on editing protected directories is the PreToolUse hook — not anything
  written here.
