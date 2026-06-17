#include "tcp_reader.h"

extern std::atomic_bool quit;

    TcpReader::TcpReader(boost::asio::io_context& io,
              std::string              host,
              uint16_t                 port,
              EventQueue&                   q)
        : io_(io)
        , resolver_(io)
        , socket_(io)
        , reconnect_timer_(io)
        , host_(std::move(host))
        , port_(port)
        , out_(q)
    {}

    void TcpReader::start() { 
        resolve(); 
    }                  // entry point
    void TcpReader::stop () { 
        boost::system::error_code ec; socket_.close(ec); 
    }

    void TcpReader::resolve() {
        resolver_.async_resolve(
            host_, std::to_string(port_),
            [self = shared_from_this()]
            (const boost::system::error_code& ec,
             const tcp::resolver::results_type& endpoints)
            {
                if (ec) {
                    std::cerr << "resolve: " << ec.message() << '\n';
                    self->schedule_reconnect();
                } else {
                    self->connect(endpoints);
                }
            });
    }

    void TcpReader::connect(const tcp::resolver::results_type& eps) {
        boost::asio::async_connect(
            socket_, eps,
            [self = shared_from_this()]
            (const boost::system::error_code& ec,
             const tcp::endpoint&)
            {
                if (ec) {
                    std::cerr << "connect: " << ec.message() << '\n';
                    self->schedule_reconnect();
                } else {
                    self->read_header();
                }
            });
    }

    void TcpReader::read_header() {
        boost::asio::async_read(
            socket_, boost::asio::buffer(hdr_buf_),
            [self = shared_from_this()]
            (const boost::system::error_code& ec, std::size_t)
            {
                if (ec) { self->handle_error(ec); return; }

                uint16_t be_len = (self->hdr_buf_[0] << 8) | self->hdr_buf_[1];
                self->body_buf_.resize(be_len);
                self->read_body(be_len);
            });
    }

    void TcpReader::read_body(std::size_t len) {
        boost::asio::async_read(
            socket_, boost::asio::buffer(body_buf_.data(), len),
            [self = shared_from_this()]
            (const boost::system::error_code& ec, std::size_t)
            {
                if (ec) { self->handle_error(ec); return; }

                Event ev;
                if (self->parseBodyToEvent(self->body_buf_.data(), self->body_buf_.size(), ev)) {
                        if (!self->out_.push(ev))
                std::cerr << "ring overflow — consumer too slow!\n";
                    }
                self->body_buf_.clear();
                self->read_header();
    });
}
    bool TcpReader::parseBodyToEvent(const void* p, std::size_t n, Event& ev){
        if (n < sizeof(WireHeader)) return false;

        auto* hdr = static_cast<const WireHeader*>(p);
        switch (hdr->type)
        {
            case FeedType::ADD: // ADD 
            {
                if (n != sizeof(WireAdd)) return false;
                auto* m  = static_cast<const WireAdd*>(p);

                ev.type            = FeedType::ADD;
                ev.order.id        = m->orderId;
                double price = static_cast<double>(m->price) / 1e4;
                if (price <= 0.0) std::cerr << "bad price " << price << '\n';
                ev.order.price = price;
                ev.order.quantity  = m->qty;           // value 0 or 1 from Python
                Side    side     = static_cast<Side>(m->side);
                ev.order.side = side;
                OrderType orderType = static_cast<OrderType>(m->orderType);
                ev.order.type = orderType;
                return true;
            }
            case FeedType::CANCEL:  // CANCEL 
            {
                if (n != sizeof(WireCancel)) return false;
                auto* m  = static_cast<const WireCancel*>(p);

                ev.type            = FeedType::CANCEL;
                ev.order.id        = m->orderId;
                return true;
            }
            case FeedType::AMEND:  // AMEND
            {
                if (n != sizeof(WireAmend)) return false;
                auto* m  = static_cast<const WireAmend*>(p);

                ev.type            = FeedType::AMEND;
                ev.order.id        = m->orderId;
                ev.order.quantity  = m->newQty;
                return true;
            }
            default:
                return false;                   // unknown message type
    }
}


    void TcpReader::handle_error(const boost::system::error_code& ec) {
        if (ec == boost::asio::error::eof) {
            std::cerr << "EOF — shutting down\n";
        } else {
            std::cerr << "read: " << ec.message() << '\n';
        }

        quit = true;                
        socket_.close();             
        io_.stop();
    }
    void TcpReader::schedule_reconnect() {
        constexpr int BACKOFF_MS = 500;
        reconnect_timer_.expires_after(std::chrono::milliseconds(BACKOFF_MS));
        reconnect_timer_.async_wait(
            [self = shared_from_this()]
            (const boost::system::error_code& /*ec*/) { self->resolve(); });
    }
