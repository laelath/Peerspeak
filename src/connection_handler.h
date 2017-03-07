#ifndef connection_handler_h_INCLUDED
#define connection_handler_h_INCLUDED

#include <asio.hpp>

#include "connection.h"

class ConnectionHandler {
public:
    ConnectionHandler(asio::io_service& io_service, asio::ip::tcp::endpoint end, uint64_t id);

    void queue_write_message(MessageType type, const asio::const_buffer& buf);

private:
    void read_callback(const asio::error_code& ec, size_t num);
    void read_buffer(const asio::error_code& ec, size_t num,
                     std::function<void(std::istream&)> func);
    void write_callback(const asio::error_code& ec, size_t num);

    void read_connect(std::istream& is);
    void read_open(std::istream& is);
    void read_error(std::istream& is);

    asio::ip::tcp::socket socket;
    asio::streambuf in_buf;

    //std::map<uint64_t, std::weak_ptr<Connection>> connections;
};

#endif // connection_handler_h_INCLUDED
