#include "types.h"
#include <iostream>
#include <vector>
#include <utility>
#pragma once
class OrderBook {
    public:
    OrderBook();
    void addOrder(Order& order);

    bool cancelOrder(OrderID id);

    bool modifyOrder(OrderID id, int newQuantity);


    void print();
    bool orderExists(OrderID id);

    const Order* getOrder(OrderID id);

    private:
    //O(1) mapping from orderID to OrderRef, which contains the position of the ACTUAL order in the order book
    std::unordered_map<OrderID, OrderRef> orderMap;

    // Buy side: highest price first
    std::map<Price, Level, std::greater<Price>> bids;

    // Sell side: lowest price first
    std::map<Price, Level> asks;

    Price getBestBid();
    Price getBestAsk();
    void match(Order& incomingOrder);
    void traversePriceLevel(std::vector<std::pair<Price, int>>& pricesFilledAt, Order& incomingOrder, Level& level);
};