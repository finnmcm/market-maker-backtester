
#include <iostream>
#include <deque>
#include <map>
#include <unordered_map>
#include "orderbook.h"
#include "types.h"
#include "tcp_reader.h"
#ifdef ENABLE_WS
#include "websocket_reader.h"
#endif
#include <memory>
#include <thread>
#include <functional>

inline void book_thread_fn(EventQueue& q, OrderBook& b, std::atomic_bool& stop) {
    //FOR NOW, assume ALL orders are LIMIT
    while (!stop.load(std::memory_order_relaxed)) {
        Event e;
        while (q.pop(e)) { 
            switch(e.type){
                case FeedType::ADD:{
                    b.addOrder(e.order);
                    break;
                }
                case FeedType::AMEND:{
                    b.modifyOrder(e.order.id, e.order.quantity);
                    break;
                }
                case FeedType::CANCEL:{
                    b.cancelOrder(e.order.id);
                    break;
                }
            }
            // Market-data output: structured trade prints + a moving top-of-book.
            for (const Trade& t : b.lastTrades())
                b.printTrade(t);
            b.printBBO(b.getBBO());
        }
       _mm_pause();  
    }

}
void runTcpWire(std::atomic_bool& quit){
    boost::asio::io_context io;
    EventQueue ring = EventQueue();
    OrderBook b;
    std::string HOST = "127.0.0.1";
    u_int16_t PORT = 9000;
    auto reader = std::make_shared<TcpReader>(io, HOST, PORT, ring);
    reader->start();
    std::thread net_thread([&]{ io.run(); });

    std::thread bookThr(book_thread_fn, std::ref(ring), std::ref(b), std::ref(quit));

    boost::asio::signal_set sig(io, SIGINT, SIGTERM);
    sig.async_wait([&](const boost::system::error_code&, int) {
        quit.store(true, std::memory_order_relaxed);
        reader->stop();     // close socket
        io.stop();          // unblock io.run()
    });

    net_thread.join();
    bookThr.join();
}
#ifdef ENABLE_WS
void runBtcWire(std::atomic_bool& quit){
    boost::asio::io_context io;
    boost::asio::ssl::context ssl{boost::asio::ssl::context::tlsv12_client};
    ssl.set_default_verify_paths();


    ssl.set_verify_mode(ssl::verify_peer);
    ssl.set_default_verify_paths();                 
    ssl.set_verify_mode(boost::asio::ssl::verify_peer);

    EventQueue queue;        
    OrderBook    book;

    auto reader = std::make_shared<WebSocketReader>(io, ssl, queue);
    reader->run();          // kicks off resolve → connect → handshake → subscribe
    io.run();

    //separate thread
    //std::thread ioThread([&]{ io.run(); });

    std::thread bookThr(book_thread_fn, std::ref(queue), std::ref(book), std::ref(quit));
    reader->stop();
    //ioThread.join();
}
#endif // ENABLE_WS
std::atomic<bool> quit{false};

int main(){
    runTcpWire(quit);
    //runBtcWire(quit);
    return 0;
}