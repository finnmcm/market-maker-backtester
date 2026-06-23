# Learning log

## 2026-06-17 22:19

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-17 22:34

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-17 22:34

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-17 22:45

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-17 22:49

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-18 21:33

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-21 10:32

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-21 17:29

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-21 17:33

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-22 21:57

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

## 2026-06-22 22:02

- **Built:** In-process backtest driver in `sim/` — single-threaded, deterministic, drives the engine by direct calls (no TCP, no threads). Owns the clock (tick loop) and the one seeded `std::mt19937_64`. Added a `FlowGenerator` virtual seam + `NullFlow` placeholder so it compiles/runs before real flow exists. Also confirmed Step 1 (observability) is actually *complete* — `printBBO`, structured `Trade` events via `lastTrades()`, all wired — and fixed stale notes/CLAUDE.md that said otherwise.

- **Learned:**
  - **Why backtest mode is deterministic + single-threaded:** it's a *controlled experiment*. To A/B two strategies you must hold the market fixed, so a P&L difference is *caused by* the strategy change, not by random luck. Threads reinject nondeterminism via scheduler interleaving even with a fixed seed, so single-threaded is near-forced. It also isolates the *market* question (does it make money?) from the *systems* question (is it fast/race-free?) — answered in two separate modes.
  - **Subclass vs seed:** the `FlowGenerator` subclass defines the *statistical law* (the distribution / kind of market); the seed picks *one sample path* from that law. → Two experiments: fix subclass+seed to A/B a strategy; fix subclass and *sweep* seeds to check an edge is robust and not overfit to one lucky path.
  - **What makes flow "interesting" isn't buy/sell — it's two axes:** (1) *aggressing vs resting* — only marketable orders trade against the MM and generate fills; resting orders just move the BBO. (2) *exogenous vs informed* — Flow v1 is dumb balanced noise (MM earns spread cleanly); the step-8 toxic flow has aggression *correlated with the subsequent mid-move*, which is what creates adverse selection. The flow is the adversary; subclasses form a ladder: balanced noise → directional (inventory risk) → informed (adverse selection).
  - **Adverse selection** = the winner's curse for liquidity providers. Your fills are a *biased sample* — you get filled precisely when the taker knows something, i.e. right before the price moves against your new position. Decompose a passive fill: `(half-spread captured ≥ 0) + (markout ≤ 0 in expectation)`. Under informed flow the markout term is negative *conditional on being filled*. → The spread isn't arbitrary friction; it's the *compensation demanded for being picked off* (Glosten–Milgrom 1985). Diagnostic to build at step 4: plot mid right after each fill vs a few ticks later (markout) — post-fill drift against you *is* adverse selection made visible.
  - **The three levers, and why they differ:**
    - *Widen spread* = a **selection** effect on *who* trades with you. Repels the price-sensitive uninformed; the informed stay because their edge still beats the wider spread. Concentrates toxicity — can't widen your way out.
    - *Quote smaller* = a **risk/exposure** lever. Scales spread-capture AND adverse-selection loss by the *same* factor, so per-unit edge is unchanged — if per-unit edge is negative, smaller size only loses *slower*. Real value: caps per-event loss (tail), rations how much the informed can extract before you reprice, slows inventory accumulation.
    - *Pull the quote (zero)* = categorically different. A quote is a *written option*; small size = a smaller option, still exercisable; zero = you never wrote the option, so fill probability on that side is exactly 0 and inventory *cannot* grow further that way. Converts you from two-sided MM into a one-directional inventory-reducer.
  - **Step-5 mapping:** *skew* = soft, continuous lean (shapes the odds); *position limit* = hard backstop (pull the quote at the risk ceiling). Need both — skew only tilts probabilities and a burst of toxic flow can walk you past tolerance one clip at a time.
  - **Latency bridge:** adverse selection is partly a *stale-quote* phenomenon — you're picked off when your quote hasn't repriced to new info yet. Small size bounds damage *per* stale quote; low latency shrinks *how long* quotes stay stale. That's why the step-10–12 latency work is a P&L lever, not just engineering vanity.

- **Still fuzzy:** How to actually *construct* informed/toxic flow — i.e. how to make aggression statistically correlate with future mid-moves without lookahead cheating. (This is the step-8 behavioral design I own.)

- **Next:** Write Flow v1 — a real `FlowGenerator` subclass (start trivial: a couple fixed limit orders to watch the BBO move and confirm the loop wires through), then layer in the stochastic behavior. Swap `NullFlow` for it in `sim.cpp:67`. Verify determinism: same seed twice → identical output; different seeds → different.

## 2026-06-22 22:30

- **Built:** 
- **Learned:** 
- **Still fuzzy:** 
- **Next:** 

