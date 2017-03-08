#ifndef connection_handler_h_INCLUDED
#define connection_handler_h_INCLUDED

#include <asio.hpp>

#include "connection.h"

// Handles a connection to a discovery server and peer discovery and initiation
class ConnectionHandler {
public:
    ConnectionHandler(asio::io_service& io_service, asio::ip::tcp::endpoint& end, uint64_t id);

    void queue_write_message(MessageType type, const asio::const_buffer& buf);

private:
    void read_callback(const asio::error_code& ec, size_t num);
    void read_buffer(const asio::error_code& ec, size_t num,
                     std::function<void(std::istream&)> func);
    void write_callback(const asio::error_code& ec, size_t num);

    void read_connect(std::istream& is);
    void read_open(std::istream& is);
    void read_error(std::istream& is);

    void punchthrough(asio::ip::tcp::endpoint& remote);
    void punch_conn_callback(const asio::error_code& ec);
    void punch_acpt_callback(const asio::error_code& ec);

    asio::ip::tcp::acceptor acceptor;
    asio::ip::tcp::socket socket;
    asio::ip::tcp::socket conn_sock;
    asio::ip::tcp::socket acpt_sock;
    asio::streambuf in_buf;

    uint64_t id;

    std::map<uint64_t, std::weak_ptr<Connection>> connections;
};

#endif // connection_handler_h_INCLUDED
