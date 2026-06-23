#include "orderbook.h"

OrderBook::OrderBook() {

}
void OrderBook::addOrder(Order& order){
        lastTrades_.clear();
        if(order.type == OrderType::LIMIT){
            match(order);
            
            if(order.quantity != 0){
                Level& orderLevel = (order.side == Side::BUY) ? OrderBook::bids[order.price] : asks[order.price];
            orderLevel.orders.push_back(order);
            orderLevel.quantity += order.quantity;
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
    lastTrades_.clear();
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
        priceLevel.quantity -= ref.iterator->quantity;
        priceLevel.orders.erase(ref.iterator);
        if(priceLevel.orders.empty())
            OrderBook::bids.erase(levelFinder);
     }
    else{
        auto levelFinder = OrderBook::asks.find(ref.price);
        if(levelFinder == OrderBook::asks.end())
            return false;
        
        Level& priceLevel = levelFinder->second;
        priceLevel.quantity -= ref.iterator->quantity;
        priceLevel.orders.erase(ref.iterator);
        if(priceLevel.orders.empty())
            OrderBook::asks.erase(levelFinder);
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

bool OrderBook::modifyOrder(OrderID id, double newQuantity){
    lastTrades_.clear();
    auto ref = OrderBook::orderMap.find(id);
    if(ref == orderMap.end())
        return false;

    OrderRef& orderRef = ref->second;
    OrderIterator& order = orderRef.iterator;
    // signed delta: positive grows the level, negative shrinks it
    double delta = newQuantity - order->quantity;

    if(order->side == Side::BUY){
        auto levelFinder = OrderBook::bids.find(orderRef.price);
        if(levelFinder == OrderBook::bids.end())
            return false;
        levelFinder->second.quantity += delta;
    }
    else{
        auto levelFinder = OrderBook::asks.find(orderRef.price);
        if(levelFinder == OrderBook::asks.end())
            return false;
        levelFinder->second.quantity += delta;
    }
    order->quantity = newQuantity;
    if(order->side == Side::BUY)
        std::cout << "BID#" << id << " now listed at x" << newQuantity << std::endl;
    else
        std::cout << "SELL#" << id << " now listed at x" << newQuantity << std::endl;
    return true;
}
BBO OrderBook::getBBO() const{
    BBO b;
    if(bids.empty()){
        b.bestBidPrice = -1;
        b.bidQty = -1;
    }
    else{        
        b.bestBidPrice = bids.begin()->first;
        b.bidQty = bids.begin()->second.quantity;

    }
    if(asks.empty()){
      b.bestAskPrice = -1;
      b.askQty = -1;
    }
    else {
        b.bestAskPrice = asks.begin()->first;
        b.askQty = asks.begin()->second.quantity;
    }
       return b;
}
void OrderBook::printBBO(const BBO& b){
  std::cout << "Best Bid: $" << b.bestBidPrice << " for " << b.bidQty << " shares." << std::endl;
  std::cout << "Best Ask: $" << b.bestAskPrice << " for " << b.askQty << " shares." << std::endl;
}
void OrderBook::printTrade(const Trade& t){
  std::cout << "TRADE " << (t.takerSide == Side::BUY ? "BUY " : "SELL")
            << " taker#" << t.takerId << " maker#" << t.makerId
            << " $" << t.price << " x" << t.quantity << std::endl;
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
Price OrderBook::getBestBidPrice() const{
    if(bids.empty())
        return -1;
    else
        return bids.begin()->first;
}
Price OrderBook::getBestAskPrice() const{
    if(asks.empty())
        return -1;
    else
        return asks.begin()->first;
}
void OrderBook::match(Order& incomingOrder){
    //BUYS
    if(incomingOrder.side == Side::BUY){
        if(asks.empty()){
            if(incomingOrder.type == OrderType::MARKET)
                std::cout << "no current asks, unable to execute market order" << std::endl;
            return;
        }
        //MARKET ORDERS
        if(incomingOrder.type == OrderType::MARKET){
            for(auto level = asks.begin(); level != asks.end(); ++level){
            if(incomingOrder.quantity > 0)
                traversePriceLevel(incomingOrder, level->second);
            else
                break;
        }
        }
        //LIMIT ORDERS (price of incomingOrder matters)
        else{
            for(auto level = asks.begin(); level != asks.end(); ++level){
            if(incomingOrder.quantity > 0 && level->first <= incomingOrder.price)
                traversePriceLevel(incomingOrder, level->second);
            else
                break;
        }
        }
        pruneEmptyFront(asks);
        // Fills are recorded in lastTrades_ by traversePriceLevel; the consumer
        // (sim loop / market data) reads them. No stdout summary here anymore.
        return;
    }
    //SELLS
    else{
        if(bids.empty()){
            if(incomingOrder.type == OrderType::MARKET)
                std::cout << "no current bids, unable to execute market order" << std::endl;
            return;
        }
        if(incomingOrder.type == OrderType::MARKET){
            for(auto level = bids.begin(); level != bids.end(); ++level){
            if(incomingOrder.quantity > 0)
                traversePriceLevel(incomingOrder, level->second);
            else
                break;
        }
        }
        else{
            for(auto level = bids.begin(); level != bids.end(); ++level){
            if(incomingOrder.quantity > 0 && level->first >= incomingOrder.price)
                traversePriceLevel(incomingOrder, level->second);
            else
                break;
        }
        }
        pruneEmptyFront(bids);
        // Fills recorded in lastTrades_ by traversePriceLevel; consumer reads them.
        return;
    }

}
void OrderBook::traversePriceLevel(Order& incomingOrder, Level& level){
    for(auto curr = level.orders.begin(); curr != level.orders.end();){
            if(incomingOrder.quantity <= 0)
                break;

            if(curr->quantity > incomingOrder.quantity){
                // resting order partially filled; trade prints at the maker's price
                lastTrades_.push_back(Trade{incomingOrder.id, curr->id, curr->price,
                                            incomingOrder.quantity, incomingOrder.side});
                curr->quantity -= incomingOrder.quantity;
                level.quantity -= incomingOrder.quantity;
                incomingOrder.quantity = 0;
                break;
            }
            else{
                //less than or equal, meaning we filled that limit order and it should be removed from the book
                lastTrades_.push_back(Trade{incomingOrder.id, curr->id, curr->price,
                                            curr->quantity, incomingOrder.side});
                level.quantity -= curr->quantity;
                incomingOrder.quantity -= curr->quantity;
                orderMap.erase(curr->id);
                curr = level.orders.erase(curr);
                continue;
            }
            if(incomingOrder.quantity > 0)
                curr++;
        }
}


