#pragma once

#include "types.h"   // Event, Order, Side, OrderType, FeedType, BBO, Trade, OrderID
#include <cstdint>
#include <random>
#include <vector>

// ───────────────────────────────────────────────────────────────────────────
// MarketMaker — the seam between the sim harness and the strategy under test.
//
// This is the HARNESS-side contract ONLY. The STRATEGY behind it — fair-value /
// mid estimation, the spread, size, inventory skew, position limits, any pricing
// model — is AUTHOR-OWNED (see CLAUDE.md) and lives in mm/. Implement it in your
// own subclass; the harness only ever calls onTick(). Mirrors flow_generator.h
// on purpose: same shape, same determinism rule, same id discipline.
//
// Where it sits in the tick (see sim.cpp): the flow acts first and its events
// hit the book, THEN the MM observes the resulting top-of-book and (re)quotes.
// The quotes it rests are exposed to NEXT tick's flow — that one-tick delay is
// what keeps the loop honest (no lookahead): you quote on what you can see now
// and only find out next tick whether you were hit.
//
// Determinism rule (same as the flow): draw ANY randomness from the passed-in
// `rng`, never wall-clock or global rand(). Most MM strategies are deterministic
// and will simply ignore `rng`; it is here so that IF a strategy ever randomizes
// it stays on the sim's one seeded stream and runs reproduce exactly from a seed.
//
// Order identity / cancel-replace: mint every new order's id as `nextId++` (the
// one monotonic counter shared with the flow, so ids never collide). The harness
// watches the events you emit and remembers which ids are yours, so it can (a)
// hand you back ONLY your own executions in `myFills` and (b) attribute P&L to
// you — you never tag orders yourself. To cancel-replace each tick, emit CANCEL
// events for the ids you rested last tick, then ADD your new quotes.
//
// Market data: `bbo` is top-of-book AFTER this tick's flow has been applied. The
// -1 sentinel still means an empty / one-sided side (see flow_generator.h): a
// valid mid exists ONLY when bestBidPrice != -1 && bestAskPrice != -1, otherwise
// fall back to your own reference price.
//
// `myFills` are the executions against YOUR orders since your last quote (empty
// on tick 0) — use them to update your own view of inventory and which quotes
// are still live. The authoritative cash / position / P&L accounting is done
// separately by the harness ledger (accounting.h); `myFills` is for your
// strategy state, not for you to re-derive P&L.
// ───────────────────────────────────────────────────────────────────────────
class MarketMaker {
public:
    virtual ~MarketMaker() = default;

    // Append zero or more events for this tick into `out` (the caller clears it
    // first). See the header comment for the meaning of every parameter.
    virtual void onTick(uint64_t tick, std::mt19937_64& rng, OrderID& nextId,
                        const BBO& bbo, const std::vector<Trade>& myFills,
                        std::vector<Event>& out) = 0;
};

// Placeholder so the harness compiles and runs end-to-end before the real
// strategy exists. It quotes nothing, so it never trades and its P&L stays flat.
// REPLACE this with your own MarketMaker subclass in mm/ (then swap the type in
// sim.cpp and add mm/ to the sim include path). It deliberately makes no pricing
// decision — closing the loop is yours to write.
class NullMM : public MarketMaker {
public:
    void onTick(uint64_t /*tick*/, std::mt19937_64& /*rng*/, OrderID& /*nextId*/,
                const BBO& /*bbo*/, const std::vector<Trade>& /*myFills*/,
                std::vector<Event>& /*out*/) override {}
};
