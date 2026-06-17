#include "websocket_reader.h"
/*
    Reference:
    namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
*/
static std::string hmac_sha256_base64(const std::string& key,
                                                const std::string& message);
WebSocketReader::WebSocketReader(boost::asio::io_context& io, ssl::context& ssl_ctx, EventQueue& q)
    : api_key_(std::getenv("CB_API_KEY"       ) ? std::getenv("CB_API_KEY")       : ""),
    api_secret_(std::getenv("CB_API_SECRET"   ) ? std::getenv("CB_API_SECRET")   : ""),
    api_passphrase_(std::getenv("CB_PASSPHRASE") ? std::getenv("CB_PASSPHRASE") : ""),
    resolver_(io),
      ws_(io, ssl_ctx), queue(q) {}

void WebSocketReader::run() {

    resolver_.async_resolve(
        host,
        port,
        boost::beast::bind_front_handler(&WebSocketReader::on_resolve, shared_from_this())
    );
}
void WebSocketReader::stop(){
        // Cancel any outstanding resolve/connect operations
    resolver_.cancel();

    // Initiate a clean WebSocket close (sends a close frame)
    ws_.async_close(
        boost::beast::websocket::close_code::normal,
        beast::bind_front_handler(&WebSocketReader::on_close, shared_from_this())
    );
    }

void WebSocketReader::on_resolve(beast::error_code ec, tcp::resolver::results_type results){
    if(ec){
        std::cerr << "RESOLVE ERROR: " << ec.message() << "\n";
        return;
    }

        // Make the connection on the IP address we get from a lookup
        boost::asio::async_connect(
            beast::get_lowest_layer(ws_),   // tcp::socket&
            results,                        // resolver results (a range)
            beast::bind_front_handler(
            &WebSocketReader::on_connect,
            shared_from_this()
        )
    );
    }

void WebSocketReader::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep){
    std::cerr << "[on_connect] called, ec=" << ec.message() << "\n";
    if(ec){
        std::cerr << "CONNECT ERROR: " << ec.message() << "\n";
        return;
    }

    // *** Debug what endpoint we actually connected to:
    std::cerr << "  → Connected to " << ep.address() << "\n";

    // Set SNI Hostname (critical for many cloud providers)
    if(! ::SSL_set_tlsext_host_name(
            ws_.next_layer().native_handle(),
            host.c_str()))
    {
        beast::error_code ec2{static_cast<int>(::ERR_get_error()),
                              boost::asio::error::get_ssl_category()};
        
    }

    //SSL HANDSHAKE
    ws_.next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(
        &WebSocketReader::on_ssl_handshake,
        shared_from_this()));
}

