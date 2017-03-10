#include "connection.h"

#include <iostream>

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

static const std::array<std::string, 5> type_strings = {
    "CONNECT", "OPEN", "ACCEPT", "ERROR", "CHAT" };

MessageType parse_message_type(std::string type)
{
    auto pos = std::find(type_strings.begin(), type_strings.end(), type);
    return static_cast<MessageType>(std::distance(type_strings.begin(), pos));
}

std::string get_message_string(MessageType type)
{
    return type_strings[type];
}

Connection::Connection(asio::io_service& io_service, asio::ip::tcp::socket sock,
                       PeerspeakWindow *window,
                       std::map<uint64_t, std::weak_ptr<Connection>>& conns)
    : socket(std::move(sock)),
      timer(io_service, std::chrono::seconds(10)),
      connections(conns)
{
    this->window = window;
}

Connection::~Connection()
{
    auto pos = connections.find(id);
    if (pos != connections.end()) {
        connections.erase(pos);
        std::cout << "Connection ID " << id << " closed" << std::endl;
    }
}

void Connection::close()
{
    socket.close();
}

asio::ip::tcp::endpoint Connection::get_endpoint()
{
    return socket.remote_endpoint();
}

void Connection::start_connection(uint64_t this_id)
{
    uint64_t temp_id = htonll(this_id);
    write_message(OPEN, asio::buffer(&temp_id, sizeof(temp_id)));
    auto self = shared_from_this();
    timer.async_wait(
        [this, self](const asio::error_code& ec) {
            if (!ec) {
                std::cout << "Error: Socket timed out" << std::endl;
                socket.close();
            }
        });
    asio::async_read_until(socket, in_buf, '\n', std::bind(&Connection::read_start_message,
                                                           self, _1, _2));
}

void Connection::write_message(MessageType type, const asio::const_buffer& buf)
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
    asio::async_write(socket, data, asio::transfer_all(), std::bind(&Connection::write_callback,
                                                                    shared_from_this(), _1, _2));
}

void Connection::read_start_message(const asio::error_code& ec, size_t num)
{
    if (!ec) {
        std::string line;
        std::istream is(&in_buf);
        std::getline(is, line);
        size_t idx = line.find(' ');
        if (line.substr(0, idx) != "OPEN")
            return;
        size_t bytes;
        try {
            bytes = std::stoul(line.substr(idx + 1));
            if (bytes != 8)
                return;
        } catch (std::invalid_argument& e) {
            return;
        } catch (std::out_of_range& e) {
            return;
        }
        if (bytes > in_buf.size())
            asio::async_read(socket, in_buf, asio::transfer_exactly(bytes - in_buf.size()),
                             std::bind(&Connection::read_start_buffer,
                                       shared_from_this(), _1, _2));
        else
            read_start_buffer(asio::error_code(), 0);
    }
}

void Connection::read_start_buffer(const asio::error_code& ec, size_t num)
{
    if (!ec) {
        timer.cancel();
        auto self = shared_from_this();
        std::istream is(&in_buf);
        is.read(reinterpret_cast<char *>(&id), sizeof(id));
        id = ntohll(id);
        if (connections.find(id) == connections.end())
            connections[id] = self;
        else
            return;

        auto end = get_endpoint();
        std::cout << "Established connection from " << end.address().to_string() << ":"
                  << end.port() << ", ID " << id << std::endl;
        asio::async_read_until(socket, in_buf, '\n', std::bind(&Connection::read_callback,
                                                               self, _1, _2));
    }
}

void Connection::read_callback(const asio::error_code& ec, size_t num)
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
        auto self = shared_from_this();

        switch (type) {
        case CHAT:
            read_func = std::bind(&Connection::read_chat, self, _1);
            break;
        case CONNECT:
            // TODO CONNECT will be handled in the future
        case OPEN:
            // This might be used with client relays
        case ACCEPT:
            // Then this would also need to be handled
        case ERROR:
            // This might have a use later
        case INVALID:
            return;
        }

        if (bytes > in_buf.size())
            asio::async_read(socket, in_buf, asio::transfer_exactly(bytes - in_buf.size()),
                             std::bind(&Connection::read_buffer, self, _1, _2, read_func));
        else
            read_buffer(asio::error_code(), 0, read_func);
    } else if (ec != asio::error::eof)
        std::cerr << "Asio error: ID " << id << " " << ec.value() << ", " << ec.message()
                  << std::endl;
}

void Connection::read_buffer(const asio::error_code& ec, size_t num,
    std::function<void(std::istream&)> func)
{
    if (!ec) {
        std::istream is(&in_buf);
        func(is);
        asio::async_read_until(socket, in_buf, '\n', std::bind(&Connection::read_callback,
                                                               shared_from_this(), _1, _2));
    } else if (ec != asio::error::eof) {
        std::cerr << "Asio error: ID " << id << " " << ec.value() << ", " << ec.message()
                  << std::endl;
    }
}

void Connection::write_callback(const asio::error_code& ec, size_t num)
{
    if (ec && ec != asio::error::eof)
        std::cerr << "Asio error: ID " << id << " " << ec.value() << ", " << ec.message()
                  << std::endl;
}

void Connection::read_chat(std::istream& is)
{
    std::string line;
    std::getline(is, line);
    std::cout << "Chat from " << id << ": " << line << std::endl;
    window->recv_chat(id, line);
}

} // namespace peerspeak
