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

## 2026-06-23 22:48

- **Built:** First pass at the Zero-Intelligence flow generator (`sim/flow/zi_flow.cpp`) — a real `FlowGenerator` subclass. *Status: written and reviewed, NOT yet compiled or wired.* `sim.cpp` still constructs `NullFlow`, and `flow/` isn't in CMake, so none of this has actually been built or run — the in-editor "header not found" diagnostics are just the linter missing include paths, but they also mean nothing here is verified yet.
  - Fixed a pile of compile-blockers in the first draft: `onTick` signature didn't match the base class and took `rng` *by value* (would copy the generator and silently break determinism); a stray duplicated copy of `flow_generator.h` pasted into the `.cpp`; undefined `dist` (meant the `uniform_real` dist), distributions called as `draw(rng())` instead of `draw(rng)`; `bbo.bestBuyPrice` (real field is `bestBidPrice`); `e.side` (lives on `e.order.side`); malformed ternaries; and `EXPONENTIAL_DISTRO_LAMBDA` declared `int = 0.1` → truncates to **0**, which is UB for an exponential dist.
  - Changed the aggressing order from a `MARKET` to a marketable `LIMIT` priced at the best it can get (buyer at best ask, seller at best bid).
  - **Order-ID ownership:** added a single monotonic `OrderID nextId` owned by the harness (`sim.cpp` `main`), passed *by reference* into `onTick`. Considered a small `IdAllocator` wrapper class but dropped it — a bare `OrderID&` member is simpler and equivalent for a single-author repo; the wrapper only would've prevented a participant from clobbering the counter.
  - **Fill feedback loop (the harness plumbing for tracking live orders):** `apply()` now accumulates each op's `lastTrades()` into a per-tick buffer; the loop feeds the *previous* tick's trades into `onTick` and `std::swap`s the buffers (no realloc, no lookahead — tick 0 sees empty). ZI keeps a `std::set<OrderID> resting_`, inserts every ADD's id, and prunes by `makerId`/`takerId` of last tick's trades at the top of `onTick`.

- **Learned:**
  - **Why the harness owns the ID counter, not the participant:** IDs have to be unique across *every* participant that writes to the one book (flow now, MM later). If each minted its own, they'd collide. Single shared monotonic source = uniqueness for free, and it touches no RNG so it stays deterministic. Same ownership pattern as the seeded `rng` — shared mutable harness resources get passed by ref into the seam.
  - **`std::set` over `unordered_set` is a determinism choice, not a perf one.** Once the cancel logic picks "a resting order to cancel," it iterates `resting_`; `unordered_set`'s iteration order isn't reproducible across runs/platforms, which would break "same seed → same run." Ordered set makes the selection deterministic. (Cost: O(log n) ops vs O(1) — fine off the hot path, and this is backtest mode.)
  - **The fill-feedback channel is the same one the MM will need.** A participant can't tell from the BBO alone that *its* order was filled — a later aggressor removes it silently. So the harness has to hand fills back. Building it as "last tick's `Trade`s into `onTick`" is symmetric with how `bbo` is already delivered, and it's exactly the fill-event path the market maker consumes later — built once, reused.

- **Still fuzzy / what ZI deliberately overlooks (honest list):**
  - **ZI's intended behavior:** per tick, maybe emit one event (~80% of ticks act). An ADD is ~70% passive / ~30% aggressing; passive orders sit at `mid ± floor(Exp(λ))·tick` (exponential offset from mid, mean ~10 ticks), aggressors cross at the touch. Empty/one-sided book falls back to a hardcoded fair value (50.0). Cancels are *not implemented yet* — the `else` branch is a TODO, but `resting_` now exists to draw from.
  - **Partial fills are invisible to `resting_`.** It's a set of ids, so a maker order that only *partially* fills still rests, but we erase it anyway — we stop tracking a still-live order. Acceptable for dumb ZI (worst case: we forget we could cancel it; it lingers harmlessly). Exact tracking would need a `map<OrderID,double>` of remaining qty. Chosen simplicity.
  - **The aggressing branch reads the touch before the empty-book guard** — on a one-sided book a buyer could read `bestAskPrice == -1` and emit a LIMIT buy at −1. Not yet guarded; flagged as a behavioral fix I own.
  - **It's the "balanced noise" rung of the flow ladder** (per the 06-22 entry): no directional drift, no informed/toxic correlation with future mid-moves. So it will let the eventual MM earn the spread cleanly — it does **not** yet create real inventory risk or adverse selection. That's intentional for v1; steps 8's toxic flow is where the real lessons come.
  - Still genuinely unsure how to *construct* the informed flow later (aggression correlated with future mid without lookahead) — carried over from last session.

- **Next:** Decide the cancel policy and write the `else` branch (selection from `resting_`). Then wire `flow/` into CMake and swap `NullFlow` → `ZIFlowGenerator` in `sim.cpp` so this actually *builds and runs*. First real verification: same seed twice → byte-identical output; different seeds → different; watch the BBO move and confirm fills prune `resting_`.

## 2026-06-24 20:11

