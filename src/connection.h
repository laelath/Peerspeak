#ifndef connection_h_INCLUDED
#define connection_h_INCLUDED

#include <chrono>
#include <map>
#include <string>

#include <asio.hpp>

#include "message.h"

namespace peerspeak {

class ConnectionHandler;

// Class that manages a single connection, must always be a shared_ptr.
class Connection
    : public std::enable_shared_from_this<Connection> {
public:
    // Creates a connection that manages sock and is stored in conns.
    Connection(asio::ip::tcp::socket sock, ConnectionHandler *handler);
    ~Connection();

    void close();

    // Starts the connection
    void start_connection();

    // Queues a message to write, checks to make sure it's in the right format.
    void write_message(uint16_t msg_id, MessageType type, const asio::const_buffer& buf);

private:
    // Initializers for connection
    void read_start_message(const asio::error_code& ec, size_t num);
    void read_start_buffer(const asio::error_code& ec, size_t num);

    // Callback for reading messages from socket.
    void read_callback(const asio::error_code& ec, size_t num);
    void read_buffer(const asio::error_code& ec, size_t num, uint64_t sender_id,
                     uint16_t msg_id, MessageType type, uint16_t num_bytes);

    // Callback for writing messages to socket.
    void write_callback(const asio::error_code& ec, size_t num);

    void read_chat(uint8_t *buf, uint16_t num_bytes, uint64_t sender_id);
    //void read_connect(std::istream& is);

    asio::ip::tcp::socket socket;
    asio::basic_waitable_timer<std::chrono::steady_clock> timer;
    asio::streambuf in_buf;

    ConnectionHandler *handler;

    uint64_t id;

    friend class ConnectionHandler;
};

} // namespace peerspeak

#endif // connection_h_INCLUDED
