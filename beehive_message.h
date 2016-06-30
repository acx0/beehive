#ifndef BEEHIVE_MESSAGE_H
#define BEEHIVE_MESSAGE_H

#include <string>

#include <sys/socket.h>
#include <sys/types.h>

#include "logger.h"

class beehive_message
{
public:
    static bool is_message(const std::string &message_type, const std::string &request);
    static std::string read_message(int socket_fd);
    static void send_message(int socket_fd, const std::string &message);

    static const size_t MAX_SIZE;
    static const std::string LISTEN;
    static const std::string LISTEN_DGRAM;
    static const std::string CONNECT;
    static const std::string ACCEPT;
    static const std::string CLOSE;
    static const std::string SEND_DGRAM;
    static const std::string INVALID;
    static const std::string USED;
    static const std::string FAILED;
    static const std::string OK;
    static const std::string SEPARATOR;
};

#endif
