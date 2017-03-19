#include "message.h"

#include <algorithm>
#include <array>

namespace peerspeak {

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

} // namespace peerspeak
