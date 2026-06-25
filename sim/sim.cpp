// sim — in-process, single-threaded, deterministic backtest driver.
//
// This is the HARNESS, not the strategy. It owns the clock, the single seeded
// RNG (determinism: seed + config reproduces a run exactly), and the wiring that
// drives the engine forward by direct calls — no TCP, no threads. The synthetic
// market (FlowGenerator) and, later, the market maker plug into the seams here;
// their logic lives elsewhere and is the author's to write.
//
// Run:  ./build/debug/sim/sim --seed 42 --ticks 20

#include "orderbook.h"
#include "types.h"
#include "flow_generator.h"
#include "zi_flow.cpp"
#include "market_maker.h"   // MarketMaker seam + NullMM placeholder (strategy is author-owned, in mm/)
#include "accounting.h"     // MmOrderSet + Ledger: harness-side P&L / inventory bookkeeping

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

// Minimal arg parse: --seed N, --ticks N. Pure mechanics.
struct Config {
    uint64_t seed  = 42;
    uint64_t ticks = 20;
};

Config parseArgs(int argc, char** argv) {
    Config c;
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string key = argv[i];
        const uint64_t    val = std::strtoull(argv[i + 1], nullptr, 10);
        if      (key == "--seed")  c.seed  = val;
        else if (key == "--ticks") c.ticks = val;
        else std::cerr << "sim: ignoring unknown arg '" << key << "'\n";
    }
    return c;
}

// Apply one event to the book and drain the structured fill feed it produced.
// This mirrors book_thread_fn in engine/main.cpp — minus the queue and the
// second thread. Direct calls, one thread, fully deterministic.
void apply(OrderBook& book, Event& e, std::vector<Trade>& tickTrades) {
    switch (e.type) {
        case FeedType::ADD:    book.addOrder(e.order);                       break;
        case FeedType::AMEND:  book.modifyOrder(e.order.id, e.order.quantity); break;
        case FeedType::CANCEL: book.cancelOrder(e.order.id);                 break;
    }
    for (const Trade& t : book.lastTrades()) {  // only ADD populates this
        book.printTrade(t);
        tickTrades.push_back(t);                // accumulate so next tick's flow sees its fills
    }
}

} // namespace

int main(int argc, char** argv) {
    const Config cfg = parseArgs(argc, argv);

    //This creates a 64-bit Mersenne Twister pseudorandom number generator named rng and initializes (seeds) it with cfg.seed.
    //std::mt19937_64 = a deterministic PRNG from the C++ standard library.
    //cfg.seed = the starting seed value.
    //Using the same seed produces the same sequence of random numbers every run.
    std::mt19937_64 rng(cfg.seed);

    OrderBook book;
    OrderID   nextId = 1;        // harness owns the one monotonic ID source; 0 left as a "no order" sentinel
    ZIFlowGenerator  flow;
    NullMM           mm;          // ← replace with your MarketMaker subclass (mm/) to close the loop
    MmOrderSet       mmOrders;    // harness-tracked ids the MM has working (drives fill attribution)
    Ledger           ledger;      // signed inventory + cash + P&L; pass Fees{...} before trusting P&L (no frictionless fills)
    double           mark = 0.0;  // last good mid, used to mark inventory to market when the book is one-sided

    std::vector<Event> events;       // flow's events, reused across ticks (allocation-free once warm)
    std::vector<Event> mmEvents;     // MM's events, reused across ticks
    std::vector<Trade> lastTrades;   // fills from the PREVIOUS tick (empty on tick 0)
    std::vector<Trade> tickTrades;   // accumulator for the current tick's fills (flow + MM)
    std::vector<Trade> myFills;      // subset of tickTrades that hit the MM's orders (fed back to the MM)

    std::cout << "sim: seed=" << cfg.seed << " ticks=" << cfg.ticks << '\n';
    for (uint64_t tick = 0; tick < cfg.ticks; ++tick) {
        // ── 1. Flow acts ─────────────────────────────────────────────────────
        // Market data the flow gets to see: top-of-book as of the END of the
        // previous tick (empty book on tick 0 → sentinel -1 prices; the flow
        // detects that and falls back to its reference price). No lookahead: this
        // snapshot predates this tick's events. Note the book here may already
        // hold the MM's quotes rested last tick — this is where the MM gets hit.
        events.clear();
        const BBO bboPre = book.getBBO();
        flow.onTick(tick, rng, nextId, bboPre, lastTrades, events);

        tickTrades.clear();
        for (Event& e : events)
            apply(book, e, tickTrades);

        // ── 2. Market maker observes the post-flow book and (re)quotes ────────
        // It sees the book AFTER this tick's flow, plus its own fills so far this
        // tick (flow trading against the quotes it rested last tick). Its new
        // quotes rest and are exposed to NEXT tick's flow — the one-tick delay
        // that keeps the loop honest. This is the seam the build plan pointed to.
        const BBO bboPost = book.getBBO();

        myFills.clear();
        for (const Trade& t : tickTrades)
            if (mmOrders.owns(t)) myFills.push_back(t);

        mmEvents.clear();
        mm.onTick(tick, rng, nextId, bboPost, myFills, mmEvents);

        // Register the MM's ids BEFORE applying, so an aggressive quote that
        // crosses immediately is still attributed to the MM (as the taker).
        for (const Event& e : mmEvents) {
            if      (e.type == FeedType::ADD)    mmOrders.onAdd(e.order.id);
            else if (e.type == FeedType::CANCEL) mmOrders.onCancel(e.order.id);
        }
        for (Event& e : mmEvents)
            apply(book, e, tickTrades);

        // ── 3. Score the tick ────────────────────────────────────────────────
        // Attribute every MM-owned execution from this tick to the ledger exactly
        // once (covers both flow-hits-MM-quote and MM-crosses-the-book cases).
        for (const Trade& t : tickTrades)
            if (mmOrders.owns(t)) ledger.apply(t, mmOrders);

        const BBO bboEnd = book.getBBO();
        if (bboEnd.bestBidPrice != -1 && bboEnd.bestAskPrice != -1)
            mark = 0.5 * (bboEnd.bestBidPrice + bboEnd.bestAskPrice);   // last good mid

        book.printBBO(bboEnd);
        std::cout << "  mm: pos=" << ledger.position()
                  << " cash="    << ledger.cash()
                  << " equity="  << ledger.equity(mark)
                  << " fills="   << ledger.fills() << '\n';

        // This tick's fills become next tick's feedback for the flow (it filters
        // for its own ids). swap reuses the buffer (no realloc); the old
        // lastTrades storage is cleared at the top of next tick's apply loop.
        std::swap(lastTrades, tickTrades);
    }
    return 0;
}