- **Built:** The harness-side scaffolding for Step 3 (market maker) — the *seam and the scorekeeping*, not the strategy. Three pieces, all mechanics, all author-boundary-safe (the strategy itself stays mine to write in `mm/`):
  - `sim/market_maker.h` — a `MarketMaker` virtual seam + `NullMM` placeholder, a deliberate clone of the `FlowGenerator` contract (same `onTick` shape, same `rng`-by-ref determinism rule, same `nextId++` id discipline). Lives in `sim/` because it's the *contract*; the real subclass goes in hook-protected `mm/`.
  - `sim/accounting.h` — `MmOrderSet` (the ids the MM currently has working) + `Ledger` (signed position, cash, mark-to-market equity, per-share maker/taker `Fees`). Pure bookkeeping: it records what happened, decides nothing.
  - `sim/sim.cpp` — restructured the tick loop into 3 labeled phases (flow acts → MM observes post-flow book & quotes → score), wired in the MM call at the seam the build plan pointed to, plus a per-tick `pos/cash/equity/fills` print.
  - **Status: compiles and builds clean** (`cmake --build --preset debug --target sim`). With `NullMM` the MM line stays flat at 0 — correct "no strategy yet" baseline. The in-editor "header not found" diagnostics are the linter missing include paths again (same as the flow session); the real compiler resolved everything via `engine_lib`'s PUBLIC include dir. Did **not** run the determinism check yet.

- **Learned:**
  - **The no-lookahead guarantee is structural, baked into phase order.** The MM quotes in phase 2 but those quotes can't trade until *next* tick's flow in phase 1. That one-tick delay is the whole point: you quote on what you can see *now* and only find out next tick whether you were filled. If a quote could be placed and filled within the same tick on that tick's info, every strategy would look artificially good — that's a lookahead bug, and the loop structure makes it impossible by construction rather than by discipline.
  - **Fill attribution keys on order-ID membership, NOT on which phase produced the trade — and this was the real insight today.** My instinct was "we apply flow events and MM events in separate loops, so phase 1 trades are flow's and phase 2 trades are the MM's." Wrong. The phase a trade is produced in only tells you who the **aggressor (taker)** was, not who the **resting counterparty (maker)** was. A resting order can come from any participant, from any earlier tick.
    - Concretely: I rest a bid in tick 5 phase 2 (no trade — passive). In tick 6 phase 1, flow sends a sell that hits it → a `Trade{taker=flow, maker=me}` is produced **inside the flow loop**, but it's *my* fill. A passive quoter mostly produces *no trades in its own phase at all*; its fills surface later, buried inside someone else's aggression.
    - So "phase 1 = flow's trades" would file away the exact fills that are a market maker's entire reason for existing — inventory and P&L would read flat forever while I'm getting picked off all day.
    - `owns(trade)` checks `makerId` *and* `takerId` against `MmOrderSet`, so it catches me whether I was the resting side (phase-1 hits) or the aggressing side (phase-2 crosses), regardless of which loop produced the trade. The ID set is the single source of truth; phase order only enforces no-lookahead.
  - **Why the membership check over the phase shortcut** (you *could* check only `makerId` in phase 1 and only `takerId` in phase 2): the both-sides membership check is simpler, handles the self-cross case for free, and stays correct if I ever restructure the loop. Not worth the cleverness.
  - **The engine `Trade` only records the *taker* side, so the ledger infers mine.** If I'm the maker, my side is the *opposite* of `takerSide` (a taker BUY lifted my ask → I sold). If I'm the taker, my side *equals* `takerSide`. Then BUY adds to position / spends cash, SELL reduces / earns. `Fees{maker,taker}` is per-share and defaults to 0 — but 0 is exactly the "frictionless fills" CLAUDE.md forbids, so I set it before trusting any P&L. Mark-to-market needs a mark the ledger refuses to guess — the harness passes the current mid.
  - **Mental model correction:** the MM doesn't *cause* trades by emitting events — it places orders into a book dominated by flow. Trades happen when any aggressor (usually flow) crosses any resting order (usually flow). I'm a *small participant in a market I don't control*, which is exactly why attribution is needed at all: most trades in `tickTrades` are flow-vs-flow and have nothing to do with me.

- **Still fuzzy:**
  - **What the v1 quoter should actually *do*** — the strategy decisions are now the open question: fair-value estimate (just mid for v1?), fixed spread width, fixed size, and the cancel-replace mechanics each tick. This is the part I own and haven't written.
  - **`MmOrderSet` has the same partial-fill blind spot as the flow's `resting_`** — it's a set of ids, so a maker order that only *partially* fills still rests but I'd treat its id as "mine" with no notion of remaining qty. Fine for attribution (each `Trade` is counted once regardless), but if the MM wants to reason about how much of a quote is still live it'll need a `map<OrderID,double>` like the flow would. Deferred.
  - What realistic `Fees` to actually plug in (maker rebate vs taker fee magnitudes) — placeholder 0 for now.
  - Carried over: how to *construct* informed/toxic flow later (aggression correlated with future mid without lookahead) — step 8, still unresolved.

- **Next:** Write the `MarketMaker` v1 subclass in `mm/` — simplest possible: fixed spread around mid, fixed size, cancel-replace each tick. Then swap `NullMM mm;` → my type in `sim.cpp`, add `mm/` to the sim include path in CMake, and set non-zero `Fees`. First verification: same seed twice → byte-identical output; watch `pos`/`cash`/`equity` actually move and confirm fills attribute correctly. This closes the end-to-end loop (Step 3, the key milestone). Then Step 4: structured logs → Python P&L/inventory plots, and watch the naive MM bleed under one-sided flow.

