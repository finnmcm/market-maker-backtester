
#include <deque>
#include <map>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#pragma once


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
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
