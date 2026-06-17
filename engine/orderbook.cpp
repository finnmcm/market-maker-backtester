#include "orderbook.h"

OrderBook::OrderBook() {

}
void OrderBook::addOrder(Order& order){
        if(order.type == OrderType::LIMIT){
            match(order);
            
            if(order.quantity != 0){
                Level& orderLevel = (order.side == Side::BUY) ? OrderBook::bids[order.price] : asks[order.price];
            orderLevel.orders.push_back(order);
            //find its position in the queue and add it to the unordered map for later lookup
            OrderIterator iter;
            iter = std::prev(orderLevel.orders.end());
            OrderRef ref = {order.price, order.side, iter};
            OrderBook::orderMap[order.id] = ref;
            }
        }
        else{
            match(order);
        }
}
bool OrderBook::cancelOrder(OrderID id){
    //get position in asks/bids from the ordermap
    auto iter = OrderBook::orderMap.find(id);
    if(iter == OrderBook::orderMap.end())
        return false;
    const OrderRef& ref = iter->second;

    if(ref.side == Side::BUY){
        //find the price level (queue) in the orderbook
        auto levelFinder = OrderBook::bids.find(ref.price);
        if(levelFinder == OrderBook::bids.end())
            return false;
        //delete the specific order from that level, using the stored iterator in the orderMap
        Level& priceLevel = levelFinder->second;
        priceLevel.orders.erase(ref.iterator);
    }
    else{
        auto levelFinder = OrderBook::asks.find(ref.price);
        if(levelFinder == OrderBook::asks.end())
            return false;
        
        Level& priceLevel = levelFinder->second;
        priceLevel.orders.erase(ref.iterator);
    }
    //delete from orderMap
    OrderBook::orderMap.erase(id);
    std::cout << "Order#" << id << "cancelled" << std::endl;
    return true;

}
void OrderBook::print(){
    std::cout << "---------BOOK---------" << std::endl;
    for(auto it = bids.begin(); it != bids.end(); ++it){
        Level& level = it->second;
        for(auto i2 = level.orders.begin(); i2 != level.orders.end(); ++i2){
            Order& order = *i2;
            std::cout << "BID#" << order.id << " $" << order.price << " x" << order.quantity  << std::endl;
        }
    }
    std::cout << std::endl;
    for(auto it = asks.begin(); it != asks.end(); ++it){
        Level& level = it->second;
        for(auto i2 = level.orders.begin(); i2 != level.orders.end(); ++i2){
            Order& order = *i2;
            std::cout << "SELL#" << order.id << " $" << order.price << " x" << order.quantity  << std::endl;
        }
    }
    std::cout << std::endl;
}

