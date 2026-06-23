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
// ───────────────────────────────────────────────────────────────────────────
class FlowGenerator {
public:
    virtual ~FlowGenerator() = default;

    // Append zero or more events for this tick into `out` (the caller clears it
    // first). `tick` is the current sim step; `rng` is the sim's one generator.
    virtual void onTick(uint64_t tick, std::mt19937_64& rng,
                        std::vector<Event>& out) = 0;
};

// Placeholder so the harness compiles and runs end-to-end before the real flow
// exists. It emits nothing, so the book stays empty. REPLACE this with your own
// generator (ideally in flow/). It deliberately makes no statistical choice —
// that decision is yours.
class NullFlow : public FlowGenerator {
public:
    void onTick(uint64_t /*tick*/, std::mt19937_64& /*rng*/,
                std::vector<Event>& /*out*/) override {}
};
