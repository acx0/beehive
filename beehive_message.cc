#include "beehive_message.h"

const size_t beehive_message::MAX_SIZE = 100;
const std::string beehive_message::LISTEN = std::string("LISTEN");
const std::string beehive_message::LISTEN_DGRAM = std::string("LISTEN_DGRAM");
const std::string beehive_message::CONNECT = std::string("CONNECT");
const std::string beehive_message::ACCEPT = std::string("ACCEPT");
const std::string beehive_message::CLOSE = std::string("CLOSE");
const std::string beehive_message::SEND_DGRAM = std::string("SEND_DGRAM");
const std::string beehive_message::NEIGHBOURS = std::string("NEIGHBOURS");;
const std::string beehive_message::NEIGHBOURS_NONE = std::string("<NONE>");;
const std::string beehive_message::INVALID = std::string("INVALID");
const std::string beehive_message::USED = std::string("USED");
const std::string beehive_message::FAILED = std::string("FAILED");
const std::string beehive_message::OK = std::string("OK");
const std::string beehive_message::SEPARATOR = std::string(":");

bool beehive_message::is_message(const std::string &message_type, const std::string &request)
{
    return request.size() >= message_type.size() && request.compare(0, message_type.size(), message_type) == 0;
}

// TODO: better way to indicate to caller that remote connection closed? can have out parameter and return error code
std::string beehive_message::read_message(int socket_fd)
{
    char buffer[MAX_SIZE];
    ssize_t bytes_read = recv(socket_fd, buffer, sizeof(buffer), 0);

    if (bytes_read == 0)
    {
        LOG_ERROR("client connection closed");
        return std::string();
    }
    else if (bytes_read == -1)
    {
        perror("recv");     // TODO: get error string and use LOG_ERROR()
        return std::string();
    }

    return std::string(buffer, bytes_read);
}

void beehive_message::send_message(int socket_fd, const std::string &message)
{
    send(socket_fd, message.c_str(), message.size(), 0);
}
