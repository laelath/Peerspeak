#ifndef message_h_INCLUDED
#define message_h_INCLUDED

#include <string>

namespace peerspeak {

enum MessageType {
    CONNECT = 0, // Send ip to peer,         4 byte IPv4 address, 2 byte port
    OPEN,        // Open request to/from id, 8 byte uint64_t
    ADD,         // Add a connection id,     8 byte uint64_t
    REMOVE,
    ACCEPT,      // Accept open request,     1 byte boolean
    ERROR,       // Response from server,    n byte string
    CHAT,        // Chat message,            n byte string
    INVALID,     // Invalid message, not sent
};

// Parse a MessageType into a string, returns INVALID on a parse error.
//MessageType parse_message_type(std::string type);

// Get a string from a MessageType, INVALID can't be converted.
//std::string get_message_string(MessageType type);

} // namespace peerspeak

#endif // message_h_INCLUDED
