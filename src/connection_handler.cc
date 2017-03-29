#include "connection_handler.h"

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

ConnectionHandler::ConnectionHandler()
    : acceptor(io_service),
      socket(io_service),
      conn_sock(io_service),
      acpt_sock(io_service)
{
    socket.open(asio::ip::tcp::v4());
    socket.set_option(asio::ip::tcp::socket::reuse_address(true));

    conn_sock.open(asio::ip::tcp::v4());
    conn_sock.set_option(asio::ip::tcp::socket::reuse_address(true));

    acceptor.open(socket.local_endpoint().protocol());
    acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
}

ConnectionHandler::~ConnectionHandler()
{
    std::cerr << "Critical: lost connection to discovery server" << std::endl;
    window->show_error("Critical: lost connection to discovery server");
}

void ConnectionHandler::init(PeerspeakWindow *window, std::string ip, uint16_t port, uint64_t id)
{
    this->window = window;
    this->id = id;

    asio::ip::address addr = asio::ip::address::from_string(ip);
    asio::ip::tcp::endpoint end(addr, port);

    socket.connect(end);
    conn_sock.bind(socket.local_endpoint());
    acceptor.bind(socket.local_endpoint());
    acceptor.listen(10);
}

void ConnectionHandler::start()
{
    uint64_t temp_id = htonll(id);
    write_message(OPEN, asio::buffer(&temp_id, sizeof(temp_id)));
    asio::async_read_until(socket, in_buf, '\n', std::bind(&ConnectionHandler::read_callback,
                                                           this, _1, _2));
    io_service.run();
}

void ConnectionHandler::write_message(MessageType type, const asio::const_buffer& buf)
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
    std::array<asio::const_buffer, 2> data = {
        asio::buffer(header),
        buf };
    asio::async_write(socket, data, asio::transfer_all(),
                      std::bind(&ConnectionHandler::write_callback, this, _1, _2));
}

void ConnectionHandler::write_message_all(MessageType type, const asio::const_buffer& buf)
{
    for (auto conn : connections)
        if (not conn.second.expired())
            conn.second.lock()->write_message(type, buf);
}

void ConnectionHandler::send_open(uint64_t other_id)
{
    other_id = htonll(other_id);
    io_service.dispatch(
        [this, other_id]() {
            write_message(OPEN, asio::buffer(&other_id, sizeof(other_id)));
        });
}

void ConnectionHandler::send_accept(bool accept)
{
    io_service.dispatch(
        [this, accept]() {
            write_message(ACCEPT, asio::buffer(&accept, sizeof(accept)));
        });
}

void ConnectionHandler::send_chat(std::string msg)
{
    io_service.dispatch(
        [this, msg]() {
            write_message_all(CHAT, asio::buffer(msg));
        });
}

void ConnectionHandler::send_close()
{
    io_service.dispatch(
        [this]() {
            socket.close();
            for (auto conn : connections)
                if (not conn.second.expired())
                    conn.second.lock()->close();
        });
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
            read_func = std::bind(&ConnectionHandler::read_connect, this, _1);
            break;
        case OPEN:
            if (bytes != 8)
                return;
            read_func = std::bind(&ConnectionHandler::read_open, this, _1);
            break;
        case ERROR:
            read_func = std::bind(&ConnectionHandler::read_error, this, _1);
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
                             std::bind(&ConnectionHandler::read_buffer, this,
                                       _1, _2, read_func));
    } else if (ec != asio::error::eof) {
        std::cerr << "Asio read error: " << ec.value() << ", " << ec.message() << std::endl;
    }
}

void ConnectionHandler::read_buffer(const asio::error_code& ec, size_t num,
                                    std::function<void(std::istream&)> func)
{
    if (!ec) {
        std::istream is(&in_buf);
        func(is);
        asio::async_read_until(socket, in_buf, '\n', std::bind(&ConnectionHandler::read_callback,
                                                               this, _1, _2));
    } else if (ec != asio::error::eof) {
        std::cerr << "Asio read error: " << ec.value() << ", " << ec.message() << std::endl;
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
    std::cout << "CONNECT request to " << addr.to_string() << ":" << port << std::endl;
    asio::ip::tcp::endpoint remote(addr, port);
    punchthrough(remote);
}

void ConnectionHandler::read_open(std::istream& is)
{
    uint64_t other_id;
    is.read(reinterpret_cast<char *>(&other_id), sizeof(other_id));
    other_id = ntohll(other_id);
    std::cout << "OPEN request from " << other_id << std::endl;
    window->recv_open(other_id);
}

void ConnectionHandler::read_error(std::istream& is)
{
    std::string msg;
    std::getline(is, msg);
    std::cerr << "Error: " << msg << std::endl;
    window->show_error("Error: " + msg);
}

void ConnectionHandler::punchthrough(asio::ip::tcp::endpoint& remote)
{
    std::cout << "Starting punchthrough to " << remote.address().to_string() << ":"
              << remote.port() << std::endl;

    acceptor.async_accept(acpt_sock, std::bind(&ConnectionHandler::punch_acpt_callback, this, _1));
    conn_sock.async_connect(remote, std::bind(&ConnectionHandler::punch_conn_callback, this, _1));
}

void ConnectionHandler::punch_conn_callback(const asio::error_code& ec)
{
    if (!ec) {
        acceptor.cancel();
        auto conn = std::make_shared<Connection>(io_service, std::move(conn_sock), window,
                                                 connections, message_signatures);
        std::cout << "Punchthrough successful, starting connection" << std::endl;
        conn->start_connection(id);
    } else
        std::cerr << "Punchthrough connect error " << ec.value() << ", " << ec.message()
                  << std::endl;
}

void ConnectionHandler::punch_acpt_callback(const asio::error_code& ec)
{
    if (!ec) {
        conn_sock.cancel();
        auto conn = std::make_shared<Connection>(io_service, std::move(acpt_sock), window,
                                                 connections, message_signatures);
        std::cout << "Punchthrough successful, starting connection" << std::endl;
        conn->start_connection(id);
    } else
        std::cerr << "Punchthrough accept error " << ec.value() << ", " << ec.message()
                  << std::endl;
}

} // namespace peerspeak
