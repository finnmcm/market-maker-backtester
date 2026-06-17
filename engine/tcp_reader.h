#include <boost/asio.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <array>
#include <chrono>
#include <immintrin.h> 
#include <atomic>
#include "types.h"
#pragma once


using boost::asio::ip::tcp;
using Byte  = unsigned char;                     // 8-bit byte
using Clock = std::chrono::steady_clock;  
                                           
#pragma pack(push, 1)
struct WireHeader {
    FeedType type;   
};

struct WireAdd {
    WireHeader h;
    uint64_t   orderId;
    int64_t    price;      
    int32_t    qty;
    Side    side;    
    OrderType orderType;   
};

struct WireCancel {
    WireHeader h;
    uint64_t   orderId;
};

struct WireAmend {
    WireHeader h;
    uint64_t   orderId;
    int32_t    newQty;
};
#pragma pack(pop)
class TcpReader : public std::enable_shared_from_this<TcpReader>{
    public:
    TcpReader(boost::asio::io_context& io, std::string host, uint16_t port, EventQueue& q);

    void start();
    void stop();

    private:
    boost::asio::io_context&  io_;
    tcp::resolver             resolver_;
    tcp::socket               socket_;
    boost::asio::steady_timer reconnect_timer_;

    std::string  host_;
    uint16_t     port_;
    EventQueue&       out_;

    std::array<Byte, 2> hdr_buf_;
    std::vector<Byte>   body_buf_;

    void resolve();
    void connect(const tcp::resolver::results_type& eps);
    void read_header();
    void read_body(std::size_t len);
    void handle_error(const boost::system::error_code& ec);
    void schedule_reconnect();
    bool parseBodyToEvent(const void* p, std::size_t n, Event& ev);

};