
#include "json.hpp"
#include <iostream>
#include <string>
#include "types.h"
#include <boost/beast/core.hpp>
#include <openssl/ssl.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#pragma once

class WebSocketReader : public std::enable_shared_from_this<WebSocketReader> {
public:
    WebSocketReader(boost::asio::io_context&, boost::asio::ssl::context&, EventQueue& queue);  
    void run();
    void stop();
private:    
    std::string api_key_;
    std::string api_secret_;
    std::string api_passphrase_; //env creds

    std::string port = "443"; //coinbase port
    std::string host = "ws-feed.exchange.coinbase.com";
    std::string target = "/"; //root path

    //lockfree queue
    EventQueue& queue;

    //weird websocket stuff
    boost::beast::flat_buffer buffer_; //for storing raw wire packets
    boost::asio::ip::tcp::resolver resolver_;
    boost::beast::websocket::stream<
    boost::beast::ssl_stream<boost::asio::ip::tcp::socket>
> ws_;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_ssl_handshake(beast::error_code ec);
    void on_handshake(beast::error_code ec);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_subscribe_send(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);
};