bool OrderBook::modifyOrder(OrderID id, int newQuantity){
    auto ref = OrderBook::orderMap.find(id);
    if(ref == orderMap.end())
        return false;
    
    OrderIterator& order = ref->second.iterator;
    order->quantity = newQuantity;
    if(order->side == Side::BUY)
        std::cout << "BID#" << id << " now listed at x" << newQuantity << std::endl;
    else
        std::cout << "SELL#" << id << " now listed at x" << newQuantity << std::endl;
    return true;
}
bool OrderBook::orderExists(OrderID id){
    auto ref = orderMap.find(id);
    if(ref == orderMap.end())
        return false;
    return true;
}
const Order* OrderBook::getOrder(OrderID id){
    if(!orderExists(id))
        return nullptr;
    OrderIterator order = orderMap[id].iterator;
    Order* o = &(*order);
    return o;
}
Price OrderBook::getBestBid(){
    if(bids.empty())
        return -1;
    else
        return bids.rbegin()->first;
}
Price OrderBook::getBestAsk(){
    if(asks.empty())
        return -1;
    else
        return asks.rbegin()->first;
}
void OrderBook::match(Order& incomingOrder){
    //BUYS
    if(incomingOrder.side == Side::BUY){
        if(asks.empty()){
            if(incomingOrder.type == OrderType::MARKET)
                std::cout << "no current asks, unable to execute market order" << std::endl;
            return;
        }
        std::vector<std::pair<Price, int>> pricesFilledAt;
        //MARKET ORDERS
        if(incomingOrder.type == OrderType::MARKET){
            for(auto level = asks.begin(); level != asks.end(); ++level){
            if(incomingOrder.quantity > 0)
                traversePriceLevel(pricesFilledAt, incomingOrder, level->second);
            else
                break;
        }
        }
        //LIMIT ORDERS (price of incomingOrder matters)
        else{
            for(auto level = asks.begin(); level != asks.end(); ++level){
            if(incomingOrder.quantity > 0 && level->first <= incomingOrder.price)
                traversePriceLevel(pricesFilledAt, incomingOrder, level->second);
            else
                break;
        }
        }
        //calculate weighted price average of filled order
        double totalCost = 0.0;
        int totalQuantity = 0;

        for (const auto& [price, qty] : pricesFilledAt) {
            totalCost += price * qty;
            totalQuantity += qty;
        }
        double avgFillPrice = (totalQuantity > 0) ? (totalCost / totalQuantity) : 0.0;
        if(totalQuantity != 0){
        if(incomingOrder.quantity > 0){
            std::cout << "Your BUY order was partially filled at $" << avgFillPrice << " for " << totalQuantity << " shares.";
            if(incomingOrder.type == OrderType::MARKET)
            std::cout << "The remaining " << incomingOrder.quantity << " shares were discared";

            std::cout << std::endl;
        }
        else
            std::cout << "Your BUY order was successfully filled at $" << avgFillPrice << " for " << totalQuantity << " shares" << std::endl;
    }
        return;
    }
    //SELLS
    else{
        if(bids.empty()){
            if(incomingOrder.type == OrderType::MARKET)
                std::cout << "no current bids, unable to execute market order" << std::endl;
            return;
        }
        std::vector<std::pair<Price, int>> pricesFilledAt;
        if(incomingOrder.type == OrderType::MARKET){
            for(auto level = bids.begin(); level != bids.end(); ++level){
            if(incomingOrder.quantity > 0)
                traversePriceLevel(pricesFilledAt, incomingOrder, level->second);
            else
                break;
        }
        }
        else{
            for(auto level = bids.begin(); level != bids.end(); ++level){
            if(incomingOrder.quantity > 0 && level->first >= incomingOrder.price)
                traversePriceLevel(pricesFilledAt, incomingOrder, level->second);
            else
                break;
        }
        }
        //calculate weighted price average of filled order
        double totalCost = 0.0;
        int totalQuantity = 0;

        for (const auto& [price, qty] : pricesFilledAt) {
            totalCost += price * qty;
            totalQuantity += qty;
        }
        double avgFillPrice = (totalQuantity > 0) ? (totalCost / totalQuantity) : 0.0;
        if(totalQuantity != 0){
        if(incomingOrder.quantity > 0){
            std::cout << "Your SELL order was partially filled at $" << avgFillPrice << " for " << totalQuantity << " shares.";
            if(incomingOrder.type == OrderType::MARKET)
             std::cout << "The remaining " << incomingOrder.quantity << " shares were discared";
            
            std::cout << std::endl;
        }
        else
            std::cout << "Your SELL order was successfully filled at $" << avgFillPrice << " for " << totalQuantity << " shares" << std::endl;
        }
        return;
    }

}
void OrderBook::traversePriceLevel(std::vector<std::pair<Price, int>>& pricesFilledAt, Order& incomingOrder, Level& level){
    for(auto curr = level.orders.begin(); curr != level.orders.end();){
            if(incomingOrder.quantity <= 0)
                break;

            if(curr->quantity > incomingOrder.quantity){
                curr->quantity -= incomingOrder.quantity;
                pricesFilledAt.push_back({curr->price, incomingOrder.quantity});
                incomingOrder.quantity = 0;
                break;
            }
            else{
                //less than or equal, meaning we filled that limit order and it should be removed from the book
                pricesFilledAt.push_back({curr->price, curr->quantity});
                incomingOrder.quantity -= curr->quantity;
                orderMap.erase(curr->id);
                curr = level.orders.erase(curr);
                continue;
            }
            if(incomingOrder.quantity > 0)
                curr++;
        }
}


