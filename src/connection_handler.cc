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
    queue_write_message(OPEN, asio::buffer(&temp_id, sizeof(temp_id)));
    asio::async_read_until(socket, in_buf, '\n', std::bind(&ConnectionHandler::read_callback, this,
            std::_1, std::_2));
}

void ConnectionHandler::queue_write_message(MessageType type,
    const asio::const_buffer& buf)
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
    std::vector<asio::const_buffer> data;
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

        std::function<void(std::istream&)> read_func;

        switch (type) {
            case CONNECT:
                if (bytes != 6)
                    return;
                read_func = std::bind(&ConnectionHandler::read_connect, this, std::_1);
                break;
            case OPEN:
                if (bytes != 8)
                    return;
                read_func = std::bind(&ConnectionHandler::read_open, this, std::_1);
                break;
            case ERROR:
                read_func = std::bind(&ConnectionHandler::read_error, this, std::_1);
                break;
            case ACCEPT:
            case CHAT:
            case INVALID:
                return;
        }

        size_t read = 0;
        if (bytes > in_buf.size())
            read = bytes - in_buf.size();

        if (bytes < in_buf.size())
            read_buffer(asio::error_code(), 0, read_func);
        else
            asio::async_read(socket, in_buf, asio::transfer_exactly(bytes - in_buf.size()), 
                std::bind(&ConnectionHandler::read_buffer, this, std::_1, std::_2, read_func));
    } else if (ec != asio::error::eof) {
        std::cerr << "Asio read error: " << ec.value() << ", " << ec.message() << std::endl;
    } else {
        std::cerr << "Critical: lost connection to discovery server" << std::endl;
    }
}

void ConnectionHandler::read_buffer(const asio::error_code& ec, size_t num,
    std::function<void(std::istream&)> func)
{
    if (!ec) {
        std::istream is(&in_buf);
        func(is);
        asio::async_read_until(socket, in_buf, '\n', std::bind(&ConnectionHandler::read_callback,
                this, std::_1, std::_2));
    } else if (ec != asio::error::eof) {
        std::cerr << "Asio read error: " << ec.value() << ", " << ec.message() << std::endl;
    } else {
        std::cerr << "Critical: lost connection to discovery server" << std::endl;
    }
}

void ConnectionHandler::write_callback(const asio::error_code& ec, size_t num)
{
    if (ec && ec != asio::error::eof) {
        std::cerr << "Asio write error: " << ec.value() << ", " << ec.message() <<  std::endl;
    }
}

void ConnectionHandler::read_connect(std::istream& is)
{
    // TODO IPv6 support
    std::array<uint8_t, 4> ip_bytes;
    uint16_t port;
    is.read(reinterpret_cast<char *>(ip_bytes.data()), ip_bytes.size());
    is.read(reinterpret_cast<char *>(&port), sizeof(port));
    port = ntohs(port);
    asio::ip::address_v4 addr(ip_bytes);
    asio::ip::tcp::endpoint end(addr, port);
    // TODO perform NAT punchthrough
    std::cout << "CONNECT request to " << addr.to_string() << ":" << port << std::endl;
}

void ConnectionHandler::read_open(std::istream& is)
{
    uint64_t other_id;
    is.read(reinterpret_cast<char *>(&other_id), sizeof(other_id));
    other_id = ntohll(other_id);
    std::cout << "OPEN request from " << other_id << std::endl;
    // TODO show message about incoming request
}

void ConnectionHandler::read_error(std::istream& is)
{
    std::string msg;
    std::getline(is, msg);
    std::cerr << "Error: " << msg << std::endl;
}
