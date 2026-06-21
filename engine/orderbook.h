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
    void traversePriceLevel(std::vector<std::pair<Price, int>>& pricesFilledAt, Order& incomingOrder, Level& level);

    // Drop fully-consumed levels left at the front of a book side so begin()
    // (and therefore the BBO) never points at an empty husk. Templated because
    // bids and asks are maps with different comparators.
    template <typename BookSide>
    void pruneEmptyFront(BookSide& book){
        while(!book.empty() && book.begin()->second.orders.empty())
            book.erase(book.begin());
    }
};
