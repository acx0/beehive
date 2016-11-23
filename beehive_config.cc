#include "beehive_config.h"

const std::string beehive_config::DEFAULT_BEEHIVE_SOCKET_PATH = "beehive0";
const std::string beehive_config::BROADCAST_SERVER_SOCKET_PATH = "beehive_simulated_wireless";

beehive_config::beehive_config()
    : beehive_socket_path(DEFAULT_BEEHIVE_SOCKET_PATH)
{
}

beehive_config::beehive_config(const std::string &beehive_socket_path)
    : beehive_socket_path(beehive_socket_path)
{
}

const std::string beehive_config::get_beehive_socket_path() const
{
    return beehive_socket_path;
}

const std::string beehive_config::get_channel_path_prefix() const
{
    return beehive_socket_path + "_stream";
}

const std::string beehive_config::get_dgram_path_prefix() const
{
    return beehive_socket_path + "_dgram";
}
