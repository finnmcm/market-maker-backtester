#include "flow_generator.h"  // FlowGenerator, NullFlow, and (via types.h) Event, Order, Side, OrderType, FeedType, BBO

#include <cmath>
#include <cstdint>
#include <random>
#include <set>
#include <vector>

// ───────────────────────────────────────────────────────────────────────────
// Gode–Sunder ZI-C (zero-intelligence, *constrained*) flow.
//
// The market is anchored to an exogenous FUNDAMENTAL_VALUE F. Every arriving
// trader is handed a private valuation dispersed around F, then quotes a
// uniformly random price WITHIN ITS NO-LOSS BUDGET:
//   • a buyer never bids ABOVE its private value,
//   • a seller never asks BELOW its private cost.
// Randomness within the budget is what drags realized trade prices toward F —
// the convergence is a property of the constraint, not of trader intelligence.
//
// Trades are EMERGENT: a new order crosses whenever its random price laps a
// resting order on the other side, so no explicit "aggressor" path is needed.
//
// Because pricing references F directly, this path never reads the BBO: the
// price process is anchored exogenously and cannot wander off the way a mid-
// referencing random walk would.
// ───────────────────────────────────────────────────────────────────────────

constexpr double FUNDAMENTAL_VALUE = 50.0; // F: the asset's exogenous fair value
constexpr double VALUE_RANGE       = 0.50; // half-width of the private-value band
                                           // around F (price units). PRIMARY KNOB:
                                           // wider → wider book & resting spread.
constexpr double TICK_SIZE = 0.01;
constexpr double QUANTITY  = 5;

static double snapToTick(double price) {
    return std::round(price / TICK_SIZE) * TICK_SIZE;
}

class ZIFlowGenerator : public FlowGenerator {
public:

    void onTick(uint64_t /*tick*/, std::mt19937_64& rng, OrderID& nextId,
                const BBO& /*bbo*/, const std::vector<Trade>& lastTrades,
                std::vector<Event>& out) override {
        std::uniform_real_distribution<double> rng_normalizer(0.0, 1.0);

        // Drop any orders we were tracking that traded last tick: a resting order
        // that got hit (maker) or a marketable one that crossed (taker) may be
        // gone from the book. NOTE: a set can't represent partial fills — this
        // treats any fill as fully removing the order, which is fine for ZI flow;
        // refine to remaining-quantity tracking later if you want exactness.
        for (const Trade& t : lastTrades) {
            resting_.erase(t.makerId);
            resting_.erase(t.takerId);
        }

        // should event even occur?
        if (rng_normalizer(rng) < 0.2)
            return;

        Event e;
        bool isAdd = (rng_normalizer(rng) > 0.2);
        bool isBuy = (rng_normalizer(rng) > 0.5);
        if (isAdd) {
            e.type           = FeedType::ADD;
            // Mint a unique id from the harness-owned counter and start tracking
            // this order as resting. A marketable draw may cross and (fully) fill
            // instead of resting; it then shows up in next tick's lastTrades and
            // is pruned above, so the set self-corrects.
            e.order.id       = nextId++;
            e.order.type     = OrderType::LIMIT;
            e.order.quantity = QUANTITY;
            resting_.insert(e.order.id);

            // Private valuation dispersed uniformly across the band [F-R, F+R].
            // Dispersion is essential: it makes buyer and seller budgets OVERLAP,
            // so orders cross and the market is liquid. A single common value
            // would let trades happen only exactly at F (a degenerate market).
            std::uniform_real_distribution<double>
                valueDraw(FUNDAMENTAL_VALUE - VALUE_RANGE,
                          FUNDAMENTAL_VALUE + VALUE_RANGE);
            double privateValue = valueDraw(rng);

            if (isBuy) {
                e.order.side = Side::BUY;
                // no-loss budget: bid uniformly in [band floor, private value].
                std::uniform_real_distribution<double>
                    bidDraw(FUNDAMENTAL_VALUE - VALUE_RANGE, privateValue);
                e.order.price = snapToTick(bidDraw(rng));
            }
            else {
                e.order.side = Side::SELL;
                // no-loss budget: ask uniformly in [private cost, band ceiling].
                std::uniform_real_distribution<double>
                    askDraw(privateValue, FUNDAMENTAL_VALUE + VALUE_RANGE);
                e.order.price = snapToTick(askDraw(rng));
            }
            out.push_back(e);
        }
        else {
            e.type = FeedType::CANCEL;
            if (!resting_.empty()) {
              // engine's cancelOrder() is keyed purely by id (orderbook.cpp),
              // so no quantity is needed here.
              OrderID orderId = *resting_.begin();
              resting_.erase(resting_.begin());
              e.order.id = orderId;
              out.push_back(e);
            }
        }
        return;
    }
private:
    // Ids of orders we've placed that we believe are still resting on the book.
    // Ordered (std::set) so iteration is deterministic — important when we
    // pick a resting order to cancel and must reproduce a run from its seed.
    std::set<OrderID> resting_;

};
