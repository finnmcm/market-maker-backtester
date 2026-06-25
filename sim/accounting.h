#pragma once

#include "types.h"   // Trade, BBO, Side, OrderID
#include <cstdint>
#include <unordered_set>

// ───────────────────────────────────────────────────────────────────────────
// Accounting — harness-side bookkeeping that turns the market maker's fills into
// position, cash, and P&L. This is MECHANICS, not strategy: it only records what
// happened. The market maker decides what to quote; the ledger keeps honest
// score. (CLAUDE.md: P&L must include costs, and there is no lookahead.)
//
// Two pieces:
//   • MmOrderSet — the ids the MM currently has working, maintained by the
//     harness from the events the MM emits (ADD → working; CANCEL → gone).
//     Membership is what attributes a Trade to the MM; the MM never tags orders.
//   • Ledger — signed inventory + cash, plus mark-to-market equity against an
//     externally supplied mark price. Fees are charged per filled share (maker
//     vs taker) so fills are never frictionless.
// ───────────────────────────────────────────────────────────────────────────

// Tracks which working order ids belong to the MM. Order ids are monotonic and
// never reused, so a filled id left behind here can never be mistaken for a
// later order — leaving stale ids in the set is harmless.
class MmOrderSet {
public:
    void onAdd(OrderID id)      { ids_.insert(id); }
    void onCancel(OrderID id)   { ids_.erase(id); }
    bool owns(OrderID id) const { return ids_.count(id) != 0; }
    bool owns(const Trade& t) const { return owns(t.makerId) || owns(t.takerId); }
private:
    std::unordered_set<OrderID> ids_;
};

// Per-share trading cost. Positive = you pay; negative = a rebate (common for
// makers on real venues). Defaults are zero ONLY as a placeholder — leaving them
// at zero is exactly the "frictionless fills" CLAUDE.md forbids, so set these to
// your venue's economics before drawing any P&L conclusions.
struct Fees {
    double maker = 0.0;   // charged when the MM's resting order is hit
    double taker = 0.0;   // charged when the MM's order crosses the spread
};

class Ledger {
public:
    explicit Ledger(Fees fees = {}) : fees_(fees) {}

    // Apply one execution that touches an MM order. Determines the MM's side(s)
    // from the trade — if the MM is the maker its side is opposite the taker's;
    // if the MM is the taker its side equals takerSide — then updates signed
    // position and cash and charges the matching fee. A self-cross (both ids
    // ours) records both legs, netting to flat and paying both fees: honest, if
    // a strategy ever crosses itself.
    void apply(const Trade& t, const MmOrderSet& mine) {
        if (mine.owns(t.makerId)) record(makerSide(t.takerSide), t.price, t.quantity, fees_.maker);
        if (mine.owns(t.takerId)) record(t.takerSide,            t.price, t.quantity, fees_.taker);
    }

    // Mark-to-market equity = cash + position valued at `mark`. With no position
    // this is just realized P&L (== cash). The caller supplies the mark (e.g. the
    // current mid); the ledger does not guess one.
    double equity(double mark) const { return cash_ + position_ * mark; }

    double   position() const { return position_; }
    double   cash()     const { return cash_; }
    uint64_t fills()    const { return fills_; }

private:
    static Side makerSide(Side takerSide) {
        // taker BUY lifts asks → maker SOLD;  taker SELL hits bids → maker BOUGHT.
        return takerSide == Side::BUY ? Side::SELL : Side::BUY;
    }
    void record(Side mmSide, double price, double qty, double feePerShare) {
        const double signedQty = (mmSide == Side::BUY) ? qty : -qty;
        position_ += signedQty;
        cash_     -= signedQty * price;   // a buy spends cash, a sell earns it
        cash_     -= feePerShare * qty;   // costs are never free
        ++fills_;
    }

    Fees     fees_;
    double   position_ = 0.0;
    double   cash_     = 0.0;
    uint64_t fills_    = 0;
};