void WebSocketReader::on_ssl_handshake(beast::error_code ec){
    std::cerr << "[on_ssl_handshake] entry, ec=" << ec.message() << "\n";
    if(ec) {
        std::cerr << "  → SSL HANDSHAKE ERROR: " << ec.message() << "\n";
        return;
    }
    std::cerr << "  → TLS handshake succeeded\n";

    // suggested timeout setting
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async-ssl");
            }));

        // Perform the websocket handshake
        ws_.async_handshake(host, "/",
            beast::bind_front_handler(
                &WebSocketReader::on_handshake,
                shared_from_this()));
    
}
void WebSocketReader::on_handshake(beast::error_code ec){
    if(ec) {
        std::cerr << "  → WS HANDSHAKE ERROR: " << ec.message() << "\n";
        return;
    }

    auto ts = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    std::cout << api_key_ << std::endl;
    std::cout << api_passphrase_ << std::endl;
    std::cout << api_secret_ << std::endl;
    // 2) Subscribe body
    nlohmann::json body = {
      {"type",        "subscribe"},
      {"product_ids", {"BTC-USD"}},
      {"channels",    {"level2"}},
      {"signature",   ""},
      {"key",         api_key_},
      {"passphrase",  ""},
      {"timestamp",   ts}
    };

    // 3) Prehash string (per Coinbase spec)
    std::string prehash = ts + "GET" + "/users/self/verify";

    // 4) Compute signature
    body["signature"] = hmac_sha256_base64(api_secret_, prehash);

    // 5) Send authenticated subscribe
    auto msg = body.dump();
    ws_.async_write(
        net::buffer(msg),
        beast::bind_front_handler(&WebSocketReader::on_subscribe_send, shared_from_this())
    );
    
}
void WebSocketReader::on_read(beast::error_code ec, std::size_t bytes_transferred){
    std::cerr << "[on_read] entry, ec=" << ec.message()
              << ", bytes=" << bytes_transferred << "\n";
    if(ec) {
        std::cerr << "  → READ ERROR: " << ec.message() << "\n";
        return;
    }

    // Convert buffer to string
    std::string msg = beast::buffers_to_string(buffer_.data());
    std::cerr << "[on_read] raw msg: " << msg << "\n";
    boost::ignore_unused(bytes_transferred);

    //PARSE coinbase msg (could be type snapshot or l2update - snapshot only sent at beginning of socket connection)
    try {
        //parse json
        auto j = nlohmann::json::parse(msg);
        std::string type = j.at("type").get<std::string>();
        if(type == "snapshot"){
            for(auto& m : j.at("bids")){
                Event e;
                e.type = FeedType::ADD;
                e.order.side = Side::BUY;
                e.order.price = std::stod(m[0].get<std::string>());
                e.order.quantity = std::stod(m[1].get<std::string>());
                queue.push(e);


            }
            for(auto& m : j.at("asks")){
                Event e;
                e.type = FeedType::ADD;
                e.order.side = Side::SELL;
                e.order.price = std::stod(m[0].get<std::string>());
                e.order.quantity = std::stod(m[1].get<std::string>());
                queue.push(e);

            }
        }
        else if(type == "l2update"){
            auto changes = j.at("changes");
            for(auto& m : changes){
                std::string orderType = m[0].get<std::string>();
                Event e;
                if(stoi(m[2].get<std::string>()) == 0)
                    e.type = FeedType::CANCEL;
                else
                    e.type = FeedType::AMEND;

                if(orderType == "buy"){
                    e.order.type = OrderType::LIMIT;
                    e.order.side = Side::BUY;
                }
                else if(orderType == "sell"){
                    e.order.type = OrderType::LIMIT;
                    e.order.side = Side::SELL;

                }
                e.order.quantity = stod(m[2].get<std::string>());
                e.order.price = stod(m[1].get<std::string>());
                queue.push(e);
            }
        }
        //clear buffer
        buffer_.consume(buffer_.size());

        //read next
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(&WebSocketReader::on_read, shared_from_this())
        );
    }
     catch(const std::exception& ex) {
        // handle parse errors
        std::cerr << "JSON parse error: " << ex.what() << "\n";
    }
    
}
void WebSocketReader::on_subscribe_send(beast::error_code ec, std::size_t bytes_transferred){
        std::cerr << "[on_subscribe_send] entry, ec=" << ec.message() << "\n";
    if(ec) {
        std::cerr << "  → SUBSCRIBE WRITE ERROR: " << ec.message() << "\n";
        return;
    }
    std::cerr << "  → Subscribe sent, kicking off read\n";
        boost::ignore_unused(bytes_transferred);

        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &WebSocketReader::on_read,
                shared_from_this()));

}
void WebSocketReader::on_close(beast::error_code ec){
    if(ec){
        std::cerr << "CLOSE ERROR: " << ec.message() << "\n";
    }
}
static std::string base64_decode(const std::string& input) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new_mem_buf(input.data(), (int)input.size());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    std::string out(input.size(), '\0');
    int decoded = BIO_read(b64, &out[0], (int)input.size());
    out.resize(decoded);
    BIO_free_all(b64);
    return out;
}
static std::string hmac_sha256_base64(const std::string& key,
                                                const std::string& message)
{
    // 1) Decode secret
    std::string secret = base64_decode(key);

    // 2) HMAC-SHA256
    unsigned int len = EVP_MAX_MD_SIZE;
    unsigned char hmac[EVP_MAX_MD_SIZE];
    HMAC(EVP_sha256(),
         (const unsigned char*)secret.data(), (int)secret.size(),
         (const unsigned char*)message.data(), (int)message.size(),
         hmac, &len);

    // 3) Re-encode to Base64
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, hmac, len);
    BIO_flush(b64);
    BUF_MEM* mem;
    BIO_get_mem_ptr(b64, &mem);
    std::string out(mem->data, mem->length);
    BIO_free_all(b64);
    return out;
}