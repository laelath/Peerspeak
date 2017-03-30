#ifndef connection_handler_h_INCLUDED
#define connection_handler_h_INCLUDED

#include <deque>
#include <map>

#include <asio.hpp>

#include "connection.h"

namespace peerspeak {

class PeerspeakWindow;

// Handles a connection to a discovery server and peer discovery and initiation
class ConnectionHandler {
public:
    ConnectionHandler(PeerspeakWindow *window);
    ~ConnectionHandler();

    // Initialize the connection handler
    void init(std::string ip, uint16_t port, uint64_t id);

    // Start accepting connections
    void start();

    // Write a message to the discovery server
    void write_message(MessageType type, const asio::const_buffer& buf);

    // Write a message to all connections in connections
    void write_message_all(MessageType type, const asio::const_buffer& buf);

    void send_open(uint64_t other_id);
    void send_accept(bool accept);
    void send_chat(std::string msg);

    // Close connections, called on program exit
    //void send_close();
    void stop();

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

    asio::io_service io_service;
    asio::ip::tcp::acceptor acceptor;
    asio::ip::tcp::socket socket;
    asio::ip::tcp::socket conn_sock;
    asio::ip::tcp::socket acpt_sock;

    asio::streambuf in_buf;

    PeerspeakWindow *window;

    uint64_t id;
    uint16_t count = 1;

    std::map<uint64_t, std::weak_ptr<Connection>> connections;

    // Message signatures for forwarding
    std::vector<std::pair<uint64_t, uint16_t>> message_signatures;

    friend class Connection;
};

} // namespace peerspeak

#endif // connection_handler_h_INCLUDED
