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
void apply(OrderBook& book, Event& e) {
    switch (e.type) {
        case FeedType::ADD:    book.addOrder(e.order);                       break;
        case FeedType::AMEND:  book.modifyOrder(e.order.id, e.order.quantity); break;
        case FeedType::CANCEL: book.cancelOrder(e.order.id);                 break;
    }
    for (const Trade& t : book.lastTrades())   // only ADD populates this
        book.printTrade(t);
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
    NullFlow  flow;             // ← replace with your FlowGenerator
    std::vector<Event> events;  // reused across ticks (allocation-free once warm)

    std::cout << "sim: seed=" << cfg.seed << " ticks=" << cfg.ticks << '\n';
    for (uint64_t tick = 0; tick < cfg.ticks; ++tick) {
        events.clear();
        flow.onTick(tick, rng, events);   // the synthetic market acts
        for (Event& e : events)
            apply(book, e);

        // Market-data snapshot after this tick's events. (Later, another seam in
        // this same spot: the market maker reads the book here and quotes.)
        book.printBBO(book.getBBO());
    }
    return 0;
}
