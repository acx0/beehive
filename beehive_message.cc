#include "beehive_message.h"

const size_t beehive_message::MAX_SIZE = 100;
const std::string beehive_message::LISTEN = std::string("LISTEN");
const std::string beehive_message::LISTEN_DGRAM = std::string("LISTEN_DGRAM");
const std::string beehive_message::CONNECT = std::string("CONNECT");
const std::string beehive_message::ACCEPT = std::string("ACCEPT");
const std::string beehive_message::CLOSE = std::string("CLOSE");
const std::string beehive_message::SEND_DGRAM = std::string("SEND_DGRAM");
const std::string beehive_message::NEIGHBOURS = std::string("NEIGHBOURS");
const std::string beehive_message::NEIGHBOURS_NONE = std::string("<NONE>");
const std::string beehive_message::INVALID = std::string("INVALID");
const std::string beehive_message::USED = std::string("USED");
const std::string beehive_message::FAILED = std::string("FAILED");
const std::string beehive_message::OK = std::string("OK");
const std::string beehive_message::SEPARATOR = std::string(":");

bool beehive_message::is_message(const std::string &message_type, const std::string &request)
{
    return request.size() >= message_type.size()
        && request.compare(0, message_type.size(), message_type) == 0;
}

// TODO: better way to indicate to caller that remote connection closed? can have out parameter and
// return error code
std::string beehive_message::read_message(int socket_fd)
{
    std::vector<uint8_t> buffer;
    ssize_t bytes_read = util::recv(socket_fd, buffer, MAX_SIZE);
    if (bytes_read <= 0)
    {
        return std::string();
    }

    return std::string(buffer.data(), buffer.data() + buffer.size());
}

void beehive_message::send_message(int socket_fd, const std::string &message)
{
    auto bytes = reinterpret_cast<const uint8_t *>(message.c_str());
    std::vector<uint8_t> buffer(bytes, bytes + message.size());
    util::send(socket_fd, buffer);  // TODO: error handling
}
