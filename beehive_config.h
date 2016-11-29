#ifndef BEEHIVE_CONFIG_H
#define BEEHIVE_CONFIG_H

#include <string>

class beehive_config
{
public:
    static const std::string BEEHIVE_SOCKET_PATH_PREFIX;
    static const std::string BROADCAST_SERVER_SOCKET_PATH;

    beehive_config();
    beehive_config(const std::string &beehive_socket_path);

    const std::string get_beehive_socket_path() const;
    const std::string get_channel_path_prefix() const;
    const std::string get_dgram_path_prefix() const;

private:
    std::string beehive_socket_path;
};

#endif
