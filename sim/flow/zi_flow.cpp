#include "flow_generator.h"  // FlowGenerator, NullFlow, and (via types.h) Event, Order, Side, OrderType, FeedType, BBO

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <set>
#include <vector>

class ZIFlowGenerator : public FlowGenerator {
public:

    void onTick(uint64_t /*tick*/, std::mt19937_64& rng, OrderID& nextId,
                const BBO& bbo, const std::vector<Trade>& lastTrades,
                std::vector<Event>& out) override {
        std::exponential_distribution<double>  draw(EXPONENTIAL_DISTRO_LAMBDA);
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
        // supporting add and cancels for now
        // WE NEED TO KNOW WHAT TO MAKE ORDERID
        bool isAdd = (rng_normalizer(rng) > 0.2);
        bool isBuy = (rng_normalizer(rng) > 0.5);
        if (isAdd) {
            e.type = FeedType::ADD;
            // Mint a unique id from the harness-owned counter and start tracking
            // this order as resting (pruned above once it shows up in a trade).
            e.order.id = nextId++;
            resting_.insert(e.order.id);

            bool isAggressingOrder = (rng_normalizer(rng) > 0.7);
            if (isAggressingOrder) {
                // aggress with a marketable LIMIT priced at the best it can get:
                // a buyer lifts the best ask, a seller hits the best bid.
                e.order.type = OrderType::LIMIT;
                e.order.quantity = QUANTITY;
                if (isBuy) {
                    e.order.price = bbo.bestAskPrice;
                    e.order.side  = Side::BUY;
                }
                else {
                    e.order.price = bbo.bestBidPrice;
                    e.order.side  = Side::SELL;
                }
                out.push_back(e);
            }
            else {
                // hardcoding
                e.order.type = OrderType::LIMIT;
                e.order.quantity = QUANTITY;

                double midp = EMPTY_BOOK_FAIR_VALUE;
                // Empty book cases - we want to populate it more specifically in this case
                if (bbo.bestAskPrice == -1) {
                    e.order.side  = Side::SELL;
                    e.order.price = midp + std::min(MAX_OFFSET, std::floor(draw(rng)) * TICK_SIZE);
                    out.push_back(e);
                    return;
                }
                else if (bbo.bestBidPrice == -1) {
                    e.order.side  = Side::BUY;
                    e.order.price = midp - std::min(MAX_OFFSET, std::floor(draw(rng)) * TICK_SIZE);
                    out.push_back(e);
                    return;
                }
                // otherwise, we can base fair value on the BBO
                midp = bbo.bestBidPrice + (bbo.bestAskPrice - bbo.bestBidPrice) / 2;
                e.order.side = isBuy ? Side::BUY : Side::SELL;
                if (isBuy)
                    e.order.price = midp - std::min(MAX_OFFSET, std::floor(draw(rng)) * TICK_SIZE);
                else
                    e.order.price = midp + std::min(MAX_OFFSET, std::floor(draw(rng)) * TICK_SIZE);

                out.push_back(e);
            }
        }
        else {
            // handling cancel later - resting_ now holds the ids you could cancel
            // (deterministically ordered). Pick one and emit a CANCEL event.
        }
        return;
    }
private:
    double EMPTY_BOOK_FAIR_VALUE = 50.0;
    int    AVG_TICK_OFFSET = 10;            // (1/lambda = 10, we want orders to fall on average ~10 ticks from mid)
    double EXPONENTIAL_DISTRO_LAMBDA = 0.1; // used for the tick-offset draws
    double TICK_SIZE = 0.01;
    double QUANTITY = 5;
    double MAX_OFFSET = 50;

    // Ids of orders we've placed that we believe are still resting on the book.
    // Ordered (std::set) so iteration is deterministic — important when we later
    // pick a resting order to cancel and must reproduce a run from its seed.
    std::set<OrderID> resting_;

};
