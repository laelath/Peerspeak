#include "connection_handler.h"

#include <iostream>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htonll(x) ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32)
#define ntohll(x) ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32)
#else
#define htonll(x) (x)
#define ntohll(x) (x)
#endif

namespace std {
    using namespace placeholders;
} // namespace std

ConnectionHandler::ConnectionHandler(asio::io_service& io_service, asio::ip::tcp::endpoint end,
    uint64_t id)
    : socket(io_service)
{
    socket.connect(end);
    uint64_t temp_id = htonll(id);
    queue_write_message(OPEN, asio::streambuf::const_buffers_type(&temp_id, sizeof(temp_id)));
    asio::async_read_until(socket, in_buf, '\n', std::bind(&ConnectionHandler::read_callback, this,
            std::_1, std::_2));
    uint64_t other_id = htonll(static_cast<uint64_t>(12345123));
    queue_write_message(OPEN, asio::streambuf::const_buffers_type(&other_id, sizeof(other_id)));
}

void ConnectionHandler::queue_write_message(MessageType type,
    const asio::streambuf::const_buffers_type& buf)
{
    switch (type) {
        case CONNECT:
            if (asio::buffer_size(buf) != 4)
                throw std::invalid_argument("CONNECT expected an IPv4 address");
            break;
        case OPEN:
            if (asio::buffer_size(buf) != 8)
                throw std::invalid_argument("OPEN expected a uint64_t id");
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

    std::string header = get_message_string(type) + " "
        + std::to_string(asio::buffer_size(buf)) + "\n";
    std::vector<asio::streambuf::const_buffers_type> data;
    data.push_back(asio::buffer(header));
    data.push_back(buf);
    asio::async_write(socket, data, asio::transfer_all(),
        std::bind(&ConnectionHandler::write_callback, this, std::_1, std::_2));
}

void ConnectionHandler::read_callback(const asio::error_code& ec, size_t num)
{
    if (!ec) {
        std::string line;
        std::istream is(&in_buf);
        std::getline(is, line);

        size_t idx = line.find(' ');
        MessageType type = parse_message_type(line.substr(0, idx));

        size_t bytes;
        try {
            bytes = std::stoul(line.substr(idx + 1));
        } catch (std::invalid_argument& e) {
            return;
        } catch (std::out_of_range& e) {
            return;
        }

        size_t read = 0;
        if (bytes > in_buf.size())
            read = bytes - in_buf.size();

        auto read_func = [this, type, bytes](const asio::error_code& ec, size_t num) {
            std::istream is(&in_buf);
            if (type == CONNECT) {
                // TODO IPv6 support
                if (bytes != 6)
                    return;
                uint8_t ip_bytes[4];
                uint16_t port;
                is.read(reinterpret_cast<char *>(ip_bytes), 4);
                is.read(reinterpret_cast<char *>(&port), sizeof(port));
                port = ntohs(port);
                asio::ip::address_v4 addr(bytes);
                asio::ip::tcp::endpoint end(addr, port);
                // TODO perform NAT punchthrough
                std::cout << "CONNECT request to " << addr.to_string() << ":" << port << std::endl;
            } else if (type == OPEN) {
                if (bytes != 8)
                    return;
                uint64_t other_id;
                is.read(reinterpret_cast<char *>(&other_id), sizeof(other_id));
                other_id = ntohll(other_id);
                std::cout << "Incoming OPEN request from " << other_id << std::endl;
                // TODO show message about incoming request
            } else if (type == ERROR) {
                std::string msg(std::istreambuf_iterator<char>(is), {});
                std::cerr << "Error: " << msg << std::endl;
            } else
                return;
            asio::async_read_until(socket, in_buf, '\n', std::bind(
                    &ConnectionHandler::read_callback, this, std::_1, std::_2));
        };

        if (read == 0)
            read_func(asio::error_code(), 0);
        else
            asio::async_read(socket, in_buf, asio::transfer_exactly(read), read_func);
    } else if (ec != asio::error::eof) {
        std::cerr << "Error: " << ec.value() << ", " << ec.message() << "." << std::endl;
    }
}

void ConnectionHandler::write_callback(const asio::error_code& ec, size_t num)
{
    if (ec && ec != asio::error::eof) {
        std::cerr << "Error: " << ec.value() << ", " << ec.message() << "." << std::endl;
    }
}
