#include "types.h"
#include <iostream>
#include <vector>
#include <utility>
#include <numeric>
#pragma once
class OrderBook {
    public:
    OrderBook();
    void addOrder(Order& order);

    bool cancelOrder(OrderID id);

    bool modifyOrder(OrderID id, double newQuantity);


    void print();
    bool orderExists(OrderID id);

    BBO getBBO() const;
    void printBBO(const BBO& b);
    void printTrade(const Trade& t);
    // Trades produced by the most recent add/cancel/modify (cleared at the start of each).
    // Only addOrder can populate this; cancel/modify leave it empty.
    const std::vector<Trade>& lastTrades() const { return lastTrades_; }
    const Order* getOrder(OrderID id);

    Price getBestBidPrice() const;
    Price getBestAskPrice() const;

    private:
    //O(1) mapping from orderID to OrderRef, which contains the position of the ACTUAL order in the order book
    std::unordered_map<OrderID, OrderRef> orderMap;

    // Buy side: highest price first
    std::map<Price, Level, std::greater<Price>> bids;

    // Sell side: lowest price first
    std::map<Price, Level> asks;

    
    void match(Order& incomingOrder);
    void traversePriceLevel(Order& incomingOrder, Level& level);

    // Trades from the most recent operation. Reused buffer (cleared, not reallocated)
    // so the fill path stays allocation-free once warmed.
    std::vector<Trade> lastTrades_;

    // Drop fully-consumed levels left at the front of a book side so begin()
    // (and therefore the BBO) never points at an empty husk. Templated because
    // bids and asks are maps with different comparators.
    template <typename BookSide>
    void pruneEmptyFront(BookSide& book){
        while(!book.empty() && book.begin()->second.orders.empty())
            book.erase(book.begin());
    }
};
