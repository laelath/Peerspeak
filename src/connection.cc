#include "connection.h"

#include <iostream>

#include "connection_handler.h"
#include "peerspeak_window.h"

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htonll(x) ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32)
#define ntohll(x) ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32)
#else
#define htonll(x) (x)
#define ntohll(x) (x)
#endif

namespace peerspeak {

using namespace std::placeholders;

const constexpr size_t message_header_length = sizeof(uint64_t) + sizeof(uint16_t)
                                             + sizeof(uint8_t) + sizeof(uint16_t);

Connection::Connection(asio::ip::tcp::socket sock, ConnectionHandler *handler)
    : socket(std::move(sock)),
      timer(handler->io_service, std::chrono::seconds(10)),
      handler(handler)
{
}

Connection::~Connection()
{
    auto pos = handler->connections.find(id);
    if (pos != handler->connections.end()) {
        handler->connections.erase(pos);
        std::cout << "Connection ID " << id << " closed" << std::endl;
        handler->window->recv_disconnect(id);
    }
}

void Connection::close()
{
    socket.close();
}

void Connection::start_connection()
{
    //uint64_t temp_id = htonll(this_id);
    //write_message(OPEN, asio::buffer(&temp_id, sizeof(temp_id)));
    uint8_t type = OPEN;
    uint16_t bytes = htons(sizeof handler->id);
    uint64_t temp_id = htonll(handler->id);
    std::array<asio::const_buffer, 3> data = {
        asio::buffer(&type, sizeof type),
        asio::buffer(&bytes, sizeof bytes),
        asio::buffer(&temp_id, sizeof temp_id)
    };
    asio::async_write(socket, data, asio::transfer_all(), std::bind(&Connection::write_callback,
                                                                    shared_from_this(), _1, _2));
    auto self = shared_from_this();
    timer.async_wait(
        [this, self](const asio::error_code& ec) {
            if (!ec) {
                std::cout << "Error: Socket timed out" << std::endl;
                handler->window->show_error("Error: Socket timed out");
                socket.close();
            }
        });
    asio::async_read(socket, in_buf, asio::transfer_exactly(sizeof(type) + sizeof(bytes)),
                     std::bind(&Connection::read_start_message, self, _1, _2));
}

void Connection::write_message(uint16_t msg_id, MessageType type, const asio::const_buffer& buf)
{
    switch (type) {
    case CONNECT:
        if (asio::buffer_size(buf) != 6)
            throw std::invalid_argument("CONNECT expected an IPv4 address and port");
        break;
    case OPEN:
        if (asio::buffer_size(buf) != 8)
            throw std::invalid_argument("OPEN expected a uint64_t id");
        break;
    case ADD:
        if (asio::buffer_size(buf) != 8)
            throw std::invalid_argument("ADD expected a uint64_t id");
        break;
    case ACCEPT:
        if (asio::buffer_size(buf) != 1)
            throw std::invalid_argument("ACCEPT expected a boolean byte");
        break;
    case ERROR:
    case CHAT:
        break;
    case INVALID:
        throw std::invalid_argument("Cannot send INVALID message");
    }

    uint64_t temp_id = htonll(handler->id);
    uint16_t temp_count = htons(msg_id); // Get count and increment it
    uint8_t temp_type = type;
    uint16_t bytes = htons(asio::buffer_size(buf));
    std::array<asio::const_buffer, 5> data = {
        asio::buffer(&temp_id, sizeof temp_id),
        asio::buffer(&temp_count, sizeof temp_count),
        asio::buffer(&temp_type, sizeof temp_type),
        asio::buffer(&bytes, sizeof bytes),
        buf
    };
    asio::async_write(socket, data, asio::transfer_all(), std::bind(&Connection::write_callback,
                                                                    shared_from_this(), _1, _2));
}

void Connection::read_start_message(const asio::error_code& ec, size_t num)
{
    if (!ec) {
        std::istream is(&in_buf);

        uint8_t net_type;
        uint16_t bytes;

        is.read(reinterpret_cast<char *>(&net_type), sizeof net_type);
        is.read(reinterpret_cast<char *>(&bytes), sizeof bytes);

        MessageType type = static_cast<MessageType>(net_type);
        bytes = ntohs(bytes);

        if (type != OPEN)
            return;

        asio::async_read(socket, in_buf, asio::transfer_exactly(bytes),
                         std::bind(&Connection::read_start_buffer, shared_from_this(), _1, _2));
    }
}

void Connection::read_start_buffer(const asio::error_code& ec, size_t num)
{
    if (!ec) {
        timer.cancel();
        auto self = shared_from_this();
        std::istream is(&in_buf);
        is.read(reinterpret_cast<char *>(&id), sizeof id);
        id = ntohll(id);
        if (handler->connections.find(id) == handler->connections.end())
            handler->connections[id] = self;
        else
            return;

        // TODO VERY CRITICAL This errors for some reason when re-establishing connections
        auto end = socket.remote_endpoint();
        std::string msg = "Established connection from " + end.address().to_string() + ":"
            + std::to_string(end.port()) + ", ID " + std::to_string(id);
        std::cout << msg << std::endl;
        handler->window->recv_connect(id);
        asio::async_read(socket, in_buf, asio::transfer_exactly(message_header_length),
                         std::bind(&Connection::read_callback, self, _1, _2));

        // TODO CRITICAL Send ADD message to peers for new connection
    }
}

void Connection::read_callback(const asio::error_code& ec, size_t num)
{
    if (!ec) {
        std::istream is(&in_buf);

        uint64_t sender_id;
        uint16_t msg_id;
        uint8_t net_type;
        uint16_t bytes;

        is.read(reinterpret_cast<char *>(&sender_id), sizeof sender_id);
        is.read(reinterpret_cast<char *>(&msg_id), sizeof msg_id);
        is.read(reinterpret_cast<char *>(&net_type), sizeof net_type);
        is.read(reinterpret_cast<char *>(&bytes), sizeof bytes);

        sender_id = ntohll(sender_id);
        msg_id = ntohs(msg_id);
        MessageType type = static_cast<MessageType>(net_type);
        bytes = ntohs(bytes);

        auto self = shared_from_this();
        asio::async_read(socket, in_buf, asio::transfer_exactly(bytes),
                         std::bind(&Connection::read_buffer, self, _1, _2,
                                   sender_id, msg_id, type, bytes));
    } else if (ec != asio::error::eof)
        std::cerr << "Asio read error: ID " << id << " " << ec.value() << ", " << ec.message()
                  << std::endl;
}

void Connection::read_buffer(const asio::error_code& ec, size_t num, uint64_t sender_id,
                             uint16_t msg_id, MessageType type, uint16_t num_bytes)
{
    if (!ec) {
        std::istream is(&in_buf);
        uint8_t cargo[num_bytes];
        is.read(reinterpret_cast<char *>(cargo), num_bytes);

        if (std::find(handler->message_signatures.begin(), handler->message_signatures.end(),
                      std::make_pair(sender_id, msg_id)) == handler->message_signatures.end()) {
            handler->message_signatures.emplace_back(sender_id, msg_id);

            // Perform the message forwarding, sending the message to all connections except
            // the one that sent it (this one), and the original sender
            uint64_t temp_sender_id = htonll(sender_id);
            uint16_t temp_msg_id = htons(msg_id);
            uint8_t temp_type = type;
            uint16_t temp_num_bytes = htons(num_bytes);

            std::array<asio::const_buffer, 5> data = {
                asio::buffer(&temp_sender_id, sizeof temp_sender_id),
                asio::buffer(&temp_msg_id, sizeof temp_msg_id),
                asio::buffer(&temp_type, sizeof temp_type),
                asio::buffer(&temp_num_bytes, sizeof temp_num_bytes),
                asio::buffer(&cargo, sizeof cargo)
            };

            for (auto conn : handler->connections) {
                if (conn.first != id and conn.first != sender_id) {
                    if (not conn.second.expired()) {
                        auto conn_ptr = conn.second.lock();
                        asio::async_write(conn_ptr->socket, data, asio::transfer_all(),
                                          std::bind(&Connection::write_callback, conn_ptr,
                                                    _1, _2));
                    }
                }
            }

            // Parse the message and perform the relavant command
            switch (type) {
            case CHAT:
                read_chat(cargo, num_bytes, sender_id);
                break;
            case ADD:
                // TODO CRITICAL Add ADD command
                break;
            case CONNECT: // Connections just done through discovery server, not used
            case OPEN:    // Only use discovery server for forming connections
            case ACCEPT:
            case ERROR:   // ERROR might have a use later
            case INVALID:
                return;
            }
        } else
            is.clear();

        asio::async_read(socket, in_buf, asio::transfer_exactly(message_header_length),
                         std::bind(&Connection::read_callback, shared_from_this(), _1, _2));
    } else if (ec != asio::error::eof) {
        std::cerr << "Asio read error: ID " << id << " " << ec.value() << ", " << ec.message()
                  << std::endl;
    }
}

void Connection::write_callback(const asio::error_code& ec, size_t num)
{
    if (ec && ec != asio::error::eof)
        std::cerr << "Asio write error: ID " << id << " " << ec.value() << ", " << ec.message()
                  << std::endl;
}

void Connection::read_chat(uint8_t *buf, uint16_t num_bytes, uint64_t sender_id)
{
    //std::string line;
    //std::getline(is, line);
    //std::cout << "Chat from " << sender_id << ": " << line << std::endl;
    std::string msg(reinterpret_cast<char *>(buf), num_bytes);
    handler->window->recv_chat(sender_id, msg);
}

} // namespace peerspeak
