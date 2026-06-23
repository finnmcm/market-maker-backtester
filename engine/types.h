#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/lockfree/spsc_queue.hpp>

// Beast/SSL are only needed by the Coinbase WebSocket feed (BUILD_WS).
#ifdef ENABLE_WS
#include <boost/beast.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl.hpp>
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
#endif

namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//easier to read
using Price = double;
using OrderID = uint64_t;
enum class Side: uint8_t { BUY = 0, SELL };
enum class OrderType: uint8_t { MARKET = 0, LIMIT};
struct Order {
    OrderID id;
    double price;
    double quantity;
    Side side;
    OrderType type;

};
struct Level {
    std::deque<Order> orders;
    double quantity = 0;
};

using OrderIterator = std::deque<Order>::iterator;
//contains exact price, side, and position in the order book of a given order
struct OrderRef {
    Price price;
    Side side;
    OrderIterator iterator;
};
enum class FeedType : uint8_t {ADD=0, CANCEL, AMEND};  

struct Event {
    FeedType  type;
    Order order;            
};   
using EventQueue = boost::lockfree::spsc_queue<Event,
                                           boost::lockfree::capacity<4096>>;

struct BBO {
  double bestBidPrice;
  double bestAskPrice;
  double bidQty;
  double askQty;
};

// One execution between an aggressing (taker) order and a resting (maker) order.
// Under price-time priority the trade prints at the maker's resting price.
struct Trade {
  OrderID takerId;    // aggressing/incoming order
  OrderID makerId;    // resting order that was hit
  double  price;      // execution price = resting (maker) price
  double  quantity;   // shares filled in this execution
  Side    takerSide;  // BUY = aggressor lifted asks; SELL = aggressor hit bids
};
