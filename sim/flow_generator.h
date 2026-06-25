#pragma once

#include "types.h"   // Event, Order, Side, OrderType, FeedType
#include <cstdint>
#include <random>
#include <vector>

// ───────────────────────────────────────────────────────────────────────────
// FlowGenerator — the seam between the sim harness and the synthetic market.
//
// The HARNESS owns this contract and the single seeded RNG. The BEHAVIOR behind
// it is yours to design (see CLAUDE.md): what distributions drive arrivals, the
// add/cancel/amend mix, where prices sit relative to mid, how much directional
// or toxic flow there is. Implement that in your own subclass; the harness only
// ever calls onTick().
//
// Determinism rule: draw ALL randomness from the passed-in `rng`. Do not create
// your own generator and do not touch wall-clock or global rand() — drawing from
// this one seeded stream is exactly what makes a run reproducible from its seed.
//
// Market data: `bbo` is the top-of-book as of the end of the PREVIOUS tick — the
// same observable feed a real participant (and, later, the market maker) sees.
// This is how you anchor prices to mid. Note the engine's sentinel: an absent
// side is reported as price == -1 (see OrderBook::getBBO). So a valid mid exists
// ONLY when bestBidPrice != -1 && bestAskPrice != -1; otherwise the book is empty
// or one-sided and you must fall back to your own reference price (bootstrap).
// ───────────────────────────────────────────────────────────────────────────
class FlowGenerator {
public:
    virtual ~FlowGenerator() = default;

    // Append zero or more events for this tick into `out` (the caller clears it
    // first). `tick` is the current sim step; `rng` is the sim's one generator;
    // `nextId` is the harness-owned monotonic order-ID counter shared by every
    // participant — mint each new order's id as `nextId++` so IDs never collide;
    // `bbo` is the current top-of-book (see sentinel note above); `lastTrades`
    // are the fills produced last tick (empty on tick 0) — use them to drop any
    // orders you were tracking that have since been filled.
    virtual void onTick(uint64_t tick, std::mt19937_64& rng, OrderID& nextId,
                        const BBO& bbo, const std::vector<Trade>& lastTrades,
                        std::vector<Event>& out) = 0;
};

// Placeholder so the harness compiles and runs end-to-end before the real flow
// exists. It emits nothing, so the book stays empty. REPLACE this with your own
// generator (ideally in flow/). It deliberately makes no statistical choice —
// that decision is yours.
class NullFlow : public FlowGenerator {
public:
    void onTick(uint64_t /*tick*/, std::mt19937_64& /*rng*/, OrderID& /*nextId*/,
                const BBO& /*bbo*/, const std::vector<Trade>& /*lastTrades*/,
                std::vector<Event>& /*out*/) override {}
};
